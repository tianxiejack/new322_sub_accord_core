/*
 * gluWindow.hpp
 *
 *  Created on: Dec 17, 2018
 *      Author: wzk
 */

#ifndef GLUWINDOW_HPP_
#define GLUWINDOW_HPP_

#include <opencv2/opencv.hpp>
#include <osa_buf.h>
#include <glew.h>
#include <glut.h>
#include <freeglut_ext.h>

typedef cv::Matx<GLfloat, 4, 4> GLMatx44f;
typedef struct _ds_blend_param{
	GLfloat fAlpha;
	GLfloat thr0Min;
	GLfloat thr0Max;
	GLfloat thr1Min;
	GLfloat thr1Max;
}DS_BlendPrm;

class CGluWindow
{
	static int count;
public:
	CGluWindow(const cv::Rect& rc, int parent = 0);
	virtual ~CGluWindow();
	int Create(bool bFullScreen = true);
	void Destroy();
	int AddRender(const cv::Rect& rc);
	void RemoveRender(int index);
	void Display();

	int m_index;
	int m_winId;
	cv::Rect m_rc;
	int m_parent;
};


#endif /* GLUWINDOW_HPP_ */
