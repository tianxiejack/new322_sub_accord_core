/*
 * v4l2camera.cpp
 *
 *  Created on: 2017年3月17日
 *      Author: sh
 */

#include "v4l2camera.hpp"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <getopt.h>             /* getopt_long() */
#include <fcntl.h>              /* low-level i/o */
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
//#include "thread.h"
#include <osa_buf.h>
#include <osa.h>
#include "arm_neon.h"

#include <cuda.h>
#include <cuda_runtime.h>
#include <malloc.h>

#define INPUT_IMAGE_HEIGHT		(1080)	//)(576)
#define INPUT_IMAGE_WIDTH		(1920)	//(720)
#define BUFFER_NUM_MAX			(CAP_CH_NUM*4)


v4l2_camera::v4l2_camera(int devId):io(IO_METHOD_USERPTR/*IO_METHOD_MMAP*/),imgwidth(IMAGE_WIDTH),imgstride(IMAGE_WIDTH),
imgheight(IMAGE_HEIGHT),imgformat(V4L2_PIX_FMT_UYVY),buffers(NULL),memType(MEMORY_NORMAL),bufferCount(BUFFER_CNT_PER_CHAN),
force_format(1),m_devFd(-1),n_buffers(0),bufSize(INPUT_IMAGE_WIDTH*INPUT_IMAGE_HEIGHT),bRun(false),
Id(/*devId*/0)
{
	sprintf(dev_name, "/dev/video%d",devId);
	// for axgs021
	switch(devId){
		case 0:
			imgformat 	= V4L2_PIX_FMT_YUYV;
			imgwidth  	= INPUT_IMAGE_WIDTH;
			imgheight 	= INPUT_IMAGE_HEIGHT;
			imgstride 	= imgwidth*2;
			bufSize 	= imgwidth * imgheight * 2;
			imgtype     = CV_8UC2;
			memType = MEMORY_LOCKED;
			bufferCount = 6;
			break;
		case 1:
			imgformat 	= V4L2_PIX_FMT_YUYV;
			imgwidth  	= INPUT_IMAGE_WIDTH;
			imgheight 	= INPUT_IMAGE_HEIGHT;
			imgstride 	= imgwidth*2;
			bufSize 	= imgwidth * imgheight * 2;
			imgtype     = CV_8UC2;
			memType = MEMORY_LOCKED;
			bufferCount = 6;
			break;
		case 2:
			imgformat 	= V4L2_PIX_FMT_YUYV;
			imgwidth  	= INPUT_IMAGE_WIDTH;
			imgheight 	= INPUT_IMAGE_HEIGHT;
			imgstride 	= imgwidth * 2;
			bufSize 	= imgwidth * imgheight * 2;
			imgtype     = CV_8UC2;
			memType = MEMORY_LOCKED;
			bufferCount = 6;
			break;
		case 3:
			imgformat 	= V4L2_PIX_FMT_GREY;
			imgwidth  	= INPUT_IMAGE_WIDTH;
			imgheight 	= INPUT_IMAGE_HEIGHT;
			imgstride 	= imgwidth;
			bufSize 	= imgwidth * imgheight;
			imgtype     = CV_8UC1;
			memType = MEMORY_LOCKED;
			bufferCount = 6;
			break;
		case 4:
			imgformat 	= V4L2_PIX_FMT_GREY;
			imgwidth  	= INPUT_IMAGE_WIDTH;
			imgheight 	= INPUT_IMAGE_HEIGHT;
			imgstride 	= imgwidth;
			bufSize 	= imgwidth * imgheight;
			imgtype     = CV_8UC1;
			memType = MEMORY_LOCKED;
			bufferCount = 6;
			break;
		default:
			printf("No such device:%s !!\n", dev_name);
	}
}

v4l2_camera::~v4l2_camera()
{
	destroy();
}

void v4l2_camera::errno_exit(const char *s)
{
	fprintf(stderr, "%s error %d, %s\n", s, errno, strerror(errno));
	exit(EXIT_FAILURE);
}

int v4l2_camera::xioctl(int fh, int request, void *arg)
{
	int ret;

	do {
		ret = ioctl(fh, request, arg);
	} while (-1 == ret && EINTR == errno);

	return ret;
}

int v4l2_camera::read_frame(unsigned char **data)
{
	struct v4l2_buffer buf;
	int i=0;

	switch (io) {
//	case IO_METHOD_READ:
//		if (-1 == read(fd, buffers[0].start, buffers[0].length)) {
//			switch (errno) {
//			case EAGAIN:
//				return 0;
//			case EIO:
//				/* Could ignore EIO, see spec. */
//				/* fall through */
//			default:
//				errno_exit("read");
//			}
//		}
//		process_image(buffers[0].start, buffers[0].length);
//		break;
	case IO_METHOD_MMAP:
		CLEAR(buf);
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;

		if (-1 == xioctl(m_devFd, VIDIOC_DQBUF, &buf)) {
			switch (errno) {
			case EAGAIN:
				return -1;
			case EIO:
				/* Could ignore EIO, see spec. */
				/* fall through */
			default:
				errno_exit("VIDIOC_DQBUF");
			}
		}
//		printf("fun:[%s]  line:[%d] buf.index=%d n_buffers=%d\n",__func__,__LINE__, buf.index, n_buffers);
		assert(buf.index < n_buffers);
		//process_image(buffers[buf.index].start, buf.bytesused);

		if (-1 == xioctl(m_devFd, VIDIOC_QBUF, &buf))
			errno_exit("VIDIOC_QBUF");
		break;
	case IO_METHOD_USERPTR:
		CLEAR(buf);

		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_USERPTR;

		if (-1 == xioctl(m_devFd, VIDIOC_DQBUF, &buf)) {
			switch (errno) {
			case EAGAIN:
				return 0;
			case EIO:
//				/* Could ignore EIO, see spec. */
//				/* fall through */
			default:
				errno_exit("VIDIOC_DQBUF");
			}
		}

		for (i=0; i<n_buffers; ++i)
			if (buf.m.userptr == (unsigned long)buffers[i].start
					&& buf.length == buffers[i].length)
				break;

		assert(i < n_buffers);

//		process_image((void *)buf.m.userptr, buf.bytesused);
//
//		if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
//			errno_exit("VIDIOC_QBUF");
//		break;
	default:
		break;
	}

	return 0;
}

void v4l2_camera::stop_capturing(void)
{
	enum v4l2_buf_type type;
	switch (io) {
//	case IO_METHOD_READ:
//		/* Nothing to do. */
//		break;
	case IO_METHOD_MMAP:
	case IO_METHOD_USERPTR:
		type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		if (-1 == xioctl(m_devFd, VIDIOC_STREAMOFF, &type))
			errno_exit("VIDIOC_STREAMOFF");
		break;
	}
}

void v4l2_camera::start_capturing(void)
{
	unsigned int i;
	enum v4l2_buf_type type;

	switch (io) {
//	case IO_METHOD_READ:
//		/* Nothing to do. */
//		break;
	case IO_METHOD_MMAP:
		for (i = 0; i < n_buffers; ++i) {
			struct v4l2_buffer buf;

			CLEAR(buf);
			buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			buf.memory = V4L2_MEMORY_MMAP;
			buf.index = i;

			if (-1 == xioctl(m_devFd, VIDIOC_QBUF, &buf))
				errno_exit("VIDIOC_QBUF");
		}
		type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		if (-1 == xioctl(m_devFd, VIDIOC_STREAMON, &type))
			errno_exit("VIDIOC_STREAMON");
		break;
	case IO_METHOD_USERPTR:
		for (i = 0; i < n_buffers; ++i) {
			struct v4l2_buffer buf;

			CLEAR(buf);
			buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			buf.memory = V4L2_MEMORY_USERPTR;
			buf.index = i;
			buf.m.userptr = (unsigned long)buffers[i].start;
			buf.length = buffers[i].length;

			if (-1 == xioctl(m_devFd, VIDIOC_QBUF, &buf))
				errno_exit("VIDIOC_QBUF");
		}
		type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		if (-1 == xioctl(m_devFd, VIDIOC_STREAMON, &type))
			errno_exit("VIDIOC_STREAMON");
		break;
	default:
		break;
	}
}

void v4l2_camera::uninit_device(void)
{
	unsigned int i;

	switch (io) {
	case IO_METHOD_READ:
		free(buffers[0].start);
		break;
	case IO_METHOD_MMAP:
		for (i = 0; i < n_buffers; ++i)
			if (-1 == munmap(buffers[i].start, buffers[i].length))
				errno_exit("munmap");
		break;
	case IO_METHOD_USERPTR:
		for (i = 0; i < n_buffers; ++i){
			if(memType == MEMORY_NORMAL){
				free(buffers[i].start);
			}else{
				cudaFreeHost(buffers[i].start);
			}
		}
		break;
	default:
		break;
	}

	free(buffers);
}

void v4l2_camera::init_read(unsigned int buffer_size)
{
	buffers = (struct buffer *)calloc(1, sizeof(*buffers));

	if (!buffers) {
		fprintf(stderr, "Out of memory\n");
		exit(EXIT_FAILURE);
	}

	buffers[0].length = buffer_size;
	buffers[0].start = (struct buffer *) malloc(buffer_size);

	if (!buffers[0].start) {
		fprintf(stderr, "Out of memory\n");
		exit(EXIT_FAILURE);
	}
}

void v4l2_camera::init_mmap(void)
{
	struct v4l2_requestbuffers req;

	CLEAR(req);

	req.count = 6;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_MMAP;

	if (-1 == xioctl(m_devFd, VIDIOC_REQBUFS, &req)) {
		if (EINVAL == errno) {
			fprintf(stderr, "%s does not support memory mapping\n", dev_name);
			exit(EXIT_FAILURE);
		} else {
			errno_exit("VIDIOC_REQBUFS");
		}
	}

	if (req.count < 2) {
		fprintf(stderr, "Insufficient buffer memory on %s\n", dev_name);
		exit(EXIT_FAILURE);
	}

	//printf("%s qbuf cnt = %d\n", dev_name, req.count);

	buffers = (struct buffer *)calloc(req.count, sizeof(*buffers));

	if (!buffers) {
		fprintf(stderr, "Out of memory\n");
		exit(EXIT_FAILURE);
	}

	for (n_buffers = 0; n_buffers < req.count; ++n_buffers) {
		struct v4l2_buffer buf;

		CLEAR(buf);

		buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory      = V4L2_MEMORY_MMAP;
		buf.index       = n_buffers;

		if (-1 == xioctl(m_devFd, VIDIOC_QUERYBUF, &buf))
			errno_exit("VIDIOC_QUERYBUF");

		buffers[n_buffers].length = buf.length;
		buffers[n_buffers].start  =	mmap(NULL /* start anywhere */,
										buf.length,
										PROT_READ | PROT_WRITE /* required */,
										MAP_SHARED /* recommended */,
										m_devFd, buf.m.offset);
		memset(buffers[n_buffers].start,0x80,buf.length);

		if (MAP_FAILED == buffers[n_buffers].start)
			errno_exit("mmap");
	}
}

void v4l2_camera::init_userp(unsigned int buffer_size)
{
	struct v4l2_requestbuffers req;
	cudaError_t ret = cudaSuccess;
	unsigned int page_size;
	page_size = getpagesize();
	buffer_size = (buffer_size + page_size-1)&~(page_size-1);

	CLEAR(req);

	req.count  = bufferCount;//6;//MAX_CHAN;
	req.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_USERPTR;

	if (-1 == xioctl(m_devFd, VIDIOC_REQBUFS, &req)) {
		if (EINVAL == errno) {
			fprintf(stderr, "%s does not support "
					"user pointer i/o\n", dev_name);
			exit(EXIT_FAILURE);
		} else {
			errno_exit("VIDIOC_REQBUFS");
		}
	}
	if (req.count < 2) {
			fprintf(stderr, "Insufficient buffer memory on %s\n", dev_name);
			exit(EXIT_FAILURE);
		}
	//printf("%s qbuf cnt = %d\n", dev_name, req.count);
	buffers = (struct buffer *)calloc(req.count, sizeof(*buffers));

	if (!buffers) {
		fprintf(stderr, "Out of memory\n");
		exit(EXIT_FAILURE);
	}

	for (n_buffers = 0; n_buffers < req.count; ++n_buffers) {
		buffers[n_buffers].length = buffer_size;
		if(memType == MEMORY_NORMAL)
			buffers[n_buffers].start = memalign(page_size,buffer_size);
		else // MEMORY_LOCKED
			ret = cudaHostAlloc(&buffers[n_buffers].start, buffer_size, cudaHostAllocDefault);
		assert(ret == cudaSuccess);
		//cudaFreeHost();
		//printf("%s %d: buffer.start = %p %d\n", __func__, __LINE__, buffers[n_buffers].start, buffer_size);

		if (!buffers[n_buffers].start) {
			fprintf(stderr, "Out of memory\n");
			exit(EXIT_FAILURE);
		}
	}
}

int v4l2_camera::init_device(void)
{
	struct v4l2_capability cap;
	struct v4l2_cropcap cropcap;
	struct v4l2_crop crop;

	unsigned int min;

	if (-1 == xioctl(m_devFd, VIDIOC_QUERYCAP, &cap)) {
		if (EINVAL == errno) {
			fprintf(stderr, "%s is no V4L2 device\n", dev_name);
			return FAILURE_DEVICEINIT;
		} else {
			errno_exit("VIDIOC_QUERYCAP");
		}
	}

	if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
		fprintf(stderr, "%s is no video capture device\n", dev_name);
		return FAILURE_DEVICEINIT;
	}

	switch (io) {
	case IO_METHOD_READ:
		if (!(cap.capabilities & V4L2_CAP_READWRITE)) {
			fprintf(stderr, "%s does not support read i/o\n", dev_name);
			return FAILURE_DEVICEINIT;
		}
		break;
	case IO_METHOD_MMAP:
	case IO_METHOD_USERPTR:
		if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
			fprintf(stderr, "%s does not support streaming i/o\n", dev_name);
			return FAILURE_DEVICEINIT;
		}
		break;
	}

	/* Select video input, video standard and tune here. */

	CLEAR(cropcap);

	cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	if (0 == xioctl(m_devFd, VIDIOC_CROPCAP, &cropcap)) {
		crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		crop.c = cropcap.defrect; /* reset to default */

		if (-1 == xioctl(m_devFd, VIDIOC_S_CROP, &crop)) {
		}
	} else {
		/* Errors ignored. */
	}

	CLEAR(m_fmt);

	m_fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (force_format) {
		//fprintf(stderr, "Set uyvy\r\n");
		m_fmt.fmt.pix.width       = imgwidth; //replace
		m_fmt.fmt.pix.height      = imgheight; //replace
		m_fmt.fmt.pix.pixelformat = imgformat;// V4L2_PIX_FMT_UYVY;
		m_fmt.fmt.pix.field       = V4L2_FIELD_ANY;
		//m_fmt.fmt.pix.code=0;
		//printf("[%d]******width =%d height=%d\n",__LINE__, m_fmt.fmt.pix.width,m_fmt.fmt.pix.height);

		if (-1 == xioctl(m_devFd, VIDIOC_S_FMT, &m_fmt))
		{
			 errno_exit("VIDIOC_S_FMT");
		}
		//printf("[%d]******width =%d height=%d\n",__LINE__, m_fmt.fmt.pix.width,m_fmt.fmt.pix.height);
		if (-1 == xioctl(m_devFd, VIDIOC_G_FMT, &m_fmt))
			errno_exit("VIDIOC_G_FMT");
		//printf("[%d]******width =%d height=%d\n",__LINE__, m_fmt.fmt.pix.width,m_fmt.fmt.pix.height);
		/* Note VIDIOC_S_FMT may change width and height. */
	} else {
		/* Preserve original settings as set by v4l2-ctl for example */
		if (-1 == xioctl(m_devFd, VIDIOC_G_FMT, &m_fmt))
			errno_exit("VIDIOC_G_FMT");
	}

	/* Buggy driver paranoia. */
	//min = m_fmt.fmt.pix.width * 2;
	min = imgwidth * 2;
	if (m_fmt.fmt.pix.bytesperline < min)
		m_fmt.fmt.pix.bytesperline = min;
	//min = m_fmt.fmt.pix.bytesperline * m_fmt.fmt.pix.height;
	min = m_fmt.fmt.pix.bytesperline * imgheight;
	if (m_fmt.fmt.pix.sizeimage < min)
		m_fmt.fmt.pix.sizeimage = min;

	switch (io) {
	case IO_METHOD_READ:
		init_read(m_fmt.fmt.pix.sizeimage);
		break;
	case IO_METHOD_MMAP:
		init_mmap();
		break;
	case IO_METHOD_USERPTR:
		init_userp(m_fmt.fmt.pix.sizeimage);
		break;
	}
	return EXIT_SUCCESS;
}

void v4l2_camera::close_device(void)
{
	if (-1 == close(m_devFd))
		errno_exit("close");

	m_devFd = -1;
}

int v4l2_camera::open_device(void)
{
	struct stat st;

	if (-1 == stat(dev_name, &st)) {
		fprintf(stderr, "Cannot identify '%s': %d, %s\n", dev_name, errno, strerror(errno));
		return FAILURE_DEVICEOPEN;
	}

	if (!S_ISCHR(st.st_mode)) {
		fprintf(stderr, "%s is no device\n", dev_name);
		return FAILURE_DEVICEOPEN;
	}

	m_devFd = open(dev_name, O_RDWR /* required */ | O_NONBLOCK, 0);
	if (-1 == m_devFd) {
		fprintf(stderr, "Cannot open '%s': %d, %s\n", dev_name, errno, strerror(errno));
		return FAILURE_DEVICEOPEN;
	}
	return EXIT_SUCCESS;
}

bool v4l2_camera::creat()
{
	int ret;

	ret = open_device();
	if(ret < 0)
		return false;

	ret = init_device();
	if(ret < 0)
		return false;

	return true;
}

void v4l2_camera::destroy()
{
	stop();
	uninit_device();
	close_device();
}

void v4l2_camera::run()
{
	start_capturing();
	bRun = true;
}

void v4l2_camera::stop()
{
	stop_capturing();
	bRun = false;
}


/*
void v4l2_camera::mainloop(void)
{
	fd_set fds;
	struct timeval tv;
	int ret;

	FD_ZERO(&fds);
	FD_SET(fd, &fds);

	/ * Timeout. * /
	tv.tv_sec = 2;
	tv.tv_usec = 0;

	ret = select(fd + 1, &fds, NULL, NULL, &tv);

	if(-1 == ret)
	{
		if (EINTR == errno)
			return;

		errno_exit("select");
	}else if(0 == ret)
	{
		fprintf(stderr, "select timeout\n");
		exit(EXIT_FAILURE);
	}

	if(-1 == read_frame())  / * EAGAIN - continue select loop. * /
		return;

}*/





