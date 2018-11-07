
#ifndef DISPLAYER_HPP_
#define DISPLAYER_HPP_

#define DS_RENDER_MAX		(9)
#define DS_CHAN_MAX         (4)

#include <cuda.h>
#include "cuda_runtime_api.h"

#include "osa.h"
#include "osa_thr.h"
#include "osa_buf.h"
#include "osa_sem.h"

#define DS_DC_CNT		(1)

using namespace std;
using namespace cv;
using namespace cr_osa;

typedef cv::Matx<GLfloat, 4, 4> GLMatx44f;

typedef struct _ds_size{
	int w;
	int h;
	int c;
}DS_Size;

typedef struct _ds_rect{
	int x;
	int y;
	int w;
	int h;
}DS_Rect;

typedef struct _ds_render
{
	int video_chId;
	DS_Rect displayrect;
	bool bCrop;
	DS_Rect croprect;
	GLMatx44f transform;
}DS_Render;

typedef struct _ds_blend_param{
	GLfloat fAlpha;
	GLfloat thr0Min;
	GLfloat thr0Max;
	GLfloat thr1Min;
	GLfloat thr1Max;
}DS_BlendPrm;

typedef struct _ds_init_param{
	bool bFullScreen;
	int winPosX;
	int winPosY;
	int winWidth;
	int winHeight;
	int disFPS;
	bool bScript;
	char szScriptFile[256];
	DS_Size channelsSize[DS_CHAN_MAX];
	int nChannels;
	int nQueueSize;
	int memType;
	//void (*displayfunc)(void);
	void (*mousefunc)(int button, int state, int x, int y);
	//void (*reshapefunc)(int width, int height);
	void (*keyboardfunc)(unsigned char key, int x, int y);
	void (*keySpecialfunc)( int, int, int );
	void (*visibilityfunc)(int state);
	void (*timerfunc)(int value);
	void (*idlefunc)(void);
	void (*closefunc)(void);
	void (*renderfunc)(void);
	int timerfunc_value;//context
}DS_InitPrm;

class CRender
{
	int tag;
	CRender();
	~CRender();
public:
	static CRender* createObject();
	static void destroyObject(CRender* obj);
	int create();
	int destroy();
	int init(DS_InitPrm *pPrm);
	void run();
	void stop();

	typedef enum{
		DS_CFG_ChId = 0,
		DS_CFG_BlendChId,
		DS_CFG_MaskChId,
		DS_CFG_CropEnable,
		DS_CFG_CropRect,
		DS_CFG_VideoTransMat,
		DS_CFG_ViewTransMat,
		DS_CFG_BlendTransMat,
		DS_CFG_BlendPrm,
		DS_CFG_Max
	}DS_CFG;

	int dynamic_config(DS_CFG type, int iPrm, void* pPrm);
	int get_videoSize(int chId, DS_Size &size);
	GLuint async_display(int chId, int width, int height, int channels);
	int setFullScreen(bool bFull);
	void disp_fps();
	int m_mainWinWidth;
	int m_mainWinHeight;
	bool m_bRun;
	bool m_bFullScreen;
	bool m_bOsd;
	Mat m_imgOsd[DS_DC_CNT];
	CvScalar m_osdColor;
	DS_Size m_videoSize[DS_CHAN_MAX];
	GLuint buffId_input[DS_CHAN_MAX];
	GLuint buffId_osd[DS_DC_CNT];
	OSA_BufHndl m_bufQue[DS_CHAN_MAX];
	OSA_MutexHndl *m_cumutex;
	bool m_timerRun;
protected:
	DS_InitPrm m_initPrm;
	DS_Render m_renders[DS_RENDER_MAX];
	int m_curMap[DS_RENDER_MAX];
	int m_blendMap[DS_CHAN_MAX];
	int m_maskMap[DS_CHAN_MAX];
	int m_renderCount;
	int initRender(bool updateMap = true);
	void uninitRender();

protected:
	static void _display(void);
	static void _timeFunc(int value);
	static void _reshape(int width, int height);
	static void _close(void);
	void gl_resize(void);

protected:
	GLint	m_glProgram[8];
	GLfloat m_glvVerts[DS_RENDER_MAX][8];
	GLfloat m_glvTexCoords[DS_RENDER_MAX][8];
	bool m_bUpdateVertex;
	GLMatx44f m_glmat44fTrans[DS_CHAN_MAX];
	GLMatx44f m_glmat44fBlend[DS_CHAN_MAX*DS_CHAN_MAX];
	DS_BlendPrm m_glBlendPrm[DS_CHAN_MAX*DS_CHAN_MAX];
	GLuint textureId_input[DS_CHAN_MAX];
	GLuint textureId_osd[DS_DC_CNT];

	int gl_create();
	void gl_destroy();
	void gl_init();
	void gl_uninit();
	void gl_display();
	void gl_updateTexOSD();
	void gl_updateTexVideo();
	int gl_updateVertex();
	int gl_loadProgram();
	int gl_unloadProgram();
	bool gltLoadShaderSrc(const char *szShaderSrc, GLuint shader);
	bool gltLoadShaderFile(const char *szFile, GLuint shader);
	GLuint gltLoadShaderPairWithAttributes(const char *szVertexProg, const char *szFragmentProg, ...);


private:
	OSA_MutexHndl m_mutex;
	uint64  m_interval;
	double m_telapse;
	uint64  m_tmBak[DS_CHAN_MAX];
	int64   m_tmRender;
	bool m_waitSync;
};

#endif /* DISPLAYER_HPP_ */
