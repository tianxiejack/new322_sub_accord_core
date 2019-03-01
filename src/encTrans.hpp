
#ifndef ENCTRANS_HPP_
#define ENCTRANS_HPP_

#include "osa.h"
#include <osa_sem.h>
#include <cuda.h>
#include "cuda_runtime_api.h"

#include "osa.h"
#include "osa_thr.h"
#include "osa_buf.h"
#include "osa_sem.h"
#include "linux/videodev2.h"
#include "gst_capture.h"

using namespace std;
using namespace cv;

#define ENT_CHN_MAX		(4)

typedef struct _enc2rtp_enc_param
{
	int fps;
	int bitrate;
	int minQP;
	int maxQP;
	int minQI;
	int maxQI;
	int minQB;
	int maxQB;
}ENC2RTP_encPrm;

typedef struct _enc2rtp_init_param
{
	bool bRtp;
	char destIpaddr[32];
	int iTransLevel;
	bool defaultEnable[ENT_CHN_MAX];
	CAPTURE_SRC srcType[ENT_CHN_MAX];
	ENC2RTP_encPrm encPrm[ENT_CHN_MAX];
	cv::Size imgSize[ENT_CHN_MAX];
	int inputFPS[ENT_CHN_MAX];
	int nChannels;
}ENC2RTP_InitPrm;

class CEnc2Rtp
{
public:
	CEnc2Rtp(){
		memset(m_record_handle, 0, sizeof(m_record_handle));
		memset(m_enable, 0, sizeof(m_enable));
		memset(m_semScheduler, 0, sizeof(m_semScheduler));
		memset(m_bufQue, 0, sizeof(m_bufQue));
		memset(m_bufSem, 0, sizeof(m_bufSem));
		m_nChannels = 2;
	};
	~CEnc2Rtp(){destroy();};
	int init(ENC2RTP_InitPrm *pPrm);
	typedef enum{
		CFG_Enable = 0,
		CFG_TransLevel,
		CFG_EncPrm,
		CFG_keyFrame,
		CFG_Max
	}CFG;

	int dynamic_config(CEnc2Rtp::CFG type, int iPrm, void* pPrm);
	void pushData(Mat frame, int chId, int pixFmt = V4L2_PIX_FMT_YUV420M);
	void scheduler(int chId);

public:
	ENC2RTP_InitPrm m_initPrm;
	int m_nChannels;
	bool m_enable[ENT_CHN_MAX];
	ENC2RTP_encPrm m_encPrm[ENT_CHN_MAX];
	int m_curBitrate[ENT_CHN_MAX];
	OSA_BufHndl *m_bufQue[ENT_CHN_MAX];
	OSA_SemHndl *m_bufSem[ENT_CHN_MAX];
	int m_curTransLevel;
	int m_curTransMask;

protected:
	OSA_MutexHndl m_mutex;
	OSA_SemHndl *m_semScheduler[ENT_CHN_MAX];
	GstCapture_data m_gstCapture_data[ENT_CHN_MAX];
	RecordHandle * m_record_handle[ENT_CHN_MAX];
	int createEncoder(int chId, CAPTURE_SRC srcType, char *ipAddr);
	int deleteEncoder(int chId);
	int destroy();
private:
};

#endif /* ENCTRANS_HPP_ */
