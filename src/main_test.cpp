
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

static	ICore_1001 *core = NULL;

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


