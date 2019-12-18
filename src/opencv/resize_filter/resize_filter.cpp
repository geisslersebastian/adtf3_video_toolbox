/**
 * Copyright 2019 Sebastian Geißler <mail@sebastiangeissler.de>
 *
 * (https://opensource.org/licenses/MIT)
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software 
 * and associated documentation files (the "Software"), to deal in the Software without restriction, 
 * including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, 
 * subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES 
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE 
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN 
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
 
#pragma once

#include <adtffiltersdk/adtf_filtersdk.h>
#include <adtfsystemsdk/adtf_systemsdk.h>

#include <opencv_base_filter/opencv_sample.h>
#include <opencv_base_filter/opencv_base_filter.h>

#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>

using namespace adtf::util;
using namespace adtf::ucom;
using namespace adtf::base;
using namespace adtf::streaming;
using namespace adtf::filter;
using namespace adtf::system;

using namespace cv::dnn;
using namespace cv;

using namespace adtf::videotb::opencv;

class cResizeFilter : public cOpenCVBaseFilter
{
public:
    ADTF_CLASS_ID_NAME(cResizeFilter,
        "resize.opencv.videotb.cid",
        "Resize Filter");

    property_variable<int> m_nWidth = 400;
    property_variable<int> m_nHeight = 400;

public:
    cResizeFilter()
    {
        RegisterPropertyVariable("width", m_nWidth);
        RegisterPropertyVariable("height", m_nHeight);
    }

    tStreamImageFormat ConvertImageFormat(const tStreamImageFormat & oImageFormat) override
    {
        tStreamImageFormat oResultImageFormat = oImageFormat;
        oResultImageFormat.m_ui32Width = m_nWidth;
        oResultImageFormat.m_ui32Height = m_nHeight;
        return oResultImageFormat;
    }

    cv::Mat ProcessMat(const cv::Mat & oMat)
    {
        cv::Mat oResult;
        resize(oMat, oResult, Size(m_nWidth, m_nHeight));
        return oResult;
    }
};

ADTF_PLUGIN("OpenCV Filter Plugin", 
    cResizeFilter)