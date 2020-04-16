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
            RETURN_IF_FAILED(set_stream_type_mat_format(*pMatStreamType.Get(), m_sCurrentFormat));
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

            cMemoryBlock::MemCopy(oMat.data, pBuffer->GetPtr(), m_nSize);
            cv::cvtColor(oMat, oMat, cv::COLOR_RGB2BGR);
            object_ptr<const ISample> pOutSample = make_object_ptr<cOpenCVSample>(oMat);
            m_pOutput->Write(pOutSample);
        }
        RETURN_NOERROR;
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

    bool m_bError = tFalse;

public:
    ADTF_CLASS_ID_NAME(cMatToImageFilter,
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
            RETURN_IF_FAILED(get_stream_type_mat_format(m_sCurrentFormat, *pStreamType.Get()));

            m_bError = tFalse;
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
                m_bError = tTrue;
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
        if (!m_bError)
        {
            if (object_ptr<const IOpenCVSample> pMatSample = pSample)
            {
                cv::Mat oMat = pMatSample->GetMat();

                tInt64 nSize = oMat.total() * oMat.elemSize();
                if (nSize != m_nSize)
                {
                    RETURN_ERROR_DESC(ERR_NOT_SUPPORTED, "received mat size miss match %d != %d", oMat.total(), m_nSize);
                }

                if (oMat.type() != m_nMatType)
                {
                    RETURN_ERROR_DESC(ERR_NOT_SUPPORTED, "Recived Mat type missmatch %d != %d", oMat.type(), m_nMatType);
                }

                cv::cvtColor(oMat, oMat, COLOR_BGR2RGB);

                object_ptr<ISample> pNewSample;
                if (IS_OK(alloc_sample(pNewSample, pSample->GetTime())))
                {
                    object_ptr_locked<ISampleBuffer> pBuffer;
                    if (IS_OK(pNewSample->WriteLock(pBuffer, m_nSize)))
                    {
                        cMemoryBlock::MemCopy(pBuffer->GetPtr(), oMat.data, m_nSize);
                    }
                    m_pOutput->Write(pNewSample);
                }
            }
        }
        RETURN_NOERROR;
    }
};

ADTF_PLUGIN("OpenCV Mat To Image Plugin", 
    cMatToImageFilter, 
    cImageToMatFilter)