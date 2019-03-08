
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
#include "secondScreen.hpp"
#include "thread.h"
#include "MultiChVideo.hpp"
#include "cuda_convert.cuh"

#include "encTrans.hpp"

static int defaultEncParamTab0[CORE_CHN_MAX*3][8] = {
	//bitrate; minQP; maxQP;minQI;maxQI;minQB;maxQB;
	{1400000,  -1,    -1,   -1,   -1,   -1,   -1, },//2M
	{2800000,  -1,    -1,   -1,   -1,   -1,   -1, },//4M
	{5600000,  -1,    -1,   -1,   -1,   -1,   -1, }, //8M
	//bitrate; minQP; maxQP;minQI;maxQI;minQB;maxQB;
	{1400000,  -1,    -1,   -1,   -1,   -1,   -1, },//2M
	{2800000,  -1,    -1,   -1,   -1,   -1,   -1, },//4M
	{5600000,  -1,    -1,   -1,   -1,   -1,   -1, }, //8M
	//bitrate; minQP; maxQP;minQI;maxQI;minQB;maxQB;
	{1400000,  -1,    -1,   -1,   -1,   -1,   -1, },//2M
	{2800000,  -1,    -1,   -1,   -1,   -1,   -1, },//4M
	{5600000,  -1,    -1,   -1,   -1,   -1,   -1, }, //8M
	//bitrate; minQP; maxQP;minQI;maxQI;minQB;maxQB;
	{1400000,  -1,    -1,   -1,   -1,   -1,   -1, },//2M
	{2800000,  -1,    -1,   -1,   -1,   -1,   -1, },//4M
	{5600000,  -1,    -1,   -1,   -1,   -1,   -1, } //8M
};
static int defaultEncParamTab1[CORE_CHN_MAX*3][8] = {
	//bitrate; minQP; maxQP;minQI;maxQI;minQB;maxQB;
	{700000,  -1,    -1,   -1,   -1,   -1,   -1, },//2M
	{1400000,  -1,    -1,   -1,   -1,   -1,   -1, },//4M
	{2800000,  -1,    -1,   -1,   -1,   -1,   -1, }, //8M
	//bitrate; minQP; maxQP;minQI;maxQI;minQB;maxQB;
	{700000,  -1,    -1,   -1,   -1,   -1,   -1, },//2M
	{1400000,  -1,    -1,   -1,   -1,   -1,   -1, },//4M
	{2800000,  -1,    -1,   -1,   -1,   -1,   -1, }, //8M
	//bitrate; minQP; maxQP;minQI;maxQI;minQB;maxQB;
	{700000,  -1,    -1,   -1,   -1,   -1,   -1, },//2M
	{1400000,  -1,    -1,   -1,   -1,   -1,   -1, },//4M
	{2800000,  -1,    -1,   -1,   -1,   -1,   -1, }, //8M
	//bitrate; minQP; maxQP;minQI;maxQI;minQB;maxQB;
	{700000,  -1,    -1,   -1,   -1,   -1,   -1, },//2M
	{1400000,  -1,    -1,   -1,   -1,   -1,   -1, },//4M
	{2800000,  -1,    -1,   -1,   -1,   -1,   -1, } //8M
};

CEnc2Rtp* enctran;

static cv::Size channelsImgSize[CORE_CHN_MAX];

static int *userEncParamTab0[CORE_CHN_MAX][3];
static int *userEncParamTab1[CORE_CHN_MAX][3];

static	ICore_1001 *core = NULL;

static char strIpAddr[32] = "192.168.0.172";

static void processFrame_core(int cap_chid,unsigned char *src, struct v4l2_buffer capInfo, int format)
{
	if(capInfo.flags & V4L2_BUF_FLAG_ERROR){
		//OSA_printf("ch%d V4L2_BUF_FLAG_ERROR", cap_chid);
		return;
	}
	if(core != NULL){
		core->processFrame(cap_chid, src, capInfo, format);
	}
}

static int callback_process(void *handle, int chId, Mat frame, struct v4l2_buffer capInfo, int format)
{
	processFrame_core(chId, frame.data, capInfo, format);
	return 0;
}
static void renderHook(int displayId, int stepIdx, int stepSub, int context)
{
	//OSA_printf("%s %d displayId = %d", __FILE__, __LINE__, displayId);
	if(stepIdx == RENDER_HOOK_RUN_SWAP){
		int width = SYS_DIS0_WIDTH;
		int height = SYS_DIS0_HEIGHT;

		glColor3f(0.0, 1.0, 0.0);
		glViewport(0, 0, width, height);
		glRasterPos2f(0.3, 0.3);
		char  str[32] = "0123456789asdf";
		glutBitmapString(GLUT_BITMAP_8_BY_13, (unsigned char *)str);

	}
	if(stepIdx == RENDER_HOOK_RUN_LEAVE){

	}
}


static __inline__ int* getEncParamTab(int chId, int level)
{
	int nEnc = 0;
	//for(int i = 0; i < nValidChannels; i++)
	//	nEnc += enableEncoderFlag[i];
	//if(nEnc>1)
	return userEncParamTab1[chId][level];
	//return userEncParamTab0[chId][level];
}


int main_test(int argc, char **argv)
{

	 CORE1001_INIT_PARAM initParam;
	core = (ICore_1001 *)ICore::Qury(COREID_1001);
	memset(&initParam, 0, sizeof(initParam));
	initParam.renderRC = cv::Rect(SYS_DIS0_X, SYS_DIS0_Y, SYS_DIS0_WIDTH, SYS_DIS0_HEIGHT);
	initParam.renderFPS = SYS_DIS0_FPS;
	initParam.renderSched = 0;
	initParam.nChannels = SYS_CHN_CNT;
	for(int i=0; i<SYS_CHN_CNT; i++){
		initParam.chnInfo[i].imgSize = cv::Size(SYS_CHN_WIDTH(i), SYS_CHN_HEIGHT(i));
		initParam.chnInfo[i].fps = SYS_CHN_FPS(i);
		initParam.chnInfo[i].format = SYS_CHN_FMT(i);
	}
	initParam.bRender = true;
	initParam.bEncoder = false;
	initParam.bHideOSD = false;
	initParam.renderHook = renderHook;

	
	core->init(&initParam, sizeof(initParam));


	
	
	for(int i=0; i<CORE_CHN_MAX; i++){
		userEncParamTab0[i][0] = defaultEncParamTab0[i*3+0];
		userEncParamTab0[i][1] = defaultEncParamTab0[i*3+1];
		userEncParamTab0[i][2] = defaultEncParamTab0[i*3+2];
		userEncParamTab1[i][0] = defaultEncParamTab1[i*3+0];
		userEncParamTab1[i][1] = defaultEncParamTab1[i*3+1];
		userEncParamTab1[i][2] = defaultEncParamTab1[i*3+2];
	}
	
	
	
	ENC2RTP_InitPrm enctranInit;
	memset(&enctranInit, 0, sizeof(enctranInit));
	enctranInit.iTransLevel = 1;
	enctranInit.nChannels = SYS_CHN_CNT;
	
	for(int chId=0; chId<enctranInit.nChannels; chId++){
		CORE1001_CHN_INIT_PARAM *chInf = &initParam.chnInfo[chId];
		int* params = getEncParamTab(chId, 1);
		enctranInit.defaultEnable[chId] = true;
		channelsImgSize[chId].width = 1920;
		channelsImgSize[chId].height = 1080;
		enctranInit.imgSize[chId] = channelsImgSize[chId];
		enctranInit.inputFPS[chId] = chInf->fps;
		enctranInit.encPrm[chId].fps = chInf->fps;
		enctranInit.encPrm[chId].bitrate = params[0];
		enctranInit.encPrm[chId].minQP = params[1];
		enctranInit.encPrm[chId].maxQP = params[2];
		enctranInit.encPrm[chId].minQI = params[3];
		enctranInit.encPrm[chId].maxQI = params[4];
		enctranInit.encPrm[chId].minQB = params[5];
		enctranInit.encPrm[chId].maxQB = params[6];
		enctranInit.srcType[chId] = APPSRC;
	}
	initParam.encStreamIpaddr = strIpAddr;
	if(initParam.encStreamIpaddr != NULL){
		enctranInit.bRtp = true;
		strcpy(enctranInit.destIpaddr, initParam.encStreamIpaddr);
	}

	enctran = new CEnc2Rtp;
	OSA_assert(enctran != NULL);
	enctran->init(&enctranInit);


	MultiChVideo MultiCh;
	MultiCh.m_user = NULL;
	MultiCh.m_usrFunc = callback_process;
	MultiCh.creat();
	MultiCh.run();

	glutMainLoop();

	core->uninit();
	ICore::Release(core);
	core = NULL;

	return 0;
}


