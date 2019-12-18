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

class cOpenCVCameraSource : public adtf::filter::cSampleStreamingSource
{
public:
    ADTF_CLASS_ID_NAME(cOpenCVCameraSource,
        "camera.opencv.videotb.cid",
        "Camera Source");

    ADTF_CLASS_DEPENDENCIES(REQUIRE_INTERFACE(adtf::services::IReferenceClock),
                            REQUIRE_INTERFACE(adtf::services::IKernel));

public:
    property_variable<tInt32> m_nCameraID = 0;
    property_variable<tInt32> m_nFramesPerSecond = 10;

    VideoCapture m_oCamera;

    cPinWriter* m_pOutput;
    tStreamImageFormat m_sCurrentFormat;

    kernel_thread_looper m_oThreadLoop;

public:

    cOpenCVCameraSource()
    {
        RegisterPropertyVariable("cameraId", m_nCameraID);
        RegisterPropertyVariable("framesPerSecond", m_nFramesPerSecond);

        m_pOutput = CreateOutputPin("data");
    }

    ~cOpenCVCameraSource()
    {

    }

    tResult StartStreaming() override
    {
        m_oCamera.open(*m_nCameraID);
        if (!m_oCamera.isOpened()) 
        {
            RETURN_ERROR_DESC(ERR_DEVICE_NOT_READY, "Unable to open camera");
        }

        m_oThreadLoop = kernel_thread_looper(cString(get_named_graph_object_full_name(*this) + "::capture_image"), &cOpenCVCameraSource::CaptureImage, this);

        if (!m_oThreadLoop.Joinable())
        {
            RETURN_ERROR_DESC(ERR_UNEXPECTED, "Unable to create kernel timer");
        }

        RETURN_NOERROR;
    }

    tVoid CaptureImage()
    {
        Mat oMatImage;
        m_oCamera >> oMatImage;
        if (oMatImage.empty())
        {
            LOG_ERROR("Captured image is empty");
        }

        if (IS_FAILED(check_stream_type(oMatImage, m_sCurrentFormat, m_pOutput)))
        {
            return;
        }

        object_ptr<const ISample> pSample = make_object_ptr<cOpenCVSample>(oMatImage);
        m_pOutput->Write(pSample);
        m_pOutput->ManualTrigger();


        std::this_thread::sleep_for(std::chrono::milliseconds(1000 / m_nFramesPerSecond));
    }

};

ADTF_PLUGIN("OpenCV Camera Source Plugin", 
    cOpenCVCameraSource)