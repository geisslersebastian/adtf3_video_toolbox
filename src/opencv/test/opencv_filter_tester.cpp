#include <adtftesting/adtf_testing.h>
#include <adtfsystemsdk/testing/test_system.h>
#include <adtffiltersdk/adtf_filtersdk.h>

#include "../dnn/opencv_sample.h"

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
    if (IS_OK(alloc_sample(pSample, pSample->GetTime())))
    {
        object_ptr_locked<ISampleBuffer> pBuffer;
        if (IS_OK(pSample->WriteLock(pBuffer, nWidth * nHeight)))
        {
            tUInt8* pData = reinterpret_cast<tUInt8*>( pBuffer->GetPtr() );

            for (int i = 0; i < nWidth; i++)
            {
                for (int j = 0; j < nHeight; j++)
                {
                    pData[(i * nHeight + j) * 3] = 1;
                    pData[(i * nHeight + j) * 3] = 2;
                    pData[(i * nHeight + j) * 3] = 3;
                }
            }
        }
    }
    return pSample;
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

    auto oOutputSamples = oMat.GetCurrentOutput().GetSamples();

    REQUIRE(oOutputSamples.size() == 1);

    object_ptr<const IOpenCVSample> pOpenCVSample = oOutputSamples.back();
    REQUIRE(pOpenCVSample);

    Mat oResultMat = pOpenCVSample->GetMat();
    REQUIRE(oResultMat.at<cv::Vec3b>(0, 1)[0] == 1);
    REQUIRE(oResultMat.at<cv::Vec3b>(0, 1)[1] == 2);
    REQUIRE(oResultMat.at<cv::Vec3b>(0, 1)[2] == 3);
}

