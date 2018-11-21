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
#include "main.h"
#include "crCore.hpp"
#include "thread.h"
#include "osa_mutex.h"
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

static int64 tmCap[2] = {0, 0};
static double gfps[2] = {0, 0};
static double gfps_min[2] = {1000, 1000};
static double gfps_max[2] = {0, 0};
static double gfps_mean[2] = {0, 0};
static int64 gfps_tmStart[2] = {0, 0};
static unsigned long gfps_count[2] = {0, 0};

static void processFrame_core_file(int cap_chid,unsigned char *src, struct v4l2_buffer capInfo, int format)
{
	if(capInfo.flags & V4L2_BUF_FLAG_ERROR){
		OSA_printf("ch%d V4L2_BUF_FLAG_ERROR", cap_chid);
		return;
	}

	int64 tm = getTickCount();
	gfps_count[cap_chid]++;
	if(tmCap[cap_chid] > 0){
		double elapsedTime = (tm - tmCap[cap_chid])*0.000001;
		gfps[cap_chid] = 1000.f/elapsedTime;
		gfps_min[cap_chid] = min(gfps_min[cap_chid], gfps[cap_chid]);
		gfps_max[cap_chid] = max(gfps_max[cap_chid], gfps[cap_chid]);
		if(gfps_count[cap_chid]>10)
		gfps_mean[cap_chid] = 1000.f*gfps_count[cap_chid]/((tm - gfps_tmStart[cap_chid])*0.000001) + 0.0005;
	}else{
		gfps_tmStart[cap_chid] = tm;
		gfps_count[cap_chid] = 0;
	}
	tmCap[cap_chid] = tm;
	if(gfps_count[cap_chid] == 300){
		gfps_min[cap_chid] = gfps[cap_chid];
		gfps_max[cap_chid] = gfps[cap_chid];
		//gfps_mean[cap_chid] = gfps[cap_chid];
		gfps_count[cap_chid] = 0;
		gfps_tmStart[cap_chid] = tm;
	}

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
static void keyboard_event(unsigned char key, int x, int y)
{
	static cv::Size winSize(80, 60);
	static int chId = 0;
	static int fovId[2] = {0,0};
	static bool mmtdEnable = false;
	static bool trkEnable = false;
	static bool motionDetect = false;
	static bool blobEnable = false;

	char strMenus[2][1024] ={
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
			" [1].[5] Enable Track By MMTD           \n"
			" [esc][q]Quit                           \n"
			"--> ",

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
		if(chId == 0)
			fovId[chId] = (fovId[chId]<4-1) ? (fovId[chId]+1) : 0;
		chId = 0;
		winSize.width *= SYS_CHN_WIDTH(chId)/1920;
		winSize.height *= SYS_CHN_HEIGHT(chId)/1080;
		core->setMainChId(chId, fovId[chId], 0, winSize);
		break;
	case 'f':
		OSA_printf("%s %d: ...", __func__, __LINE__);
		if(chId == 1)
			fovId[chId] = (fovId[chId]<4-1) ? (fovId[chId]+1) : 0;
		chId = 1;
		winSize.width *= SYS_CHN_WIDTH(chId)/1920;
		winSize.height *= SYS_CHN_HEIGHT(chId)/1080;
		//OSA_printf("%s %d: ...", __func__, __LINE__);
		core->setMainChId(chId, fovId[chId], 0, winSize);
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
	case 'w':
		winSize.width = 80; winSize.height = 60;
		core->enableTrack(trkEnable, winSize, true);
		break;
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
		enhEnable[chId] ^= 1;
		core->enableEnh(enhEnable[chId]);
		break;
	case 'd':
		static int ezoomx[2] = {1, 1};
		ezoomx[chId] = (ezoomx[chId] == 4) ? 1 : ezoomx[chId]<<1;
		core->setEZoomx(ezoomx[chId]);
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
		winSize.width *= SYS_CHN_WIDTH(chId)/1920;
		winSize.height *= SYS_CHN_HEIGHT(chId)/1080;
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
		encEnable[chId] ^=1;
		core->enableEncoder(chId, encEnable[chId]);
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

static void *thrdhndl_timer( void * p )
{
	struct timeval timeout;
	char strTmp[64];
	char strFov[2][16];
	char strFPS[2][64];
	cv::Point posTmp;
	bool bHide = false;
	CORE1001_STATS stats;

	VideoCapture cap;
	//cap.open("mvDetect.h264");
	cap.open("test_fr.h264");
	assert(cap.isOpened());

	while( *(bool*)p )
	{
		timeout.tv_sec = 0;
		timeout.tv_usec = 30*1000;
		select( 0, NULL, NULL, NULL, &timeout );

		Mat frame;
		cap >> frame;
		struct v4l2_buffer capInfo;
		memset(&capInfo, 0, sizeof(capInfo));
		processFrame_core_file(0, frame.data, capInfo, V4L2_PIX_FMT_BGR24);
		//OSA_printf("%s: %d x %d", __func__, frame.cols, frame.rows);

		//cvtColor(prev, prev_grey, COLOR_BGR2GRAY);
		//imshow("file", frame);
		//waitKey(1);


		memcpy(&stats, &core->m_stats, sizeof(stats));
		struct tm curTmt;
		time_t curTm;
		time(&curTm);
		memcpy(&curTmt,localtime(&curTm),sizeof(curTmt));
		if(bHide){
			putText(core->m_dc[0], strTmp, cv::Point(50,50), CV_FONT_HERSHEY_COMPLEX, 1.2,cvScalar(0),1);
			putText(core->m_dc[1], strTmp, cv::Point(50,50), CV_FONT_HERSHEY_COMPLEX, 1.2,cvScalar(0),1);
			putText(core->m_dc[0], strFov[0], cv::Point(50,50*2), CV_FONT_HERSHEY_COMPLEX, 1.2,cvScalar(0),1);
			putText(core->m_dc[1], strFov[1], cv::Point(50,50*3), CV_FONT_HERSHEY_COMPLEX, 1.2,cvScalar(0),1);
			putText(core->m_dc[0], strFPS[0], cv::Point(50,50*4), CV_FONT_HERSHEY_COMPLEX, 1.2,cvScalar(0),1);
			putText(core->m_dc[1], strFPS[1], cv::Point(50,50*5), CV_FONT_HERSHEY_COMPLEX, 1.2,cvScalar(0),1);
			//cv::circle(core->m_dc[0], posTmp, 16, cvScalar(0), 4);
		}
		sprintf(strTmp, "%04d-%02d-%02d %02d:%02d:%02d",
				curTmt.tm_year+1900, curTmt.tm_mon+1, curTmt.tm_mday,
				curTmt.tm_hour, curTmt.tm_hour, curTmt.tm_sec);
		sprintf(strFov[0], "ch0 FOV: %d", stats.chn[0].fovId);
		sprintf(strFov[1], "ch1 FOV: %d", stats.chn[1].fovId);
		sprintf(strFPS[0], "ch0 FPS: %.2f (%.2f %.2f)", gfps_mean[0],gfps_max[0],gfps_min[0]);
		sprintf(strFPS[1], "ch1 FPS: %.2f (%.2f %.2f)", gfps_mean[1],gfps_max[1],gfps_min[1]);
		putText(core->m_dc[0], strTmp, cv::Point(50,50), CV_FONT_HERSHEY_COMPLEX, 1.2,cvScalar(255),1);
		putText(core->m_dc[1], strTmp, cv::Point(50,50), CV_FONT_HERSHEY_COMPLEX, 1.2,cvScalar(255),1);
		putText(core->m_dc[0], strFov[0], cv::Point(50,50*2), CV_FONT_HERSHEY_COMPLEX, 1.2,cvScalar(255),1);
		putText(core->m_dc[1], strFov[1], cv::Point(50,50*3), CV_FONT_HERSHEY_COMPLEX, 1.2,cvScalar(255),1);
		putText(core->m_dc[0], strFPS[0], cv::Point(50,50*4), CV_FONT_HERSHEY_COMPLEX, 1.2,cvScalar(255),1);
		putText(core->m_dc[1], strFPS[1], cv::Point(50,50*5), CV_FONT_HERSHEY_COMPLEX, 1.2,cvScalar(255),1);
		//posTmp = cv::Point(stats.chn[0].axis.x, stats.chn[0].axis.y);
		posTmp = cv::Point(stats.trackPos.x, stats.trackPos.y);
		//cv::circle(core->m_dc[0], posTmp, 16, cvScalar(255), 2);
		bHide = true;
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

	processFrame_core_file(chId, frame.data, capInfo, V4L2_PIX_FMT_YUYV);
	return 0;
}

static CORE1001_INIT_PARAM initParam;
static bool bLoop = true;
static char strIpAddr[32] = "192.168.1.88";
int main_core_file(int argc, char **argv)
{
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
	if(argc>=2){
		initParam.bEncoder = true;
		strcpy(strIpAddr, argv[1]);
	}
	initParam.encStreamIpaddr = strIpAddr;
	core->init(&initParam, sizeof(initParam));
	start_thread(thrdhndl_timer, &bLoop);

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

	return 0;
}


