/*
 * gluWindow.cpp
 *
 *  Created on: Dec 17, 2018
 *      Author: wzk
 */

#include "gluWindow.hpp"
#include "osa_image_queue.h"
#include <cuda.h>
#include <cuda_gl_interop.h>
#include <cuda_runtime_api.h>
#include "cuda_mem.hpp"
#include <linux/videodev2.h>
#include <glew.h>
#include <glut.h>
#include <freeglut_ext.h>

//using namespace cv;

class CGLVideo
{
	__inline__ int GetChannels(int format)
	{
		return (format==V4L2_PIX_FMT_GREY) ? 1 : 3;
	}
	unsigned long nCnt;
	uint64  m_tmBak;
	GLuint m_pbo;
public:
	int m_idx;
	cv::Size m_size;
	int m_format;
	int m_channels;
	int m_fps;
	int m_nDrop;
	cr_osa::OSA_BufHndl m_bufQue;
	int m_memType;
	GLuint textureId;
	GLMatx44f m_matrix;

	CGLVideo(int idx, const cv::Size& imgSize, int format, int fps = 30, int nDrop = 0, int memType = memtype_glpbo)
	:m_idx(idx),m_nDrop(nDrop), m_size(imgSize),m_format(format),m_fps(fps),m_memType(memType),textureId(0),nCnt(0ul),m_pbo(-1)
	{
		memset(&m_bufQue, 0, sizeof(m_bufQue));
		//m_nDrop = m_disFPS/m_fps;
		int channels = GetChannels(format);
		int iRet = image_queue_create(&m_bufQue, 3, m_size.width*m_size.height*channels,m_memType);
		OSA_assert(iRet == OSA_SOK);
		glGenTextures(1, &textureId);
		assert(glIsTexture(textureId));
		glBindTexture(GL_TEXTURE_2D, textureId);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);//GL_NEAREST);//GL_NEAREST_MIPMAP_NEAREST);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);//GL_NEAREST);//GL_NEAREST_MIPMAP_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);//GL_CLAMP);//GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);//GL_CLAMP);//GL_CLAMP_TO_EDGE);

		if(channels == 1)
			glTexImage2D(GL_TEXTURE_2D, 0, channels, m_size.width, m_size.height, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
		else
			glTexImage2D(GL_TEXTURE_2D, 0, channels, m_size.width, m_size.height, 0, GL_BGR_EXT, GL_UNSIGNED_BYTE, NULL);

		if(memtype_glpbo != m_memType)
		{
			glGenBuffers(1, &m_pbo);
			glBindBuffer(GL_PIXEL_UNPACK_BUFFER, m_pbo);
			glBufferData(GL_PIXEL_UNPACK_BUFFER, m_size.width*m_size.height*channels, NULL, GL_DYNAMIC_COPY);//GL_STATIC_DRAW);//GL_DYNAMIC_DRAW);
			glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
		}
		m_channels = channels;
	}

	virtual ~CGLVideo()
	{
		image_queue_delete(&m_bufQue);
		glDeleteTextures(1, &textureId);
		if(m_pbo>0)
			glDeleteBuffers(1, &m_pbo);
	}

	virtual void update()
	{
		if(m_bufQue.bMap){
			for(int i=0; i<m_bufQue.numBuf; i++){
				cuMap(&m_bufQue.bufInfo[i]);
				image_queue_putEmpty(&m_bufQue, &m_bufQue.bufInfo[i]);
			}
			m_bufQue.bMap = false;
		}

		bool bDevMem = false;
		OSA_BufInfo* info = NULL;
		cv::Mat img;
		nCnt ++;
		if(nCnt > 300){
			int count = OSA_bufGetFullCount(&m_bufQue);
			nCnt = 1;
			if(count>1){
				OSA_printf("[%d]%s: ch%d queue count = %d, sync", OSA_getCurTimeInMsec(), __func__, m_idx, count);
				while(count>1){
					//image_queue_switchEmpty(&m_bufQue[chId]);
					info = image_queue_getFull(&m_bufQue);
					OSA_assert(info != NULL);
					image_queue_putEmpty(&m_bufQue, info);
					count = OSA_bufGetFullCount(&m_bufQue);
				}
			}
		}

		if(m_nDrop>1 && (nCnt%m_nDrop)!=1)
			return;

		info = image_queue_getFull(&m_bufQue);
		if(info != NULL)
		{
			GLuint pbo = 0;
			bDevMem = (info->memtype == memtype_cudev || info->memtype == memtype_cumap);
			void *data = (bDevMem) ? info->physAddr : info->virtAddr;
			if(info->channels == 1){
				img = cv::Mat(info->height, info->width, CV_8UC1, data);
			}else{
				img = cv::Mat(info->height, info->width, CV_8UC3, data);
			}
			m_tmBak = info->timestampCap;

			OSA_assert(img.cols > 0 && img.rows > 0);
			if(bDevMem)
			{
				unsigned int byteCount = img.cols*img.rows*img.channels();
				unsigned char *dev_pbo = NULL;
				size_t tmpSize;
				pbo = m_pbo;
				glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo);
				cudaResource_RegisterBuffer(m_idx, pbo, byteCount);
				cudaResource_mapBuffer(m_idx, (void **)&dev_pbo, &tmpSize);
				assert(tmpSize == byteCount);
				cudaMemcpy(dev_pbo, img.data, byteCount, cudaMemcpyDeviceToDevice);
				cudaDeviceSynchronize();
				cudaResource_unmapBuffer(m_idx);
				cudaResource_UnregisterBuffer(m_idx);
				img.data = NULL;
			}else if(info->memtype == memtype_glpbo){
				cuUnmap(info);
				pbo = info->pbo;
				glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo);
			}else{
				unsigned int byteCount = img.cols*img.rows*img.channels();
				unsigned char *dev_pbo = NULL;
				size_t tmpSize;
				pbo = m_pbo;
				OSA_assert(img.data != NULL);
				glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo);
				cudaResource_RegisterBuffer(m_idx, pbo, byteCount);
				cudaResource_mapBuffer(m_idx, (void **)&dev_pbo, &tmpSize);
				assert(tmpSize == byteCount);
				cudaMemcpy(dev_pbo, img.data, byteCount, cudaMemcpyHostToDevice);
				cudaDeviceSynchronize();
				cudaResource_unmapBuffer(m_idx);
				cudaResource_UnregisterBuffer(m_idx);
			}
			glBindTexture(GL_TEXTURE_2D, textureId);
			if(img.channels() == 1)
				glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, img.cols, img.rows, GL_RED, GL_UNSIGNED_BYTE, NULL);
			else //if(img.channels() == 3)
				glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, img.cols, img.rows, GL_BGR_EXT, GL_UNSIGNED_BYTE, NULL);
			glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
			if(info->memtype == memtype_glpbo){
				cuMap(info);
			}
			image_queue_putEmpty(&m_bufQue, info);
		}
	}
};

static GLint	glProgram[8];
#define ATTRIB_VERTEX 3
#define ATTRIB_TEXTURE 4
static const GLfloat defaultVertices[8] = {
    -1.f, -1.f,
    1.f, -1.f,
    -1.f, 1.f,
    1.f, 1.f
};
static const GLfloat defaultTextureCoords[8] = {
    0.0f, 1.0f,
    1.0f, 1.0f,
    0.0f, 0.0f,
    1.0f, 0.0f
};

class CGLVideoRender
{
protected:
	const CGLVideo* m_video;
	GLMatx44f m_matrix;
	cv::Rect m_viewPort;
	GLfloat m_vVerts[8];
	GLfloat m_vTexCoords[8];
	OSA_MutexHndl m_mutex;
public:
	CGLVideoRender(CGLVideo *video, const GLMatx44f& matrix, const cv::Rect& viewPort):m_video(video),m_matrix(matrix),m_viewPort(viewPort){
		memcpy(m_vVerts, defaultVertices, sizeof(defaultVertices));
		memcpy(m_vTexCoords, defaultTextureCoords, sizeof(defaultTextureCoords));
		OSA_mutexCreate(&m_mutex);
	}
	virtual ~CGLVideoRender(){
		OSA_mutexDelete(&m_mutex);
	}
	virtual void render()
	{
		OSA_mutexLock(&m_mutex);
		if(m_video == NULL){
			OSA_mutexUnlock(&m_mutex);
			return;
		}
		GLint glProg = glProgram[1];
		glUseProgram(glProg);
		GLint Uniform_tex_in = glGetUniformLocation(glProg, "tex_in");
		GLint Uniform_mvp = glGetUniformLocation(glProg, "mvpMatrix");
		GLMatx44f mTrans = m_video->m_matrix*m_matrix;
		glUniformMatrix4fv(Uniform_mvp, 1, GL_FALSE, mTrans.val);
		glUniform1i(Uniform_tex_in, 0);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, m_video->textureId);
		glVertexAttribPointer(ATTRIB_VERTEX, 2, GL_FLOAT, GL_FALSE, 0, m_vVerts);
		glVertexAttribPointer(ATTRIB_TEXTURE, 2, GL_FLOAT, GL_FALSE, 0, m_vTexCoords);
		glEnableVertexAttribArray(ATTRIB_VERTEX);
		glEnableVertexAttribArray(ATTRIB_TEXTURE);
		//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glViewport(m_viewPort.x, m_viewPort.y, m_viewPort.width, m_viewPort.height);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		glUseProgram(0);
		OSA_mutexUnlock(&m_mutex);
	}
	void set(const GLMatx44f& matrix)
	{
		OSA_mutexLock(&m_mutex);
		m_matrix = matrix;
		OSA_mutexUnlock(&m_mutex);
	}
	void set(const CGLVideo* video)
	{
		OSA_mutexLock(&m_mutex);
		m_video = video;
		OSA_mutexUnlock(&m_mutex);
	}
	void set(const cv::Rect& viewPort)
	{
		OSA_mutexLock(&m_mutex);
		m_viewPort = viewPort;
		OSA_mutexUnlock(&m_mutex);
	}
};

class CGLVideoBlendRender : public CGLVideoRender
{
protected:
	const CGLVideo* m_blend;
	GLMatx44f m_matrixBlend;
	DS_BlendPrm m_blendPrm;
public:
	CGLVideoBlendRender(CGLVideo *video, const GLMatx44f& matrix, const cv::Rect& viewPort, CGLVideo *blend, const GLMatx44f& blendMatrix, const DS_BlendPrm& prm)
		:CGLVideoRender(video, matrix, viewPort),m_blend(blend),m_matrixBlend(blendMatrix){ memcpy(&m_blendPrm, &prm, sizeof(m_blendPrm)); }
	virtual ~CGLVideoBlendRender(){}
	virtual void render()
	{
		CGLVideoRender::render();
		OSA_mutexLock(&m_mutex);
		if(m_video == NULL || m_blend == NULL){
			OSA_mutexUnlock(&m_mutex);
			return;
		}
		GLint glProg;
		GLMatx44f mTrans = m_matrixBlend*m_video->m_matrix*m_matrix;

		if(m_blend->m_channels == 1){
			glProg = glProgram[2];
		}else{
			glProg = glProgram[3];
		}
		glUseProgram(glProg);
		GLint Uniform_tex_in = glGetUniformLocation(glProg, "tex_in");
		GLint uniform_fAlpha = glGetUniformLocation(glProg, "fAlpha");
		GLint uniform_thr0Min = glGetUniformLocation(glProg, "thr0Min");
		GLint uniform_thr0Max = glGetUniformLocation(glProg, "thr0Max");
		GLint uniform_thr1Min = glGetUniformLocation(glProg, "thr1Min");
		GLint uniform_thr1Max = glGetUniformLocation(glProg, "thr1Max");
		GLint Uniform_mvp = glGetUniformLocation(glProg, "mvpMatrix");
		glUniformMatrix4fv(Uniform_mvp, 1, GL_FALSE, mTrans.val);
		glUniform1f(uniform_fAlpha, m_blendPrm.fAlpha);
		glUniform1f(uniform_thr0Min, m_blendPrm.thr0Min);
		glUniform1f(uniform_thr0Max, m_blendPrm.thr0Max);
		glUniform1f(uniform_thr1Min, m_blendPrm.thr1Min);
		glUniform1f(uniform_thr1Max, m_blendPrm.thr1Max);
		glUniform1i(Uniform_tex_in, 0);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, m_blend->textureId);

		glVertexAttribPointer(ATTRIB_VERTEX, 2, GL_FLOAT, GL_FALSE, 0, m_vVerts);
		glVertexAttribPointer(ATTRIB_TEXTURE, 2, GL_FLOAT, GL_FALSE, 0, m_vTexCoords);
		glEnableVertexAttribArray(ATTRIB_VERTEX);
		glEnableVertexAttribArray(ATTRIB_TEXTURE);
		glEnable(GL_MULTISAMPLE);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		glDisable(GL_MULTISAMPLE);
		glDisable(GL_BLEND);
		glUseProgram(0);
		OSA_mutexUnlock(&m_mutex);
	}
	void matrix(const GLMatx44f& matrix)
	{
		OSA_mutexLock(&m_mutex);
		m_matrixBlend = matrix;
		OSA_mutexUnlock(&m_mutex);
	}
	void blend(const CGLVideo* video)
	{
		OSA_mutexLock(&m_mutex);
		m_blend = video;
		OSA_mutexUnlock(&m_mutex);
	}
	void params(const DS_BlendPrm& prm)
	{
		OSA_mutexLock(&m_mutex);
		memcpy(&m_blendPrm, &prm, sizeof(m_blendPrm));
		OSA_mutexUnlock(&m_mutex);
	}
};

class CGLVideoMaskBlendRender : public CGLVideoRender
{
protected:
	const CGLVideo* m_blend;
	const CGLVideo* m_mask;
	GLMatx44f m_matrixBlend;
public:
	CGLVideoMaskBlendRender(CGLVideo *video, const GLMatx44f& matrix, const cv::Rect& viewPort, CGLVideo *blend, const GLMatx44f& blendMatrix, CGLVideo *mask)
		:CGLVideoRender(video, matrix, viewPort),m_blend(blend),m_matrixBlend(blendMatrix),m_mask(mask){}
	virtual ~CGLVideoMaskBlendRender(){}
	virtual void render()
	{
		CGLVideoRender::render();
		OSA_mutexLock(&m_mutex);
		if(m_video == NULL || m_blend == NULL || m_mask == NULL){
			OSA_mutexUnlock(&m_mutex);
			return;
		}
		GLint glProg;
		GLMatx44f mTrans = m_matrixBlend*m_video->m_matrix*m_matrix;

		glProg = glProgram[4];
		glUseProgram(glProg);
		GLint Uniform_tex_in = glGetUniformLocation(glProg, "tex_in");
		GLint Uniform_tex_mask = glGetUniformLocation(glProg, "tex_mask");
		GLint Uniform_mvp = glGetUniformLocation(glProg, "mvpMatrix");
		glUniformMatrix4fv(Uniform_mvp, 1, GL_FALSE, mTrans.val);
		glUniform1i(Uniform_tex_in, 0);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, m_blend->textureId);
		glUniform1i(Uniform_tex_mask, 1);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, m_mask->textureId);

		glVertexAttribPointer(ATTRIB_VERTEX, 2, GL_FLOAT, GL_FALSE, 0, m_vVerts);
		glVertexAttribPointer(ATTRIB_TEXTURE, 2, GL_FLOAT, GL_FALSE, 0, m_vTexCoords);
		glEnableVertexAttribArray(ATTRIB_VERTEX);
		glEnableVertexAttribArray(ATTRIB_TEXTURE);
		glEnable(GL_MULTISAMPLE);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		glDisable(GL_MULTISAMPLE);
		glDisable(GL_BLEND);
		glUseProgram(0);
		OSA_mutexUnlock(&m_mutex);
	}
	void matrix(const GLMatx44f& matrix)
	{
		OSA_mutexLock(&m_mutex);
		m_matrixBlend = matrix;
		OSA_mutexUnlock(&m_mutex);
	}
	void blend(const CGLVideo* video)
	{
		OSA_mutexLock(&m_mutex);
		m_blend = video;
		OSA_mutexUnlock(&m_mutex);
	}
	void mask(const CGLVideo* mask)
	{
		OSA_mutexLock(&m_mutex);
		m_mask = mask;
		OSA_mutexUnlock(&m_mutex);
	}
};

int CGluWindow::count = 0;
CGluWindow::CGluWindow(const cv::Rect& rc, int parent)
:m_rc(rc),m_parent(parent)
{
	m_index = count++;
}
CGluWindow::~CGluWindow()
{
	Destroy();
}

int CGluWindow::Create(bool FullScreen)
{
	if(m_parent>0){
		m_winId = glutCreateSubWindow(m_parent,m_rc.x,m_rc.y,m_rc.width,m_rc.height);
	}else{
		glutInitWindowPosition(m_rc.x, m_rc.y);
		glutInitWindowSize(m_rc.width, m_rc.height);
		glutSetOption(GLUT_RENDERING_CONTEXT,GLUT_USE_CURRENT_CONTEXT);
		char title[32];
		sprintf(title, "window%d", m_index);
		m_winId = glutCreateWindow(title);
	}
	OSA_assert(m_winId > 0);
	glutSetWindow(m_winId);

	GLenum err = glewInit();
	if (GLEW_OK != err) {
		fprintf(stderr, "\n[Render] %s %d: Error in glewInit. %s\n", __func__, __LINE__, glewGetErrorString(err));
		return -1;
	}

	glutSetWindow(m_winId);
	if(FullScreen){
		glutFullScreen();
	}
}

int CGluWindow::AddRender(const cv::Rect& rc)
{

}

void CGluWindow::RemoveRender(int index)
{

}

void CGluWindow::Display()
{

}

void CGluWindow::Destroy()
{

}
