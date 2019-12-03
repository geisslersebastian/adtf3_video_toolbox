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
#include <adtfsystemsdk/adtf_systemsdk.h>

#include "opencv_sample.h"

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

tStreamImageFormat create_stream_type(const Mat & oMat)
{
    tStreamImageFormat oStreamType;

    oStreamType.m_ui32Height = oMat.rows;
    oStreamType.m_ui32Width = oMat.cols;

    if (oMat.type() == CV_8UC3)
    {
        oStreamType.m_strFormatName = ADTF_IMAGE_FORMAT(RGB_24);
        oStreamType.m_szMaxByteSize = oMat.rows * oMat.cols * 3;
    }
    else if (oMat.type() == CV_8UC1)
    {
        oStreamType.m_strFormatName = ADTF_IMAGE_FORMAT(GREYSCALE_8);
        oStreamType.m_szMaxByteSize = oMat.rows * oMat.cols * 1;
    }
    else
    {
        oStreamType.m_strFormatName = "";
    }

    return oStreamType;
}

tResult check_stream_type(const Mat & oMat, tStreamImageFormat & oCurrentType, cPinWriter* pOutput)
{
    auto oStreamType = create_stream_type(oMat);

    if ((oCurrentType.m_ui32Height != oStreamType.m_ui32Height ||
        oCurrentType.m_ui32Width != oStreamType.m_ui32Width ||
        oCurrentType.m_szMaxByteSize != oStreamType.m_szMaxByteSize ||
        oCurrentType.m_strFormatName != oStreamType.m_strFormatName)
        && oStreamType.m_strFormatName != "")
    {
        object_ptr<IStreamType> pMatStreamType = make_object_ptr<cStreamType>(stream_meta_type_mat());
        RETURN_IF_FAILED(set_stream_type_mat_format(*pMatStreamType.Get(), oStreamType));
        pOutput->ChangeType(pMatStreamType);
        oCurrentType = oStreamType;
    }

    if (oCurrentType.m_strFormatName == "")
    {
        RETURN_ERROR_DESC(ERR_UNKNOWN_FORMAT, "Image format is unkonwn");
    }

    RETURN_NOERROR;
}

class cOpenCVBaseFilter : public cFilter
{
private:
    cPinWriter* m_pOutput;
    cPinReader* m_pInput;

    tStreamImageFormat m_sCurrentFormat;

public:
    cOpenCVBaseFilter()
    {
        object_ptr<IStreamType> pStreamType = make_object_ptr<cStreamType>(stream_meta_type_mat());
        m_pOutput = CreateOutputPin("mat_out", pStreamType);
        m_pInput = CreateInputPin("mat_in", pStreamType);
        m_pInput->SetAcceptTypeCallback([this](const auto & pStreamType) -> tResult
        {
            RETURN_IF_FAILED(get_stream_type_mat_format(m_sCurrentFormat, *pStreamType.Get()));
            m_sCurrentFormat = ConvertImageFormat(m_sCurrentFormat);
            object_ptr<IStreamType> pImageStreamType = make_object_ptr<cStreamType>(stream_meta_type_mat());
            RETURN_IF_FAILED(set_stream_type_mat_format(*pImageStreamType.Get(), m_sCurrentFormat));
            RETURN_IF_FAILED(m_pOutput->ChangeType(pImageStreamType));
            RETURN_NOERROR;
        });
    }
    
    virtual tStreamImageFormat ConvertImageFormat(const tStreamImageFormat oImageFormat)
    {
        return oImageFormat;
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

    tStreamImageFormat ConvertImageFormat(const tStreamImageFormat oImageFormat) override
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
    std::vector<std::string> m_vecOutputLayerNames;

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
        if (!cFileSystem::Exists(m_strConfig))
        {
            RETURN_ERROR_DESC(ERR_NOT_FOUND, "Could not find dnn config at %s", m_strConfig->GetPtr());
        }
        if (!cFileSystem::Exists(m_strModule))
        {
            RETURN_ERROR_DESC(ERR_NOT_FOUND, "Could not find dnn module at %s", m_strModule->GetPtr());
        }

        m_oDnnNet = readNet(m_strConfig->GetPtr(),
            m_strModule->GetPtr());

        if (m_oDnnNet.empty())
        {
            RETURN_ERROR_DESC(ERR_NOT_READY, "Error while creating Dnn net");
        }

        m_vecOutputLayerNames.clear();

        std::vector<int> unconnectedLayers = m_oDnnNet.getUnconnectedOutLayers();
        std::vector<String> allLayerNames = m_oDnnNet.getLayerNames();
        m_vecOutputLayerNames.resize(unconnectedLayers.size());
        
        for (size_t i = 0; i < unconnectedLayers.size(); ++i)
        {
            m_vecOutputLayerNames.push_back(allLayerNames[unconnectedLayers[i] - 1]);
        }

        RETURN_NOERROR;
    }

    std::vector<String> getOutputsNames(const Net& net)
    {
        static std::vector<String> names;
        if (names.empty())
        {
            std::vector<int> outLayers = net.getUnconnectedOutLayers();
            std::vector<String> layersNames = net.getLayerNames();
            names.resize(outLayers.size());
            for (size_t i = 0; i < outLayers.size(); ++i)
                names[i] = layersNames[outLayers[i] - 1];
        }
        return names;
    }

    cv::Mat ProcessMat(const cv::Mat & oMat)
    {
        if (!m_oDnnNet.empty())
        {
            Mat oBlob = blobFromImage(oMat,
                *m_fBlobScale,
                Size(m_fBlobWidth, m_fBlobHeight),
                Scalar(m_fBlobMeanRed, m_fBlobMeanGreen, m_fBlobMeanBlue),
                true,
                false);

            m_oDnnNet.setInput(oBlob);
            if (m_oDnnNet.getLayer(0)->outputNameToIndex("im_info") != -1)  // Faster-RCNN or R-FCN
            {
                resize(oMat, oMat, Size(m_fBlobWidth, m_fBlobHeight));
                Mat imInfo = (Mat_<float>(1, 3) << m_fBlobHeight, m_fBlobWidth, 1.6f);
                m_oDnnNet.setInput(imInfo, "im_info");
            }
            getOutputsNames(m_oDnnNet);

            std::vector<Mat> outs;
            m_oDnnNet.forward(outs, getOutputsNames(m_oDnnNet));

            std::vector<int> outLayers = m_oDnnNet.getUnconnectedOutLayers();
            std::string outLayerType = m_oDnnNet.getLayer(outLayers[0])->type;

            return outs[0];
        }

        return Mat();
    }
    
};

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

ADTF_PLUGIN("OpenCV Filter Plugin", 
    cOpenCVImagesSource, 
    cMatToImageFilter, 
    cImageToMatFilter, 
    cDNNOpenCVFilter, 
    cOpenCVCameraSource, 
    cResizeFilter)