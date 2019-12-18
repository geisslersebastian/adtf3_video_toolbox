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
 
#include <adtftesting/adtf_testing.h>
#include <adtfsystemsdk/testing/test_system.h>
#include <adtffiltersdk/adtf_filtersdk.h>

#include <opencv_base_filter/opencv_sample.h>

#include <opencv2/core.hpp>

using namespace adtf::util;
using namespace adtf::ucom;
using namespace adtf::base;
using namespace adtf::streaming;
using namespace adtf::filter;
using namespace adtf::services;
using namespace adtf::videotb::opencv;
using namespace cv;

struct cMyTestSystem: adtf::system::testing::cTestSystem
{
    cMyTestSystem()
    {
        LoadPlugin("opencv_filter.adtfplugin");
    }
};

object_ptr<ISample> CreateImage(tInt32 nWidth, tInt32 nHeight)
{
    object_ptr<ISample> pSample;
    if (IS_OK(alloc_sample(pSample, 0)))
    {
        object_ptr_locked<ISampleBuffer> pBuffer;
        if (IS_OK(pSample->WriteLock(pBuffer, nWidth * nHeight * 3)))
        {
            tUInt8* pData = reinterpret_cast<tUInt8*>( pBuffer->GetPtr() );
            
            for (int i = 0; i < nHeight; i++)
            {
                for (int j = 0; j < nWidth; j++)
                {
                    pData[(i * nHeight + j) * 3] = 1;
                    pData[(i * nHeight + j) * 3 + 1] = 2;
                    pData[(i * nHeight + j) * 3 + 2] = 3;
                }
            }
        }
    }
    return pSample;
}

object_ptr<ISample> CreateMat(tInt32 nWidth, tInt32 nHeight)
{
    auto oMat = Mat(nWidth, nHeight, CV_8UC3, Scalar(1, 2, 3));
    return make_object_ptr<cOpenCVSample>(oMat);
}


TEST_CASE_METHOD(cMyTestSystem, "Mat To Image")
{
    adtf::ucom::object_ptr<adtf::streaming::IFilter> pFilter;
    REQUIRE_OK(_runtime->CreateInstance("mat_to_image.opencv.videotb.cid", pFilter));

    tStreamImageFormat m_sCurrentFormat;
    m_sCurrentFormat.m_strFormatName = ADTF_IMAGE_FORMAT(RGB_24);
    m_sCurrentFormat.m_ui32Height = 2;
    m_sCurrentFormat.m_ui32Width = 2;
    m_sCurrentFormat.m_szMaxByteSize = m_sCurrentFormat.m_ui32Width * m_sCurrentFormat.m_ui32Height * 3;
    
    object_ptr<IStreamType> pStreamType = make_object_ptr<cStreamType>(stream_meta_type_mat());
    REQUIRE(IS_OK(set_stream_type_mat_format(*pStreamType, m_sCurrentFormat)));

    adtf::filter::testing::cTestWriter oImage(pFilter, "mat", pStreamType);
    adtf::filter::testing::cOutputRecorder oMat(pFilter, "image");

    REQUIRE_OK(pFilter->SetState(adtf::streaming::IFilter::tFilterState::State_Running));
    
    auto pInputSample = CreateMat(2, 2);
    oImage.WriteSample(pInputSample);
    oImage.ManualTrigger();

    auto oOutputSamples = oMat.GetCurrentOutput().GetSamples();

    REQUIRE(oOutputSamples.size() == 1);

    object_ptr<const ISample> pImageSample = oOutputSamples.back();
    REQUIRE(pImageSample);

    object_ptr_shared_locked<const ISampleBuffer> pBuffer;
    REQUIRE(IS_OK(pImageSample->Lock(pBuffer)));

    const tUInt8* pData = reinterpret_cast<const tUInt8*>(pBuffer->GetPtr());
    REQUIRE(pData[0] == 1);
    REQUIRE(pData[1] == 2);
    REQUIRE(pData[2] == 3);
}

TEST_CASE_METHOD(cMyTestSystem, "Image to Mat")
{
    adtf::ucom::object_ptr<adtf::streaming::IFilter> pFilter;
    REQUIRE_OK(_runtime->CreateInstance("image_to_mat.opencv.videotb.cid", pFilter));

    tStreamImageFormat m_sCurrentFormat;
    m_sCurrentFormat.m_strFormatName = ADTF_IMAGE_FORMAT(RGB_24);
    m_sCurrentFormat.m_ui32Height = 2;
    m_sCurrentFormat.m_ui32Width = 2;
    m_sCurrentFormat.m_szMaxByteSize = m_sCurrentFormat.m_ui32Width * m_sCurrentFormat.m_ui32Height * 3;

    object_ptr<IStreamType> pStreamType = make_object_ptr<cStreamType>(stream_meta_type_image());
    set_stream_type_image_format(*pStreamType, m_sCurrentFormat);

    adtf::filter::testing::cTestWriter oImage(pFilter, "image", pStreamType);
    adtf::filter::testing::cOutputRecorder oMat(pFilter, "mat");

    REQUIRE_OK(pFilter->SetState(adtf::streaming::IFilter::tFilterState::State_Running));

    auto pInputSample = CreateImage(2, 2);
    oImage.WriteSample(pInputSample);
    oImage.ManualTrigger();

    auto oOutputSamples = oMat.GetCurrentOutput().GetSamples();

    REQUIRE(oOutputSamples.size() == 1);

    object_ptr<const IOpenCVSample> pOpenCVSample = oOutputSamples.back();
    REQUIRE(pOpenCVSample);

    Mat oResultMat = pOpenCVSample->GetMat();
    Vec3b oVector = oResultMat.at<Vec3b>(0, 1);
    REQUIRE(oVector[0] == 1);
    REQUIRE(oVector[1] == 2);
    REQUIRE(oVector[2] == 3);
}