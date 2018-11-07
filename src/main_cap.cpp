/*
 * main.cpp
 *
 *  Created on: 2018年8月23日
 *      Author: fsmdn121
 */
#include <glew.h>
#include <glut.h>
#include <freeglut_ext.h>
#include <linux/videodev2.h>
#include "MultiChVideo.hpp"
#include "Displayer.hpp"
#include "main.h"
#include "cuda_convert.cuh"
#include "osa_image_queue.h"

static int curChannel = 0;
static OSA_BufHndl *imgQ[SYS_CHN_CNT];
static CRender *render = NULL;

void processFrame_cap(int cap_chid,unsigned char *src, struct v4l2_buffer capInfo, int format)
{
	static uint64 timestamp[MAX_CHAN] = {0ul,0ul};
	char WindowName[64]={0};
	char strTmp[64];
	float fFps = 0.0f;
	Mat img;

	if(capInfo.flags & V4L2_BUF_FLAG_ERROR){
		OSA_printf("ch%d V4L2_BUF_FLAG_ERROR", cap_chid);
		return;
	}
	if(cap_chid>=MAX_CHAN)
		return;

	uint64 curStamp = (uint64)capInfo.timestamp.tv_sec*1000000000ul + (uint64)capInfo.timestamp.tv_usec*1000ul;
	//uint64 curStamp = (uint64_t)getTickCount();
	if(timestamp[cap_chid] != 0ul)
		fFps = 1000000000.0/(curStamp - timestamp[cap_chid]);
	timestamp[cap_chid] = curStamp;

	if(curChannel == cap_chid)
	{
		if(format==V4L2_PIX_FMT_YUYV)
		{
			//OSA_printf("%s ch%d %d", __func__, cap_chid, OSA_getCurTimeInMsec());
			static int a = 0;
			if(!a)
			{
				Mat temp;
				OSA_BufInfo* info = image_queue_getEmpty(imgQ[cap_chid]);
				if(info != NULL)
				{
					img = Mat(SYS_CHN_HEIGHT(cap_chid),SYS_CHN_WIDTH(cap_chid),CV_8UC3, info->physAddr);
					temp = Mat(SYS_CHN_HEIGHT(cap_chid),SYS_CHN_WIDTH(cap_chid),CV_8UC2,src);
					sprintf(strTmp, "fps: %.1f", fFps);
					putText(temp, strTmp, cv::Point(50,50), CV_FONT_HERSHEY_COMPLEX, 1,cvScalar(255),1);
					cuConvert_yuv2bgr_yuyv_async(cap_chid, temp, img, CUT_FLAG_devAlloc);
					info->chId = cap_chid;
					info->channels = img.channels();
					info->width = img.cols;
					info->height = img.rows;
					info->timestampCap = (uint64)capInfo.timestamp.tv_sec*1000000000ul
							+ (uint64)capInfo.timestamp.tv_usec*1000ul;
					info->timestamp = (uint64_t)getTickCount();
					image_queue_putFull(imgQ[cap_chid], info);
				}
				else
				{
					OSA_printf("[%d]%s. ch%d %ld.%ld queue overflow!\n", OSA_getCurTimeInMsec(), __func__,cap_chid,
							capInfo.timestamp.tv_sec, capInfo.timestamp.tv_usec);
				}
			}
			//a ^=1;
		}
		else if(format==V4L2_PIX_FMT_GREY)
		{
			//OSA_printf("%s ch%d %d", __func__, cap_chid, OSA_getCurTimeInMsec());
			static int b = 0;
			if(!b)
			{
				OSA_BufInfo* info = image_queue_getEmpty(imgQ[cap_chid]);
				if(info != NULL)
				{
					img = Mat(SYS_CHN_HEIGHT(cap_chid),SYS_CHN_WIDTH(cap_chid),CV_8UC1, info->physAddr);
					cudaMemcpy(info->physAddr, src, img.cols*img.rows, cudaMemcpyHostToDevice);
					info->chId = cap_chid;
					info->channels = img.channels();
					info->width = img.cols;
					info->height = img.rows;
					info->timestampCap = (uint64)capInfo.timestamp.tv_sec*1000000000
							+ (uint64)capInfo.timestamp.tv_usec*1000;
					info->timestamp = (uint64_t)getTickCount();
					//OSA_printf("[%d] %s. ch%d %ld.%ld", OSA_getCurTimeInMsec(), __func__,
					//		cap_chid, capInfo.timestamp.tv_sec, capInfo.timestamp.tv_usec);
					image_queue_putFull(imgQ[cap_chid], info);
				}
				else
				{
					OSA_printf("[%d]%s. ch%d %ld.%ld queue overflow!\n", OSA_getCurTimeInMsec(), __func__,cap_chid,
							capInfo.timestamp.tv_sec, capInfo.timestamp.tv_usec);
				}
			}
			//b ^= 1;
		}
	}
}

static void keyboard_event(unsigned char key, int x, int y)
{
	switch(key)
	{
	case '0':
		curChannel = 0;
		render->dynamic_config(CRender::DS_CFG_ChId, 0, &curChannel);
		break;
	case '1':
		curChannel = 1;
		render->dynamic_config(CRender::DS_CFG_ChId, 0, &curChannel);
		break;
	case 'q':
	case 27:
		glutLeaveMainLoop();
		break;
	default:
		break;
	}
}

static int callback_process(void *handle, int chId, Mat frame, struct v4l2_buffer capInfo, int format)
{
	if(chId>=2)
		return 0;

	/*if(chId == 0){
		static int cc = 0;
		if(cc%100 != 0){
			cc++;
			return 0;
		}
		cc++;
		Mat md = cv::Mat(frame.rows, frame.cols, CV_8UC3);
		cvtColor(frame, md, CV_YUV2BGR_YUYV);
		//cudaMemcpy(md.data,info->physAddr, info->width*info->height*info->channels, cudaMemcpyDeviceToHost);
		imshow("md", md);
		waitKey(1);
		return 0;
	}*/

	processFrame_cap(chId, frame.data, capInfo, V4L2_PIX_FMT_YUYV);
	return 0;
}

int main_cap(int argc, char **argv)
{
	cuConvertInit(SYS_CHN_CNT);

	DS_InitPrm dsInit;
	memset(&dsInit, 0, sizeof(DS_InitPrm));
	render = CRender::createObject();
	render->create();
	OSA_printf("render create success!");
	dsInit.bFullScreen = false;
	dsInit.keyboardfunc = keyboard_event;
	dsInit.nChannels = SYS_CHN_CNT;
	dsInit.memType = memtype_cudev;//memtype_glpbo;//memtype_cudev;
	dsInit.nQueueSize = 2;
	dsInit.disFPS = DIS_FPS;
	dsInit.channelsSize[0].w = SYS_CHN_WIDTH(0);
	dsInit.channelsSize[0].h = SYS_CHN_HEIGHT(0);
	dsInit.channelsSize[0].c = 3;
	dsInit.channelsSize[1].w = SYS_CHN_WIDTH(1);
	dsInit.channelsSize[1].h = SYS_CHN_HEIGHT(1);
	dsInit.channelsSize[1].c = 3;
	render->init(&dsInit);
	render->run();
	render->m_bOsd = true;

	imgQ[0] = &render->m_bufQue[0];
	imgQ[1] = &render->m_bufQue[1];


	MultiChVideo MultiCh;
	MultiCh.m_user = NULL;
	MultiCh.m_usrFunc = callback_process;
	MultiCh.creat();
	MultiCh.run();
 	//OSA_printf("getTickFrequency() = %f", getTickFrequency());

	//glutSetCursor(GLUT_CURSOR_NONE);
	glutMainLoop();

	render->stop();
	render->destroy();
	CRender::destroyObject(render);

	cuConvertUinit();

	return 0;
}
