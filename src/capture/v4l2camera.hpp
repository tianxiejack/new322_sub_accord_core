/*
 * v4l2camera.hpp
 *
 *  Created on: 2017年3月17日
 *      Author: sh
 */

#ifndef V4L2CAMERA_HPP_
#define V4L2CAMERA_HPP_

#include <sys/types.h>
#include <opencv2/opencv.hpp>
#include <linux/videodev2.h>
//#include "Camera.h"
//#include "timing.h"
//#include "struct.hpp"
#include "osa_buf.h"

using namespace cv;
//#define SHOW_TIMES
#define CLEAR(x) memset(&(x), 0, sizeof(x))


#define MAX_CHAN     (3)	// for XGS021
#define BUFFER_CNT_PER_CHAN	(6)
#define LARGE_PHOTO_WIDTH  (4096)
#define LARGE_PHOTO_HEIGHT (3072)

#define CAP_CH_NUM 		(1)

#define IMAGE_WIDTH  	1920		//1440
#define IMAGE_HEIGHT 	1080	//576

#ifndef V4L2_PIX_FMT_H264
#define V4L2_PIX_FMT_H264     v4l2_fourcc('H', '2', '6', '4') /* H264 with start codes */
#endif

//#define EXIT_SUCCESS         0;
#define FAILURE_ALLOCBUFFER  -1;
#define FAILURE_DEVICEOPEN   -2;
#define FAILURE_DEVICEINIT   -3;

#define BUFFER_HEAD_LEN			(sizeof(int)*32)
#define BUFFER_ID(p)			((int*)(p) + 0)
#define BUFFER_CHID(p)			((int*)(p) + 1)
#define BUFFER_DATA(p)			((unsigned char*)(p) + BUFFER_HEAD_LEN)
#define BUFFER_ID_INV(data)		BUFFER_ID( ((unsigned char*)(data) - BUFFER_HEAD_LEN) )
#define BUFFER_CHID_INV(data)	BUFFER_CHID( ((unsigned char*)(data) - BUFFER_HEAD_LEN) )

enum io_method {
	IO_METHOD_READ,
	IO_METHOD_MMAP,
	IO_METHOD_USERPTR,
};

enum	memory_type{
	MEMORY_NORMAL,
	MEMORY_LOCKED,

};
struct buffer {
	void   *start;
	size_t  length;
};

class v4l2_camera
{

public:
	v4l2_camera(int devId);
	~v4l2_camera();
	bool creat(void);
	void destroy(void);
	void run(void);
	void stop();
	static int  xioctl(int fh, int request, void *arg);
	int m_devFd;
	int imgwidth;
	int imgheight;
	int imgtype;
	int imgformat;
	struct v4l2_format m_fmt;
	struct buffer   *buffers;
	bool bRun;
private:
	int  open_device(void);
	void close_device(void);
	int  init_device(void);
	void uninit_device(void);
	void init_userp(unsigned int buffer_size);
	void init_mmap(void);
	void init_read(unsigned int buffer_size);

	void start_capturing(void);
	void stop_capturing(void);
	int  read_frame(unsigned char **data);


	void errno_exit(const char *s);

	char            dev_name[16];

	enum io_method  io;
	enum	memory_type		memType;

	unsigned int     n_buffers;
	unsigned int     bufSize;
	int							 bufferCount;

	int imgstride;
	int force_format;
	int Id;

};

typedef unsigned char uchar;
typedef	struct Header_Line{
	uchar frameNo:4;   /* Byte 1 */
	uchar HD1:4;

	uchar fieldNo:1;   /* Byte 2 */
	char :3;
	uchar HD2:4;

	uchar LineNoH:4;   /* Byte 3 */
	uchar HD3:4;

	uchar LineNoM:4;   /* Byte 4 */
	uchar HD4:4;

	uchar LineNoL:4;   /* Byte 5 */
	uchar HD5:4;

	uchar ChId:2;      /* Byte 6 */
	char :2;
	uchar HD6:4;

	char VLock:1;    /* Byte 7 */
	char SLock:1;
	char HLock:1;
	char VDLoss:1;
	uchar HD7:4;

	char V_Stable:1; /* Byte 8 */
	char Derstus:1;
	char DET50:1;
	char MONO:1;
	uchar HD8:4;
}LineHeader;


#endif /* V4L2CAMERA_HPP_ */
