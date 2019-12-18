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
#include <opencv2/opencv.hpp>

namespace adtf
{
namespace videotb
{
namespace opencv
{

    adtf::streaming::tStreamImageFormat create_stream_type(const cv::Mat & oMat);
    tResult check_stream_type(const cv::Mat & oMat, adtf::streaming::tStreamImageFormat & oCurrentType, adtf::filter::cPinWriter* pOutput);

    class cOpenCVBaseFilter : public adtf::filter::cFilter
    {
    private:
        adtf::filter::cPinWriter* m_pOutput;
        adtf::filter::cPinReader* m_pInput;

        adtf::streaming::tStreamImageFormat m_sCurrentFormat;

    public:
        cOpenCVBaseFilter();

        virtual adtf::streaming::tStreamImageFormat ConvertImageFormat(const adtf::streaming::tStreamImageFormat & oImageFormat);

        virtual cv::Mat ProcessMat(const cv::Mat & oMat) = 0;
        virtual tResult OnStageFirst() { RETURN_NOERROR; };
        virtual tResult OnStagePreConnect() { RETURN_NOERROR; };
        virtual tResult OnStagePostConnect() { RETURN_NOERROR; };

        tResult ProcessInput(adtf::streaming::ISampleReader* pReader,
            const adtf::ucom::iobject_ptr<const adtf::streaming::ISample>& pSample);

        tResult Init(adtf::streaming::ant::cFilterLevelmachine::tInitStage eStage);
    };
}
}
}