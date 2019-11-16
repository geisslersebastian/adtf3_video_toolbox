/**
 * Copyright 2019 Sebastian Geißler <mail@sebastian.geissler.de>
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

#include "opencv_sample.h"

#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>

using namespace adtf::util;
using namespace adtf::ucom;
using namespace adtf::base;
using namespace adtf::streaming;
using namespace adtf::filter;
using namespace adtf::services;

using namespace cv::dnn;
using namespace cv;

using namespace adtf::videotb::opencv;

class cOpenCVBaseFilter : public cFilter
{
private:
    cPinWriter* m_pOutput;
    cPinReader* m_pInput;

public:
    cOpenCVBaseFilter()
    {
        object_ptr<IStreamType> pStreamType = make_object_ptr<cStreamType>(stream_meta_type_mat());
        m_pOutput = CreateOutputPin("mat_out", pStreamType);
        m_pInput = CreateInputPin("mat_in", pStreamType);
    }

    virtual cv::Mat ProcessMat(const cv::Mat & oMat) = 0;
    virtual tResult OnStageFirst() { RETURN_NOERROR; };
    virtual tResult OnStagePreConnect() { RETURN_NOERROR; };
    virtual tResult OnStagePostConnect() { RETURN_NOERROR; };

    tResult ProcessInput(ISampleReader* pReader,
        const iobject_ptr<const ISample>& pSample)
    {
        object_ptr<const IOpenCVSample> pMatSample = pSample;
        if (pMatSample)
        {
            cv::Mat oMat = ProcessMat(pMatSample->GetMat());
            if(!oMat.empty())
            {
                object_ptr<ISample> pSample = make_object_ptr<cOpenCVSample>(oMat);
                m_pOutput->Write(pSample);
            }
        }
        RETURN_NOERROR;
    }

    tResult Init(tInitStage eStage)
    {
        RETURN_IF_FAILED(cFilter::Init(eStage));

        switch (eStage)
        {
            case tInitStage::StageFirst:
            {
                RETURN_IF_FAILED(OnStageFirst());
            }
            break;
            case tInitStage::StagePreConnect:
            {
                RETURN_IF_FAILED(OnStagePreConnect());
            }
            break;
            case tInitStage::StagePostConnect:
            {
                RETURN_IF_FAILED(OnStagePostConnect());
            }
            break;
        }

        RETURN_NOERROR;
    }

};

class cImageToMatFilter : public cFilter
{
private:
    cPinWriter* m_pOutput;
    cPinReader* m_pInput;
    tStreamImageFormat m_sCurrentFormat;

    tInt32 m_nMatType;
    tInt32 m_nSize;

public:
    ADTF_CLASS_ID_NAME(cImageToMatFilter,
        "image_to_mat.opencv.videotb.cid",
        "Image to Mat Filter");

public:
    cImageToMatFilter()
    {
        object_ptr<IStreamType> pMatStreamType = make_object_ptr<cStreamType>(stream_meta_type_mat());
        m_pOutput = CreateOutputPin("mat", pMatStreamType);

        object_ptr<IStreamType> pImageStreamType = make_object_ptr<cStreamType>(stream_meta_type_image());
        m_pInput = CreateInputPin("image", pImageStreamType);

        m_pInput->SetAcceptTypeCallback([this](const auto & pStreamType) -> tResult
        {
            RETURN_IF_FAILED(get_stream_type_image_format(m_sCurrentFormat, *pStreamType.Get()));

            if (m_sCurrentFormat.m_strFormatName == ADTF_IMAGE_FORMAT(RGB_24))
            {
                m_nMatType = CV_8UC3;
                m_nSize = m_sCurrentFormat.m_ui32Width * m_sCurrentFormat.m_ui32Height * 3;
            }
            else if (m_sCurrentFormat.m_strFormatName == ADTF_IMAGE_FORMAT(GREYSCALE_8))
            {
                m_nMatType = CV_8UC1;
                m_nSize = m_sCurrentFormat.m_ui32Width * m_sCurrentFormat.m_ui32Height * 1;
            }
            else
            {
                RETURN_ERROR_DESC(ERR_NOT_SUPPORTED, "Image to Mat not support image format");
            }

            object_ptr<IStreamType> pMatStreamType = make_object_ptr<cStreamType>(stream_meta_type_mat());
            set_stream_type_image_format(*pMatStreamType.Get(), m_sCurrentFormat);
            m_pOutput->ChangeType(pMatStreamType);

            RETURN_NOERROR;
        });
    }

    tResult ProcessInput(ISampleReader* pReader,
        const iobject_ptr<const ISample>& pSample)
    {
        object_ptr_shared_locked<const ISampleBuffer> pBuffer;
        if (IS_OK(pSample->Lock(pBuffer)))
        {
            if (pBuffer->GetSize() != m_nSize)
            {
                RETURN_ERROR_DESC(ERR_NOT_SUPPORTED, "received sample size miss match %d != %d", pBuffer->GetSize(), m_nSize);
            }

            Mat oMat = Mat(m_sCurrentFormat.m_ui32Height,
                           m_sCurrentFormat.m_ui32Width,
                           m_nMatType);

            memcpy(oMat.data, pBuffer->GetPtr(), m_nSize);

            object_ptr<const ISample> pOutSample = make_object_ptr<cOpenCVSample>(oMat);
            m_pOutput->Write(pOutSample);
        }
    }
    
};

class cMatToImageFilter : public cFilter
{
private:
    cPinWriter* m_pOutput;
    cPinReader* m_pInput;
    tStreamImageFormat m_sCurrentFormat;

    tInt64 m_nMatType;
    tInt64 m_nSize;

public:
    ADTF_CLASS_ID_NAME(cImageToMatFilter,
        "mat_to_image.opencv.videotb.cid",
        "Mat to Image Filter");

public:
    cMatToImageFilter()
    {
        object_ptr<IStreamType> pImageStreamType = make_object_ptr<cStreamType>(stream_meta_type_image());
        m_pOutput = CreateOutputPin("image", pImageStreamType);

        object_ptr<IStreamType> pMatStreamType = make_object_ptr<cStreamType>(stream_meta_type_mat());
        m_pInput = CreateInputPin("mat", pMatStreamType);

        m_pInput->SetAcceptTypeCallback([this](const iobject_ptr<const IStreamType> & pStreamType) -> tResult
        {
            RETURN_IF_FAILED(get_stream_type_image_format(m_sCurrentFormat, *pStreamType.Get()));

            if (m_sCurrentFormat.m_strFormatName == ADTF_IMAGE_FORMAT(RGB_24))
            {
                m_nMatType = CV_8UC3;
                m_nSize = m_sCurrentFormat.m_ui32Width * m_sCurrentFormat.m_ui32Height * 3;
            }
            else if (m_sCurrentFormat.m_strFormatName == ADTF_IMAGE_FORMAT(GREYSCALE_8))
            {
                m_nMatType = CV_8UC1;
                m_nSize = m_sCurrentFormat.m_ui32Width * m_sCurrentFormat.m_ui32Height * 1;
            }
            else
            {
                RETURN_ERROR_DESC(ERR_NOT_SUPPORTED, "Image to Mat not support image format");
            }

            object_ptr<IStreamType> pImageStreamType = make_object_ptr<cStreamType>(stream_meta_type_image());
            RETURN_IF_FAILED(set_stream_type_image_format(*pImageStreamType.Get(), m_sCurrentFormat));
            RETURN_IF_FAILED(m_pOutput->ChangeType(pImageStreamType));

            RETURN_NOERROR;
        });
    }

    tResult ProcessInput(ISampleReader* pReader,
        const iobject_ptr<const ISample>& pSample)
    {
        if (object_ptr<const IOpenCVSample> pMatSample = pSample)
        {
            cv::Mat oMat = pMatSample->GetMat();

            if (oMat.total() != m_nSize)
            {
                RETURN_ERROR_DESC(ERR_NOT_SUPPORTED, "received mat size miss match %d != %d", oMat.total(), m_nSize);
            }

            object_ptr<ISample> pSample;
            if (IS_OK(alloc_sample(pSample, pSample->GetTime())))
            {
                object_ptr_locked<ISampleBuffer> pBuffer;
                if (IS_OK(pSample->WriteLock(pBuffer, m_nSize)))
                {
                    memcpy(pBuffer->GetPtr(), oMat.data, m_nSize);
                }
                m_pOutput->Write(pSample);
            }
        }
    }
};

class cDNNOpenCVFilter : public cOpenCVBaseFilter
{
public: 
    ADTF_CLASS_ID_NAME(cDNNOpenCVFilter,
        "dnn.opencv.videotb.cid",
        "DNN Filter");

public:
    property_variable<cFilename> m_strModule;
    property_variable<cFilename> m_strConfig;

    property_variable<tFloat32> m_fBlobScale = 1.0;
    property_variable<tFloat32> m_fBlobMeanRed = 0.0;
    property_variable<tFloat32> m_fBlobMeanGreen = 0.0;
    property_variable<tFloat32> m_fBlobMeanBlue = 0.0;

    property_variable<tInt32> m_fBlobWidth = 224;
    property_variable<tInt32> m_fBlobHeight = 224;

    Net m_oDnnNet;

public:
    
    cDNNOpenCVFilter()
    {
        SetDescription("OpenCV DNN Filter");

        RegisterPropertyVariable("module", m_strModule);
        RegisterPropertyVariable("config", m_strConfig);
        
        RegisterPropertyVariable("blob_scale", m_fBlobScale);
        RegisterPropertyVariable("blob_mean_red", m_fBlobMeanRed);
        RegisterPropertyVariable("blob_mean_green", m_fBlobMeanGreen);
        RegisterPropertyVariable("blob_mean_blue", m_fBlobMeanBlue);

        RegisterPropertyVariable("blob_input_width", m_fBlobWidth);
        RegisterPropertyVariable("blob_input_height", m_fBlobHeight);
    }
    
    ~cDNNOpenCVFilter()
    {
        
    }

    tResult OnStagePreConnect() override
    {
        if (cFileSystem::Exists(m_strConfig))
        {
            RETURN_ERROR_DESC(ERR_NOT_FOUND, "Could not find dnn config at %s", m_strConfig->GetPtr());
        }
        if (cFileSystem::Exists(m_strModule))
        {
            RETURN_ERROR_DESC(ERR_NOT_FOUND, "Could not find dnn module at %s", m_strModule->GetPtr());
        }

        m_oDnnNet = readNet(m_strConfig->GetPtr(),
            m_strModule->GetPtr());

        if (m_oDnnNet.empty())
        {
            RETURN_ERROR_DESC(ERR_NOT_READY, "Error while creating Dnn net");
        }
        RETURN_NOERROR;
    }

    cv::Mat ProcessMat(const cv::Mat & oMat)
    {
        if(!m_oDnnNet.empty())
        {
            Mat oBlob = blobFromImage(oMat,
                                      *m_fBlobScale, 
                                      Size(m_fBlobWidth, m_fBlobHeight), 
                                      Scalar(m_fBlobMeanRed, m_fBlobMeanGreen, m_fBlobMeanBlue), 
                                      false, 
                                      false);
            m_oDnnNet.setInput(oBlob);
            return m_oDnnNet.forward();
        }

        return cv::Mat();
    }

};

ADTF_PLUGIN("OpenCV Filter Plugin", cDNNOpenCVFilter)