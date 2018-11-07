/*
 * crCore.hpp
 *
 *  Created on: Sep 27, 2018
 *      Author: wzk
 */

#ifndef CRCORE_HPP_
#define CRCORE_HPP_

/***********************************************
 * core version 1.0.3
 */
#include "osa.h"
#include "osa_sem.h"

#define CORE_1001_VERSION_  "1.0.3"
#define COREID_1001			(0x10010000)
#define COREID_1001_040		(CORNID_1001 + 1)

class ICore
{
public:
	static ICore* Qury(int coreID=COREID_1001);
	static void Release(ICore* core);
	virtual int init(void *pParam, int paramSize) = 0;
	virtual int uninit() = 0;
	virtual void processFrame(int chId, unsigned char *data, struct v4l2_buffer capInfo, int format) = 0;
};

#define CORE_CHN_MAX	(4)
#define CORE_TGT_NUM_MAX	(32)
typedef struct{
	int valid;
	cv::Rect Box;
	cv::Point2f pos;
}CORE_TGT_INFO;
typedef struct _core_1001_chn_stats{
	cv::Size imgSize;
	int fovId;
	cv::Point2f axis;
	bool enableEnh;
	int iEZoomx;
	bool enableEncoder;
}CORE1001_CHN_STATS;

typedef struct _core_1001_stats{
	int mainChId;
	bool enableTrack;
	bool enableMMTD;
	bool enableMotionDetect;
	cv::Size acqWinSize;
	int iTrackorStat;
	cv::Point2f trackPos;
	cv::Size trackWinSize;
	CORE_TGT_INFO tgts[CORE_TGT_NUM_MAX];
	CORE1001_CHN_STATS chn[CORE_CHN_MAX];
	CORE_TGT_INFO blob;
}CORE1001_STATS;

typedef struct _core_1001_chnInfo_init{
	cv::Size imgSize;
	int format;
	int fps;
}CORE1001_CHN_INIT_PARAM;
typedef struct _core_1001_init{
	CORE1001_CHN_INIT_PARAM chnInfo[CORE_CHN_MAX];
	int nChannels;
	OSA_SemHndl *notify;
	bool bEncoder;
	bool bRender;
	bool bHideOSD;
	cv::Size renderSize;
	int renderFPS;
	int *encoderParamTab[3];
	char *encStreamIpaddr;
}CORE1001_INIT_PARAM;
typedef cv::Rect_<float> Rect2f;
class ICore_1001 : public ICore
{
public:
	virtual int setMainChId(int chId, int fovId, int ndrop, cv::Size acqSize) = 0;
	virtual int setSubChId(int chId) = 0;
	virtual int enableTrack(bool enable, cv::Size winSize, bool bFixSize = false) = 0;
	virtual int enableTrack(bool enable, Rect2f winRect, bool bFixSize = false) = 0;
	virtual int enableMMTD(bool enable, int nTarget) = 0;
	virtual int enableTrackByMMTD(int index, cv::Size *winSize = NULL, bool bFixSize = false) = 0;
	virtual int enableMotionDetect(bool enable) = 0;
	virtual int enableEnh(bool enable) = 0;
	virtual int enableEnh(int chId, bool enable) = 0;
	virtual int enableBlob(bool enable) = 0;
	virtual int bindBlend(int blendchId, cv::Matx44f matric) = 0;
	virtual int bindBlend(int chId, int blendchId, cv::Matx44f matric) = 0;
	virtual int enableOSD(bool enable) = 0;
	virtual int enableEncoder(int chId, bool enable) = 0;
	virtual int setAxisPos(cv::Point pos) = 0;
	virtual int saveAxisPos() = 0;
	virtual int setTrackPosRef(cv::Point2f ref) = 0;
	virtual int setTrackCoast(int nFrames) = 0;
	virtual int setEZoomx(int value) = 0;
	virtual int setEZoomx(int chId, int value) = 0;
	virtual int setOSDColor(int yuv, int thickness = 2) = 0;
	virtual int setOSD(cv::Scalar color, int thickness = 2) = 0;
	virtual int setEncTransLevel(int iLevel) = 0;

	CORE1001_STATS m_stats;
	cv::Mat m_dc[CORE_CHN_MAX];
};


#endif /* CRCORN_HPP_ */
