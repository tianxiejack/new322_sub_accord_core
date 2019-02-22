/*
 * secondScreen.hpp
 *
 *  Created on: Feb 22, 2019
 *      Author: ubuntu
 */

#ifndef SECONDSCREEN_HPP_
#define SECONDSCREEN_HPP_

#include "crcoreSecondScreen.hpp"

class CT
{
public:
	CT(){OSA_printf("%s %d %s", __FILE__, __LINE__, __func__);};
	virtual ~CT(){OSA_printf("%s %d %s", __FILE__, __LINE__, __func__);};
};
class CSecondScreen : public CSecondScreenBase
{
	CT m_ct;
public:
	CSecondScreen(const cv::Rect& rc, int fps, int fontSize = 45, const char* fontFile = NULL):
		CSecondScreenBase(rc, fps, true, fontSize, fontFile){};
	virtual ~CSecondScreen(){};
};

#endif /* SECONDSCREEN_HPP_ */
