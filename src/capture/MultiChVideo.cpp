/*
 * MultiChVideo.cpp
 *
 *  Created on:
 *      Author: sh
 */

#include <pthread.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include "MultiChVideo.hpp"
//#include "gpio_rdwr.h"

MultiChVideo::MultiChVideo():m_nCap(0),m_usrFunc(NULL),m_user(NULL)
{
	memset(m_thrCap, 0, sizeof(m_thrCap));
	memset(VCap, 0, MultiV_MAX_CHAN*sizeof(void*));
	memset(m_bufQueue, 0, sizeof(m_bufQueue));
}

MultiChVideo::~MultiChVideo()
{
	destroy();
}


int MultiChVideo::creat()
{
	int chId;
	int ret = OSA_SOK;
	VCap[m_nCap] = new v4l2_camera(0);
	VCap[m_nCap++]->creat();
	VCap[m_nCap] = new v4l2_camera(1);
	VCap[m_nCap++]->creat();
	//VCap[m_nCap] = new v4l2_camera(2);
	//VCap[m_nCap++]->creat();
	//VCap[m_nCap] = new v4l2_camera(3);
	//VCap[m_nCap++]->creat();

	for(chId=0; chId<m_nCap; chId++){
		m_bufQueue[chId] = new OSA_BufHndl;
		OSA_assert(m_bufQueue[chId] != NULL);
		ret = image_queue_create(m_bufQueue[chId], BUFFER_CNT_PER_CHAN, 0, memtype_null);
		OSA_assert(ret == OSA_SOK);
		for(int i=0; i<m_bufQueue[chId]->numBuf; i++)
		{
			OSA_BufInfo* bufInfo = &m_bufQueue[chId]->bufInfo[i];
			struct v4l2_buffer *vbuf = new struct v4l2_buffer;
			OSA_assert(vbuf != NULL);
			memset(vbuf, 0, sizeof(vbuf));
			bufInfo->resource = vbuf;
		}
	}

	return 0;
}

int MultiChVideo::destroy()
{
	for(int chId=0; chId<m_nCap; chId++){
		if(VCap[chId] != NULL)
		{
			stop();
			VCap[chId]->stop();
			delete VCap[chId];
			VCap[chId] = NULL;

			if(m_bufQueue[chId] != NULL){
				for(int i=0; i<m_bufQueue[chId]->numBuf; i++){
					struct v4l2_buffer *vbuf = (struct v4l2_buffer *)m_bufQueue[chId]->bufInfo[i].resource;
					if(vbuf != NULL)
						delete vbuf;
					m_bufQueue[chId]->bufInfo[i].resource = NULL;
				}
				image_queue_delete(m_bufQueue[chId]);
				delete m_bufQueue[chId];
				m_bufQueue[chId] = NULL;
			}
		}
	}
	m_nCap = 0;
	return 0;
}

int MultiChVideo::run()
{
	int iRet = 0;

	for(int i=0; i<m_nCap; i++){
		VCap[i]->run();
		m_thrCxt[i].pUser = this;
		m_thrCxt[i].chId = i;
		iRet = OSA_thrCreate(&m_thrCap[i], capThreadFunc, 0, 0, &m_thrCxt[i]);
	}

	return iRet;
}

int MultiChVideo::stop()
{
	for(int i=0; i<m_nCap; i++){
		VCap[i]->stop();
		OSA_thrDelete(&m_thrCap[i]);
	}

	return 0;
}

int MultiChVideo::run(int chId)
{
	int iRet;
	if(chId<0 || chId>=m_nCap)
		return -1;

	VCap[chId]->run();
	m_thrCxt[chId].pUser = this;
	m_thrCxt[chId].chId = chId;
	iRet = OSA_thrCreate(&m_thrCap[chId], capThreadFunc, 0, 0, &m_thrCxt[chId]);

	return iRet;
}

int MultiChVideo::stop(int chId)
{
	if(chId<0 || chId>=m_nCap)
		return -1;

	OSA_thrDelete(&m_thrCap[chId]);
	VCap[chId]->stop();

	return 0;
}
/*
void MultiChVideo::process()
{
	int chId;
	fd_set fds;
	struct timeval tv;
	int ret;

	Mat frame;

	FD_ZERO(&fds);

	for(chId=0; chId<MAX_CHAN; chId++){
		if(VCap[chId]->m_devFd != -1 &&VCap[chId]->bRun )
			FD_SET(VCap[chId]->m_devFd, &fds);
		//OSA_printf("MultiChVideo::process: FD_SET ch%d -- fd %d", chId, VCap[chId]->m_devFd);
	}

	tv.tv_sec = 2;
	tv.tv_usec = 0;

	ret = select(FD_SETSIZE, &fds, NULL, NULL, &tv);
	//OSA_printf("MultiChVideo::process: ...");

	if(-1 == ret)
	{
		if (EINTR == errno)
			return;
	}else if(0 == ret)
	{
		return;
	}

	for(chId=0; chId<MAX_CHAN; chId++){
		if(VCap[chId]->m_devFd != -1 && FD_ISSET(VCap[chId]->m_devFd, &fds)){
			struct v4l2_buffer buf;
			memset(&buf, 0, sizeof(buf));
			buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			buf.memory = V4L2_MEMORY_USERPTR;//V4L2_MEMORY_MMAP;
			struct v4l2_buffer *vbuf = &buf;

			OSA_BufInfo* bufInfo = image_queue_getEmpty(m_bufQueue[chId]);

			if (-1 == v4l2_camera::xioctl(VCap[chId]->m_devFd, VIDIOC_DQBUF, vbuf))
			{
				//fprintf(stderr, "cap ch%d DQBUF Error!\n", chId);
			}
			else
			{
				int flag = 0;
				if(m_usrFunc != NULL){
					frame = cv::Mat(VCap[chId]->imgheight, VCap[chId]->imgwidth, VCap[chId]->imgtype,
							VCap[chId]->buffers[vbuf->index].start);

					flag = m_usrFunc(m_user, chId, frame, *vbuf, VCap[chId]->imgformat);
				}
				if(flag == 0){
					if (-1 == v4l2_camera::xioctl(VCap[chId]->m_devFd, VIDIOC_QBUF, vbuf)){
						fprintf(stderr, "VIDIOC_QBUF error %d, %s\n", errno, strerror(errno));
						exit(EXIT_FAILURE);
					}
				}
			}
		}
	}
}*/

static int videoflag = 0;
void MultiChVideo::process(int chId)
{
	fd_set fds;
	struct timeval tv;
	int ret;

	Mat frame;

	FD_ZERO(&fds);

	FD_SET(VCap[chId]->m_devFd, &fds);

	tv.tv_sec = 1;
	tv.tv_usec = 0;

	ret = select(VCap[chId]->m_devFd+1, &fds, NULL, NULL, &tv);

	if(-1 == ret)
	{
		if (EINTR == errno)
			return;
	}else if(0 == ret)
	{
		return;
	}

	if(VCap[chId]->m_devFd != -1 && FD_ISSET(VCap[chId]->m_devFd, &fds))
	{
		struct v4l2_buffer buf;
		memset(&buf, 0, sizeof(buf));
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_USERPTR;//V4L2_MEMORY_MMAP
		struct v4l2_buffer *vbuf = &buf;

		if (v4l2_camera::xioctl(VCap[chId]->m_devFd, VIDIOC_DQBUF, vbuf)<0)
		{
			return ;
		}

		OSA_BufInfo* bufInfo = image_queue_getEmpty(m_bufQueue[chId]);
		if(bufInfo != NULL){
			memcpy(bufInfo->resource, vbuf, sizeof(struct v4l2_buffer));
			bufInfo->chId = chId;
			bufInfo->width = VCap[chId]->imgwidth;
			bufInfo->height = VCap[chId]->imgheight;
			bufInfo->format = VCap[chId]->imgformat;
			bufInfo->channels = CV_MAT_CN(VCap[chId]->imgtype);
			//OSA_assert(bufInfo->channels == 2);
			bufInfo->flags = (int)vbuf->flags;
			bufInfo->timestampCap = (uint64)vbuf->timestamp.tv_sec*1000000000ul
					+ (uint64)vbuf->timestamp.tv_usec*1000ul;
			bufInfo->timestamp = (uint64_t)getTickCount();
			bufInfo->virtAddr = VCap[chId]->buffers[vbuf->index].start;
			image_queue_putFull(m_bufQueue[chId], bufInfo);
		}else{
			//OSA_assert(0);
		}

		int flag = 0;
		if(m_usrFunc != NULL){
			frame = cv::Mat(VCap[chId]->imgheight, VCap[chId]->imgwidth, VCap[chId]->imgtype,
					VCap[chId]->buffers[vbuf->index].start);

			flag = m_usrFunc(m_user, chId, frame, *vbuf, VCap[chId]->imgformat);
		}

		if(flag == 0){
			if (-1 == v4l2_camera::xioctl(VCap[chId]->m_devFd, VIDIOC_QBUF, vbuf)){
				fprintf(stderr, "VIDIOC_QBUF error %d, %s\n", errno, strerror(errno));
				exit(EXIT_FAILURE);
			}
			if(bufInfo != NULL){
				image_queue_switchEmpty(m_bufQueue[chId]);
			}
		}
	}
}
