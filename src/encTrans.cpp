/*
 * main_osd.cpp
 *
 *  Created on: 2018年8月28日
 *      Author: fsmdn121
 */
/*
 * main.cpp
 *
 *  Created on: 2018年8月23日
 *      Author: fsmdn121
 */

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv/cv.hpp>
#include <opencv2/opencv.hpp>

#include "osa.h"
#include "gst_capture.h"
#include "encTrans.hpp"

//void processFrame_osd(int cap_chid,unsigned char *src)
void CEnc2Rtp::pushData(Mat img, int chId, int pixFmt)
{
	OSA_assert(pixFmt == V4L2_PIX_FMT_YUV420M);
	//return CRender::display(img, chId, code);
	//OSA_printf("[CEncTrans]%s %d: ch%d enable%d", __func__, __LINE__, chId, m_enable[chId]);
	if(m_enable[chId]){
		gstCapturePushData(m_record_handle[chId],(char *)img.data,img.cols*img.rows*img.channels());
	}
	return ;
}

void CEnc2Rtp::scheduler(int chId)
{
	if(m_semScheduler[chId] != NULL)
		OSA_semSignal(m_semScheduler[chId]);
}

static char TARGET_IP[32] = "127.0.0.1";
static int PORT_RTP = 7000;
static char strFormat[16] = "I420";//"YUY2"//"GRAY8"

int CEnc2Rtp::createEncoder(int chId, CAPTURE_SRC srcType, char *ipAddr)
{
	memset(&m_gstCapture_data[chId], 0, sizeof(GstCapture_data));
	m_gstCapture_data[chId].width = m_initPrm.imgSize[chId].width;
	m_gstCapture_data[chId].height = m_initPrm.imgSize[chId].height;
	m_gstCapture_data[chId].framerate = m_initPrm.encPrm[chId].fps;
	m_gstCapture_data[chId].bitrate = m_initPrm.encPrm[chId].bitrate;
	m_gstCapture_data[chId].ip_port = PORT_RTP+chId;
	m_gstCapture_data[chId].filp_method = FLIP_METHOD_VERTICAL_FLIP;
	m_gstCapture_data[chId].capture_src = srcType;
	//m_gstCapture_data[chId].format = "YUY2";
	m_gstCapture_data[chId].format = strFormat;//"I420";
	m_gstCapture_data[chId].ip_addr =ipAddr;
	m_gstCapture_data[chId].sd_cb=NULL;//sync422_ontime_video;
	for(int i=0;i<ENC_QP_PARAMS_COUNT;i++)
		m_gstCapture_data[chId].Q_PIB[i]=-1;
	m_gstCapture_data[chId].notify = m_semScheduler[chId];

	m_record_handle[chId] = gstCaptureInit(m_gstCapture_data[chId]);

	return (m_record_handle[chId] == NULL) ? -1 : 0;
}

int CEnc2Rtp::deleteEncoder(int chId)
{
	return gstCaptureUninit(m_record_handle[chId]);
}

int CEnc2Rtp::init(ENC2RTP_InitPrm *pPrm)
{
	int ret = 0;

	OSA_mutexCreate(&m_mutex);

	OSA_assert(pPrm != NULL);
	memcpy(&m_initPrm, pPrm, sizeof(m_initPrm));
	memcpy(m_enable, m_initPrm.defaultEnable, sizeof(m_enable));
	memcpy(m_encPrm, m_initPrm.encPrm, sizeof(m_encPrm));
	if(m_initPrm.nChannels<=0 || m_initPrm.nChannels>ENT_CHN_MAX)
		m_initPrm.nChannels = 2;
	m_nChannels = m_initPrm.nChannels;

	for(int chId=0; chId<m_nChannels; chId++){
		if(m_initPrm.encPrm[chId].fps<=0)
			m_initPrm.encPrm[chId].fps = 30;
	}

	if(m_initPrm.bRtp){
		for(int chId=0; chId<m_nChannels; chId++){
			createEncoder(chId, m_initPrm.srcType[chId], m_initPrm.destIpaddr);
			m_bufQue[chId] = m_record_handle[chId]->pushQueue;
			m_bufSem[chId] = m_record_handle[chId]->pushSem;
		}
	}
	else
	{

	}

	OSA_mutexLock(&m_mutex);
	for(int i=0; i<m_nChannels; i++){
		m_curBitrate[i]=ChangeBitRate(m_record_handle[i], m_encPrm[i].bitrate);
		ChangeQP_range(m_record_handle[i],
				m_encPrm[i].minQP, m_encPrm[i].maxQP,
				m_encPrm[i].minQI, m_encPrm[i].maxQI,
				m_encPrm[i].minQB, m_encPrm[i].maxQB);
	}
	OSA_mutexUnlock(&m_mutex);

	return ret;
}

int CEnc2Rtp::destroy()
{
	int ret = 0;
	OSA_mutexLock(&m_mutex);
	for(int i=0; i<m_nChannels; i++){
		deleteEncoder(i);
		if(m_semScheduler[i] == NULL)
			continue;
		OSA_semSignal(m_semScheduler[i]);
		OSA_semDelete(m_semScheduler[i]);
		delete m_semScheduler[i];
		m_semScheduler[i] = NULL;
	}
	OSA_mutexDelete(&m_mutex);

	return ret;
}

#define BITRATE(r) ((r==0) ? "2M" : ((r==1) ? "4M": "8M"))
int CEnc2Rtp::dynamic_config(CEnc2Rtp::CFG type, int iPrm, void* pPrm)
{
	int ret = OSA_SOK;
	bool bEnable;
	if(type == CFG_Enable){
		if(iPrm >= m_nChannels || iPrm < 0)
			return -1;
		if(pPrm == NULL)
			return -2;
		bEnable = *(bool*)pPrm;
		m_enable[iPrm] = bEnable;
	}
	if(type == CFG_EncPrm){
		if(iPrm >= m_nChannels || iPrm < 0)
			return -1;
		if(pPrm == NULL)
			return -2;
		memcpy(&m_encPrm[iPrm], pPrm, sizeof(ENC2RTP_encPrm));
		//OSA_mutexLock(&m_mutex);
		m_curBitrate[iPrm]=ChangeBitRate(m_record_handle[iPrm], m_encPrm[iPrm].bitrate);
		ChangeQP_range(m_record_handle[iPrm],
				m_encPrm[iPrm].minQP, m_encPrm[iPrm].maxQP,
				m_encPrm[iPrm].minQI, m_encPrm[iPrm].maxQI,
				m_encPrm[iPrm].minQB, m_encPrm[iPrm].maxQB);
		OSA_printf("%s %i: CH%d %dbps QP %d-%d QI %d-%d QB %d-%d", __func__, __LINE__,
				iPrm,m_encPrm[iPrm].bitrate,
				m_encPrm[iPrm].minQP, m_encPrm[iPrm].maxQP,
				m_encPrm[iPrm].minQI, m_encPrm[iPrm].maxQI,
				m_encPrm[iPrm].minQB, m_encPrm[iPrm].maxQB);
		//ret = sync422_ontime_ctrl(ctrl_prm_uartrate, 0, SPEED(m_encPrm[iPrm].bitrate));
		//OSA_mutexUnlock(&m_mutex);
		OSA_assert(ret == OSA_SOK);
	}

	if(type == CFG_keyFrame){
		if(iPrm >= m_nChannels || iPrm < 0)
			return -1;
		if(pPrm == NULL)
			return -2;
		m_curBitrate[iPrm]=ChangeBitRate(m_record_handle[iPrm], m_curBitrate[iPrm]);
	}
	return 0;
}
#undef BITRATE

