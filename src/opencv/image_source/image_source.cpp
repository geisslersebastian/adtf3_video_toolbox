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

class cOpenCVImagesSource : public adtf::filter::cSampleStreamingSource
{
public:
    ADTF_CLASS_ID_NAME(cOpenCVImagesSource,
        "images.opencv.videotb.cid",
        "Images Source");

    ADTF_CLASS_DEPENDENCIES(REQUIRE_INTERFACE(adtf::services::IReferenceClock),
        REQUIRE_INTERFACE(adtf::services::IKernel));

public:
    property_variable<cFilepathList> m_strImageFolders;
    property_variable<tInt32> m_nFramesPerSecond = 10;

    VideoCapture m_oCamera;

    cPinWriter* m_pOutput;
    tStreamImageFormat m_sCurrentFormat;

    kernel_thread_looper m_oThreadLoop;

    tInt32 m_nIndex = 0;
    std::vector<cString> m_lstImages;

    std::mutex m_oMutex;

public:

    cOpenCVImagesSource()
    {
        RegisterPropertyVariable("image_folders", m_strImageFolders);
        RegisterPropertyVariable("framesPerSecond", m_nFramesPerSecond);

        m_pOutput = CreateOutputPin("data");
    }

    ~cOpenCVImagesSource()
    {
    }

    tResult StopStreaming()
    {
        m_oThreadLoop = kernel_thread_looper();
        return adtf::filter::cSampleStreamingSource::StopStreaming();
    }

    tResult StartStreaming() override
    {
        RETURN_IF_FAILED(adtf::filter::cSampleStreamingSource::StartStreaming());
        for (auto strPath : *m_strImageFolders)
        {
            cStringList lstTempFiles;
            cFileSystem::EnumDirectory(strPath, lstTempFiles), "Parsing folder %s failed", strPath.GetPtr();

            for (auto strFilename : lstTempFiles)
            {
                cString strImagePath = strPath + "/" + strFilename;
                m_lstImages.push_back(strImagePath);
            }
        }
            
        m_oThreadLoop = kernel_thread_looper(cString(get_named_graph_object_full_name(*this) + "::capture_image"), &cOpenCVImagesSource::CaptureImage, this);

        if (!m_oThreadLoop.Joinable())
        {
            RETURN_ERROR_DESC(ERR_UNEXPECTED, "Unable to create kernel timer");
        }

        RETURN_NOERROR;
    }

    

    cString GetNextImage()
    {
        if (m_lstImages.size() == 0)
        {
            return "";
        }

        if (m_nIndex >= m_lstImages.size())
        {
            m_nIndex = 0;
        }

        return m_lstImages.at(m_nIndex++);
    }

    tVoid CaptureImage()
    {
        std::lock_guard<std::mutex> oLock(m_oMutex);

        cString strImagePath = GetNextImage();
        LOG_INFO("Load image %s", strImagePath.GetPtr());
        Mat oMatImage = cv::imread(strImagePath.GetPtr(),
                                   cv::IMREAD_COLOR);

        if (oMatImage.empty())
        {
            LOG_ERROR("Captured image is empty");
            return;
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

ADTF_PLUGIN("OpenCV Image Source Plugin", 
    cOpenCVImagesSource)