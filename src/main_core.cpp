/*
 * main_core.cpp
 *
 *  Created on: Sep 27, 2018
 *      Author: wzk
 */

#include <opencv2/opencv.hpp>
#include <glew.h>
#include <glut.h>
#include <freeglut_ext.h>
#include <opencv/cv.hpp>
#include <linux/videodev2.h>
#include <unistd.h>
#include <osa_sem.h>
#include "main.h"
#include "crCore.hpp"
#include "crosd.hpp"
#include "thread.h"
#include "MultiChVideo.hpp"
#include "cuda_convert.cuh"

using namespace cv;

#define WHITECOLOR 		0x008080FF
#define YELLOWCOLOR 	0x009110D2
#define CRAYCOLOR		0x0010A6AA
#define GREENCOLOR		0x00223691
#define MAGENTACOLOR	0x00DECA6A
#define REDCOLOR		0x00F05A51
#define BLUECOLOR		0x006EF029
#define BLACKCOLOR		0x00808010
#define BLANKCOLOR		0x00000000

static ICore_1001 *core = NULL;

void gpuConvertYUYVtoGray(unsigned char *src, unsigned char *dst,
		unsigned int width, unsigned int height);
static void extractYUYV2Gray(unsigned char *data,int ImgWidth, int ImgHeight)
{
#if 0
	uint8_t  *  pDst8_t;
	uint8_t *  pSrc8_t;

	pSrc8_t = (uint8_t*)(data);
	pDst8_t = (uint8_t*)(data);

#pragma omp parallel for
	for(int y = 0; y < ImgHeight*ImgWidth; y++)
	{
		pDst8_t[y] = pSrc8_t[y*2];
	}
#else
	gpuConvertYUYVtoGray(data, data,ImgWidth, ImgHeight);
	//Mat src(ImgHeight, ImgWidth, CV_8UC2, data);
	//cv::cvtColor(src, src, CV_YUV2GRAY_YUYV);
#endif

#if 0
	Mat gray(ImgHeight, ImgWidth, CV_8UC1, data);
	cv::imshow("t",gray);
	cv::waitKey(1);
#endif
}

class FPS
{
public:
	unsigned long nframes;
	double fps;
	double cmin;
	double cmax;
	double cmean;
	int64 tmLast;
	int64 tmStart;
	FPS(void):nframes(0l), tmLast(0l), tmStart(0l),fps(0.0),cmin(1000.0),cmax(0.0),cmean(0.0){};
	virtual ~FPS(){};
	void signal(void){
		nframes ++;
		int64 tm = getTickCount();
		if(tmLast > 0){
			double elapsedTime = (tm - tmLast)*0.000001;
			fps = 1000.f/elapsedTime;
			cmin = min(cmin, fps);
			cmax = max(cmax, fps);
			if(nframes>10)
				cmean = 1000.f*nframes/((tm - tmStart)*0.000001) + 0.0005;
		}else{
			tmStart = tm;
			nframes = 0;
		}
		tmLast = tm;
		if(nframes == 300){
			cmin = fps;
			cmax = 0;
			nframes = 0;
			tmStart = tm;
		}
	}
};

static FPS gcnt[2];
static OSA_SemHndl semNotify;
static FPS prcFps;
static wchar_t strProFPS[128] = L"";

static void processFrame_core(int cap_chid,unsigned char *src, struct v4l2_buffer capInfo, int format)
{
	if(capInfo.flags & V4L2_BUF_FLAG_ERROR){
		OSA_printf("ch%d V4L2_BUF_FLAG_ERROR", cap_chid);
		return;
	}

	gcnt[cap_chid].signal();

	if(core != NULL){
		/*if(cap_chid == 1){
			//Uint32 tstart = OSA_getCurTimeInMsec();
			//extractYUYV2Gray(src, SYS_CHN_WIDTH(cap_chid), SYS_CHN_HEIGHT(cap_chid));
			//Uint32 elapsedTime = (OSA_getCurTimeInMsec() - tstart);
			//OSA_printf("%d", elapsedTime);
			static unsigned char *mem = NULL;
			if(mem == NULL){
				cudaHostAlloc(&mem, SYS_CHN_WIDTH(cap_chid)*SYS_CHN_HEIGHT(cap_chid), cudaHostAllocDefault);
				memset(mem, 128, SYS_CHN_WIDTH(cap_chid)*SYS_CHN_HEIGHT(cap_chid));
				Mat gray(SYS_CHN_HEIGHT(cap_chid), SYS_CHN_WIDTH(cap_chid), CV_8UC1, mem);
				cv::line(gray, Point(800, 140-10), Point(800, 140+10), cv::Scalar(255), 20, 8, 0);
				cv::line(gray, Point(700, 240-10), Point(700, 240+10), cv::Scalar(254), 20, 8, 0);
				cv::line(gray, Point(600, 340-10), Point(600, 340+10), cv::Scalar(253), 20, 8, 0);
				cv::line(gray, Point(500, 440-10), Point(500, 440+10), cv::Scalar(3), 20, 8, 0);
				cv::line(gray, Point(400, 540-10), Point(400, 540+10), cv::Scalar(2), 20, 8, 0);
				cv::line(gray, Point(300, 640-10), Point(300, 640+10), cv::Scalar(1), 20, 8, 0);
				cv::line(gray, Point(200, 740-10), Point(200, 740+10), cv::Scalar(0), 20, 8, 0);
			}
			src = mem;
			format = V4L2_PIX_FMT_GREY;
		}*/
		core->processFrame(cap_chid, src, capInfo, format);
	}
}

enum {
	KEYDIR_UP = 0, KEYDIR_DOWN, KEYDIR_LEFT, KEYDIR_RIGHT
};
static void Track_armRefine(int dir, int step = 1)
{
	switch(dir)
	{
	case KEYDIR_UP:
		{
			cv::Point raf(0, -1*step);
			core->setTrackPosRef(raf);
		}
		break;
	case KEYDIR_DOWN:
		{
			cv::Point raf(0, step);
			core->setTrackPosRef(raf);
		}
		break;
	case KEYDIR_LEFT:
		{
			cv::Point raf(-1*step, 0);
			core->setTrackPosRef(raf);
		}
		break;
	case KEYDIR_RIGHT:
		{
			cv::Point raf(step, 0);
			core->setTrackPosRef(raf);
		}
		break;
	default:
		break;
	}
}

static void Axis_move(int dir, int step = 1)
{
	cv::Point2f curPos = core->m_stats.chn[core->m_stats.mainChId].axis;
	cv::Point pos;
	switch(dir)
	{
	case KEYDIR_UP:
		{
			pos = cv::Point(curPos.x+0.5, curPos.y+0.5-1*step);
			core->setAxisPos(pos);
		}
		break;
	case KEYDIR_DOWN:
		{
			pos = cv::Point(curPos.x+0.5, curPos.y+0.5+1*step);
			core->setAxisPos(pos);
		}
		break;
	case KEYDIR_LEFT:
		{
			pos = cv::Point(curPos.x+0.5-1*step, curPos.y+0.5);
			core->setAxisPos(pos);
		}
		break;
	case KEYDIR_RIGHT:
		{
			pos = cv::Point(curPos.x+0.5+1*step, curPos.y+0.5);
			core->setAxisPos(pos);
		}
		break;
	default:
		break;
	}
	OSA_printf("%s %d: cur(%f,%f) to(%d %d)", __func__, __LINE__,curPos.x, curPos.y, pos.x, pos.y);
}

static int iMenu = 0;
static int chrChId = 0;
static void keyboard_event(unsigned char key, int x, int y)
{
	static cv::Size winSize(80, 60);
	static int fovId[2] = {0,0};
	static bool mmtdEnable = false;
	static bool trkEnable = false;
	static bool motionDetect = false;
	static bool blobEnable = false;

	char strMenus[2][1024] ={
			"\n"
			"----------------------------------------\n"
			"|---Main Menu -------------------------|\n"
			"----------------------------------------\n"
			" [t] Main Channel Choice TV And Fov++   \n"
			" [f] Main Channel Choice FLR And Fov++  \n"
			" [a] Enable/Disable Track               \n"
			" [b] Enable/Disable MMTD                \n"
			" [c] Enable/Disable Enh                 \n"
			" [d] Change EZoomx (X1/X2/X4)           \n"
			" [e] Enable/Disable OSD                 \n"
			" [g] Change OSD Color                   \n"
			" [h] Force Track To Coast               \n"
			" [i][k][j][l] Refine Track Pos          \n"
			" [m] Setup Axis                         \n"
			" [n] Start/Pause Encoder transfer       \n"
			" [u] Change EncTrans level (0/1/2)      \n"
			" [o] Enable/Disable motion detect       \n"
			" [p] Change Pinp channel ID (0/1/null)  \n"
			" [r] Bind/Unbind blend TV BY Flr        \n"
			" [s] Enable/Disable Blob detect         \n"
			" [v] Move sub window postion            \n"
			" [w] EZoomx sub window video            \n"
			" [1].[5] Enable Track By MMTD           \n"
			" [esc][q]Quit                           \n"
			"--> ",

			"\n"
			"----------------------------------------\n"
			"|---Axis Menu -------------------------|\n"
			"----------------------------------------\n"
			" [t] Main Channel Choice TV And Fov++   \n"
			" [f] Main Channel Choice FLR And Fov++  \n"
			" [i] Move Up                            \n"
			" [k] Move Down                          \n"
			" [j] Move Left                          \n"
			" [l] Move Right                         \n"
			" [s] Save to file                       \n"
			" [esc][q]Back                           \n"
			"--> ",
	};

	switch(key)
	{
	case 't':
		if(chrChId == 0)
			fovId[chrChId] = (fovId[chrChId]<4-1) ? (fovId[chrChId]+1) : 0;
		chrChId = 0;
		winSize.width *= SYS_CHN_WIDTH(chrChId)/1920;
		winSize.height *= SYS_CHN_HEIGHT(chrChId)/1080;
		core->setMainChId(chrChId, fovId[chrChId], 0, winSize);
		break;
	case 'f':
		OSA_printf("%s %d: ...", __func__, __LINE__);
		if(chrChId == 1)
			fovId[chrChId] = (fovId[chrChId]<4-1) ? (fovId[chrChId]+1) : 0;
		chrChId = 1;
		winSize.width *= SYS_CHN_WIDTH(chrChId)/1920;
		winSize.height *= SYS_CHN_HEIGHT(chrChId)/1080;
		//OSA_printf("%s %d: ...", __func__, __LINE__);
		core->setMainChId(chrChId, fovId[chrChId], 0, winSize);
		OSA_printf("%s %d: ...leave\n", __func__, __LINE__);
		break;
	case 'u':
		static int speedLevel = 0;
		speedLevel++;
		if(speedLevel>2)
			speedLevel=0;
		core->setEncTransLevel(speedLevel);
		break;
	case 'a':
		trkEnable ^= 1;
		winSize.width = 60; winSize.height = 45;
		core->enableTrack(trkEnable, winSize, true);
		//Track_armRefine(KEYDIR_UP, 1);
		break;
	//case 'w':
	//	winSize.width = 80; winSize.height = 60;
	//	core->enableTrack(trkEnable, winSize, true);
	//	break;
	case 'b':
		mmtdEnable ^=1;
		core->enableMMTD(mmtdEnable, 5);
		break;
	case 'o':
		motionDetect ^=1;
		core->enableMotionDetect(motionDetect);
		break;
	case 'c':
		static bool enhEnable[2] = {false, false};
		enhEnable[chrChId] ^= 1;
		core->enableEnh(enhEnable[chrChId]);
		break;
	case 'd':
		static int ezoomx[2] = {1, 1};
		ezoomx[chrChId] = (ezoomx[chrChId] == 4) ? 1 : ezoomx[chrChId]<<1;
		core->setEZoomx(ezoomx[chrChId]);
		break;
	case 'e':
		static bool osdEnable = true;
		osdEnable ^= 1;
		core->enableOSD(osdEnable);
		break;
	case 'g':
		static int colorTab[] = {WHITECOLOR,YELLOWCOLOR,CRAYCOLOR,GREENCOLOR,MAGENTACOLOR,REDCOLOR,BLUECOLOR,BLACKCOLOR};
		static int icolor = 0;
		icolor = (icolor<sizeof(colorTab)/sizeof(int)-1) ? (icolor+1) : 0;
		core->setOSDColor(colorTab[icolor]);
		cr_osd::set(colorTab[icolor]);
		break;
	case 'h':
		static bool coastEnable = false;
		coastEnable ^= 1;
		core->setTrackCoast(((coastEnable) ? -1: 0));
		break;
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
		winSize.width *= SYS_CHN_WIDTH(chrChId)/1920;
		winSize.height *= SYS_CHN_HEIGHT(chrChId)/1080;
		if(core->enableTrackByMMTD(key-'1', &winSize, false)==OSA_SOK){
			trkEnable = true;
			mmtdEnable = false;
			core->enableMMTD(mmtdEnable, 0);
		}
		break;
	case 'i'://move up
		if(iMenu == 0 && trkEnable)
			Track_armRefine(KEYDIR_UP);
		if(iMenu == 1)
			Axis_move(KEYDIR_UP);
		break;
	case 'k'://move down
		if(iMenu == 0 && trkEnable)
			Track_armRefine(KEYDIR_DOWN);
		if(iMenu == 1)
			Axis_move(KEYDIR_DOWN);
		break;
	case 'j'://move left
		if(iMenu == 0 && trkEnable)
			Track_armRefine(KEYDIR_LEFT);
		if(iMenu == 1)
			Axis_move(KEYDIR_LEFT);
		break;
	case 'l'://move right
		if(iMenu == 0 && trkEnable)
			Track_armRefine(KEYDIR_RIGHT);
		if(iMenu == 1)
			Axis_move(KEYDIR_RIGHT);
		break;
	case 'm':
		printf("%s",strMenus[iMenu]);
		iMenu = 1;
		break;
	case 'n':
		static bool encEnable[2] = {true, true};
		encEnable[chrChId] ^=1;
		core->enableEncoder(chrChId, encEnable[chrChId]);
		break;
	case 's':
		if(iMenu == 1)
			core->saveAxisPos();
		else{
			blobEnable ^= 1;
			core->enableBlob(blobEnable);
		}
		break;
	case 'p':
		static int subChId = -1;
		subChId++;
		if(subChId==2)
			subChId = -1;
		core->setSubChId(subChId);
		break;
	case 'r':
	{
		static int blendchId = -1;
		blendchId = (blendchId == -1) ? 0 : -1;
		cv::Matx44f matricScale;
		cv::Matx44f matricRotate;
		cv::Matx44f matricTranslate;
		matricScale = cv::Matx44f::eye();
		matricRotate = cv::Matx44f::eye();
		matricTranslate = cv::Matx44f::eye();
		//matricScale.val[0] = 0.5f;
		//matricScale.val[5] = 0.5f;
		float rads = float(0) * 0.0174532925f;
		matricRotate.val[0] = cos(rads);
		matricRotate.val[1] = -sin(rads);
		matricRotate.val[4] = sin(rads);
		matricRotate.val[5] = cos(rads);
		//matricTranslate.val[3] = 0.5f;
		//matricTranslate.val[7] = -0.5f;
		core->bindBlend(1, blendchId, (matricTranslate*matricRotate*matricScale).t());
	}
		break;

	case 'v':
	{
		static int posflag = -1;
		posflag++;
		posflag = (posflag >= 4) ? 0 : posflag;
		cv::Rect rc;
		int width = 1920, height = 1080;
		switch(posflag)
		{
		case 1:
			rc = cv::Rect(width*2/3, 0, width/3, height/3);
			break;
		case 2:
			rc = cv::Rect(0, 0, width/3, height/3);
			break;
		case 3:
			rc = cv::Rect(0, height*2/3, width/3, height/3);
			break;
		case 0:
		default:
			rc = cv::Rect(width*2/3, height*2/3, width/3, height/3);
			break;
		}
		core->setWinPos(1, rc);
	}
		break;
	case 'w':
	{
		static int ezoomxSub = 1;
		ezoomxSub = (ezoomxSub == 8) ? 1 : ezoomxSub<<1;
		cv::Matx44f matricScale;
		matricScale = cv::Matx44f::eye();
		matricScale.val[0] = ezoomxSub;
		matricScale.val[5] = ezoomxSub;
		core->setWinMatric(1, matricScale.t());
	}
		break;
	case 'q':
	case 27:
		if(iMenu == 0)
			glutLeaveMainLoop();
		else
			iMenu = 0;
		break;
	default:
		printf("%s",strMenus[iMenu]);
		break;
	}
}


/*********************************************************
 *
 * test main
 */
static void *thrdhndl_glutloop(void *context)
{
	for(;;){
		int key = getc(stdin);
		if(key == 10)
			continue;
		if(key == 'q' || key == 'Q' || key == 27){
			if(iMenu == 0)
				break;
			else
				iMenu = 0;
		}
		keyboard_event(key, 0, 0);
	}
	if(*(bool*)context)
		glutLeaveMainLoop();
	//exit(0);
	return NULL;
}
static void *thrdhndl_keyevent(void *context)
{
	for(;;){
		int key = getchar();//getc(stdin);
		if(iMenu == 0 && (key == 'q' || key == 'Q' || key == 27)){
			break;
		}
		//OSA_printf("key = %d\n\r", key);
		keyboard_event(key, 0, 0);
	}
	if(*(bool*)context)
		glutLeaveMainLoop();
	//exit(0);
	return NULL;
}
static void *thrdhndl_notify( void * p )
{
	using namespace cr_osd;
	int64 tm = getTickCount();
	//static std::vector<float> vArray;
	static std::vector<float> vArray;
	vArray.resize(300);
	for(int i=0; i<300; i++)
		vArray[i] = sin(i*10*0.017453292519943296);
	IPattern* pat = IPattern::Create(&vArray, cv::Rect(0, 0, 600, 200));
	while( *(bool*)p )
	{
		OSA_semWait(&semNotify, OSA_TIMEOUT_FOREVER);
		int64 tm2 = getTickCount();
		float interval = float((tm2-tm)*0.000000001f);
		tm = tm2;
		prcFps.signal();
		vArray.erase(vArray.begin());
		vArray.push_back(interval*10.0);
		swprintf(strProFPS, 128, L"PRC FPS: %.2f (%.2f %.2f)", prcFps.cmean,prcFps.cmax,prcFps.cmin);
	}
	cr_osd::IPattern::Destroy(pat);
}
static void *thrdhndl_timer( void * p )
{
	struct timeval timeout;
	wchar_t strTmp[64] = L"";
	wchar_t strFov[2][64] = {L"",L""};
	wchar_t strFPS[2][64] = {L"",L""};
	cv::Point posTmp;
	CORE1001_STATS stats;

	unsigned int tmpValue = 0;

	cr_osd::put(strTmp, cv::Point(50,45), cvScalar(255,255,255,255));
	cr_osd::put(strFov[0], cv::Point(50,45*2), cvScalar(255,255,255,255));
	cr_osd::put(strFov[1], cv::Point(50,45*3), cvScalar(255,255,255,255));
	cr_osd::put(strFPS[0], cv::Point(50,45*4), cvScalar(255,255,255,255));
	cr_osd::put(strFPS[1], cv::Point(50,45*5), cvScalar(255,255,255,255));
	cr_osd::put(strProFPS, cv::Point(50,45*6), cvScalar(255,255,255,255));
	cr_osd::put(&tmpValue, L"value = %d", cv::Point(50,45*7), cvScalar(255,255,255,255));
	cr_osd::put(&chrChId, 2, cv::Point(50, 45*8), cvScalar(255, 0, 0, 255), L"嵌润信息科技 TV", L"自动视频跟踪 FLR");

	cr_osd::Line line1;
	line1.draw(cv::Point(50, 45*9), cv::Point(265, 45*9), cvScalar(255, 0,0,255), 2);

	cr_osd::Polygon polygon1(3, GL_LINE_LOOP);
	cr_osd::Polygon polygon2(3, GL_POLYGON);
	std::vector<cv::Point> vpts;
	vpts.clear();
	vpts.push_back(cv::Point(300, 440));
	vpts.push_back(cv::Point(320, 440));
	vpts.push_back(cv::Point(310, 470));
	polygon1.draw(vpts, cvScalar(0, 0, 255,255), 2);
	vpts.clear();
	vpts.push_back(cv::Point(310, 410));
	vpts.push_back(cv::Point(300, 440));
	vpts.push_back(cv::Point(320, 440));
	polygon2.draw(vpts, cvScalar(255, 0, 0,255), 2);



	while( *(bool*)p )
	{
		timeout.tv_sec = 0;
		timeout.tv_usec = 100*1000;
		select( 0, NULL, NULL, NULL, &timeout );
		memcpy(&stats, &core->m_stats, sizeof(stats));
		struct tm curTmt;
		time_t curTm;
		time(&curTm);
		memcpy(&curTmt,localtime(&curTm),sizeof(curTmt));
		swprintf(strTmp, 64, L"%04d-%02d-%02d %02d:%02d:%02d",
				curTmt.tm_year+1900, curTmt.tm_mon+1, curTmt.tm_mday,
				curTmt.tm_hour, curTmt.tm_min, curTmt.tm_sec);
		swprintf(strFov[0], 64, L"ch0 FOV: %d", stats.chn[0].fovId);
		swprintf(strFov[1], 64, L"ch1 FOV: %d", stats.chn[1].fovId);
		swprintf(strFPS[0], 64, L"ch0 FPS: %.2f (%.2f %.2f)", gcnt[0].cmean,gcnt[0].cmax,gcnt[0].cmin);
		swprintf(strFPS[1], 64, L"ch1 FPS: %.2f (%.2f %.2f)", gcnt[1].cmean,gcnt[1].cmax,gcnt[1].cmin);
		//swprintf(strProFPS, 128, L"PRC FPS: %.2f (%.2f %.2f)", prcFps.cmean,prcFps.cmax,prcFps.cmin);
		//posTmp = cv::Point(stats.chn[0].axis.x, stats.chn[0].axis.y);
		posTmp = cv::Point(stats.trackPos.x, stats.trackPos.y);
		//cv::circle(core->m_dc[0], posTmp, 16, cvScalar(255), 2);
		tmpValue += 1;
	}
	return NULL;
}

static int encParamTab_T18[][8] = {
	//bitrate; minQP; maxQP;minQI;maxQI;minQB;maxQB;
	{1400000,  41,    51,   41,   51,   -1,   -1, },//2M
	{2800000,  38,    51,   38,   51,   -1,   -1, },//4M
	{5600000,  34,    51,   34,   51,   -1,   -1, } //8M
};
static int encParamTab[][8] = {
	//bitrate; minQP; maxQP;minQI;maxQI;minQB;maxQB;
	{1400000,  -1,    -1,   -1,   -1,   -1,   -1, },//2M
	{2800000,  -1,    -1,   -1,   -1,   -1,   -1, },//4M
	{5600000,  -1,    -1,   -1,   -1,   -1,   -1, } //8M
};

static int callback_process(void *handle, int chId, Mat frame, struct v4l2_buffer capInfo, int format)
{
	if(chId>=2)
		return 0;

	processFrame_core(chId, frame.data, capInfo, V4L2_PIX_FMT_YUYV);
	return 0;
}

static CORE1001_INIT_PARAM initParam;
static bool bLoop = true;
static char strIpAddr[32] = "192.168.1.88";
int main_core(int argc, char **argv)
{
#if defined(__linux__)
    setenv ("DISPLAY", ":1", 0);
    printf("\n %s\n",getenv("DISPLAY"));
#endif
	core = (ICore_1001 *)ICore::Qury(COREID_1001);
	memset(&initParam, 0, sizeof(initParam));
	initParam.nChannels = SYS_CHN_CNT;
	initParam.renderFPS = DIS_FPS;
	initParam.chnInfo[0].imgSize = cv::Size(SYS_CHN_WIDTH(0), SYS_CHN_HEIGHT(0));
	initParam.chnInfo[0].fps = SYS_CHN_FPS(0);
	initParam.chnInfo[0].format = V4L2_PIX_FMT_YUYV;
	initParam.chnInfo[1].imgSize = cv::Size(SYS_CHN_WIDTH(1), SYS_CHN_HEIGHT(1));
	initParam.chnInfo[1].fps = SYS_CHN_FPS(1);
	initParam.chnInfo[1].format = V4L2_PIX_FMT_YUYV;
	initParam.encoderParamTab[0] = encParamTab[0];
	initParam.encoderParamTab[1] = encParamTab[1];
	initParam.encoderParamTab[2] = encParamTab[2];
	initParam.bRender = true;
	initParam.bEncoder = false;
	initParam.bHideOSD = false;
	OSA_semCreate(&semNotify, 1, 0);
	initParam.notify = &semNotify;
	if(argc>=2){
		initParam.bEncoder = true;
		strcpy(strIpAddr, argv[1]);
	}
	initParam.encStreamIpaddr = strIpAddr;
	core->init(&initParam, sizeof(initParam));
	start_thread(thrdhndl_timer, &bLoop);
	start_thread(thrdhndl_notify, &bLoop);

	MultiChVideo MultiCh;
	MultiCh.m_user = NULL;
	MultiCh.m_usrFunc = callback_process;
	MultiCh.creat();
	MultiCh.run();

	if(initParam.bRender){
		start_thread(thrdhndl_keyevent, &initParam.bRender);
		glutKeyboardFunc(keyboard_event);
		glutMainLoop();
	}else{
		thrdhndl_keyevent(&initParam.bRender);
	}
	bLoop = false;
	core->uninit();
	ICore::Release(core);
	core = NULL;
	OSA_semDelete(&semNotify);

	return 0;
}


