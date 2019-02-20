/*
 * crCore.hpp
 *
 *  Created on: Sep 27, 2018
 *      Author: wzk
 */

#ifndef CRCORE_HPP_
#define CRCORE_HPP_

/***********************************************
 * core version 1.0.9
 *
 * 20/12/2018 motify: CORE1001_INIT_PARAM::renderHook
 *	enum{	 RUN_ENTER = 0,RUN_WIN,	RUN_SWAP,	RUN_LEAVE };
 * 11/01/2019 motify: int ICore_1001::setHideSysOsd(bool bHideOSD)
 * 20/02/2019 motify: cv::Rect renderRC;
 */
#include "osa.h"
#include "osa_sem.h"

#define CORE_1001_VERSION_  "1.0.9"
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
	int index;
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
	uint64_t frameTimestamp;
	int blendBindId;
	cv::Matx44f blendMatric;
}CORE1001_CHN_STATS;

typedef struct _core_1001_stats{
	int mainChId;
	int subChId;
	bool enableTrack;
	bool enableMMTD;
	bool enableMotionDetect;
	bool enableBlob;
	bool enableOSD;
	cv::Size acqWinSize;
	int iTrackorStat;
	cv::Point2f trackPos;
	cv::Size trackWinSize;
	CORE_TGT_INFO tgts[CORE_TGT_NUM_MAX];
	CORE1001_CHN_STATS chn[CORE_CHN_MAX];
	CORE_TGT_INFO blob;
	unsigned int lossCoastFrames;
	unsigned int lossCoastTelapse;//ms
	cv::Rect subRc;
	cv::Matx44f subMatric;
	int colorYUV;
	int transLevel;
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
	void (*renderHook)(int displayId, int stepIdx, int stepSub, int context);
	bool bEncoder;
	bool bRender;
	bool bHideOSD;
	cv::Rect renderRC;
	int renderFPS;
	float renderSched;
	char *encStreamIpaddr;
	int *encoderParamTab[3];
	int *encoderParamTabMulti[CORE_CHN_MAX][3];
}CORE1001_INIT_PARAM;
enum{	 RENDER_HOOK_RUN_ENTER = 0, RENDER_HOOK_RUN_WIN,	RENDER_HOOK_RUN_SWAP,	RENDER_HOOK_RUN_LEAVE };
typedef cv::Rect_<float> Rect2f;
class ICore_1001 : public ICore
{
public:
	virtual int setMainChId(int chId, int fovId, int ndrop, cv::Size acqSize) = 0;
	virtual int setSubChId(int chId) = 0;
	virtual int enableTrack(bool enable, cv::Size winSize, bool bFixSize = false) = 0;
	virtual int enableTrack(bool enable, Rect2f winRect, bool bFixSize = false) = 0;
	virtual int enableMMTD(bool enable, int nTarget, int nSel = 0) = 0;
	virtual int enableTrackByMMTD(int index, cv::Size *winSize = NULL, bool bFixSize = false) = 0;
	virtual int enableMotionDetect(bool enable) = 0;
	virtual int enableEnh(bool enable) = 0;
	virtual int enableEnh(int chId, bool enable) = 0;
	virtual int enableBlob(bool enable) = 0;
	virtual int bindBlend(int blendchId, const cv::Matx44f& matric) = 0;
	virtual int bindBlend(int chId, int blendchId, const cv::Matx44f& matric) = 0;
	virtual int enableOSD(bool enable) = 0;
	virtual int enableEncoder(int chId, bool enable) = 0;
	virtual int setAxisPos(cv::Point pos) = 0;
	virtual int saveAxisPos() = 0;
	virtual int setTrackPosRef(cv::Point2f ref) = 0;
	virtual int setTrackCoast(int nFrames) = 0;
	virtual int setEZoomx(int value) = 0;
	virtual int setEZoomx(int chId, int value) = 0;
	virtual int setWinPos(int winId, const cv::Rect& rc) = 0;//! (0,0):(left,bottom)
	virtual int setWinMatric(int winId, const cv::Matx44f& matric) = 0;
	virtual int setOSDColor(int yuv, int thickness = 2) = 0;
	virtual int setOSD(cv::Scalar color, int thickness = 2) = 0;
	virtual int setEncTransLevel(int iLevel) = 0;
	virtual int setHideSysOsd(bool bHideOSD) = 0;

	CORE1001_STATS m_stats;
	cv::Mat m_dc[CORE_CHN_MAX];
};


#endif /* CRCORN_HPP_ */
