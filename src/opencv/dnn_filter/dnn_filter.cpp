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

    property_variable<tInt32> m_nBackend = 0;
    property_variable<tInt32> m_nTarget = 0;

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

        m_nBackend.SetDescription("Enum of computation backends supported by layers.");
        m_nBackend.SetValueList({
            {cv::dnn::DNN_BACKEND_DEFAULT, "DNN_BACKEND_DEFAULT"},
            {cv::dnn::DNN_BACKEND_HALIDE, "DNN_BACKEND_HALIDE"},
            {cv::dnn::DNN_BACKEND_INFERENCE_ENGINE, "DNN_BACKEND_INFERENCE_ENGINE"},
            {cv::dnn::DNN_BACKEND_OPENCV, "DNN_BACKEND_OPENCV"},
            {cv::dnn::DNN_BACKEND_VKCOM, "DNN_BACKEND_VKCOM"},
            {cv::dnn::DNN_BACKEND_CUDA, "DNN_BACKEND_CUDA"},
            });

        RegisterPropertyVariable("backend", m_nBackend);

        m_nTarget.SetDescription("Enum of target devices for computations.");
        m_nTarget.SetValueList({
            {cv::dnn::DNN_TARGET_CPU, "DNN_TARGET_CPU"},
            {cv::dnn::DNN_TARGET_OPENCL, "DNN_TARGET_OPENCL"},
            {cv::dnn::DNN_TARGET_OPENCL_FP16, "DNN_TARGET_OPENCL_FP16"},
            {cv::dnn::DNN_TARGET_MYRIAD, "DNN_TARGET_MYRIAD"},
            {cv::dnn::DNN_TARGET_VULKAN, "DNN_TARGET_VULKAN"},
            {cv::dnn::DNN_TARGET_FPGA, "DNN_TARGET_FPGA"},
            {cv::dnn::DNN_TARGET_CUDA, "DNN_TARGET_CUDA"},
            {cv::dnn::DNN_TARGET_CUDA_FP16, "DNN_TARGET_CUDA_FP16"},
            });

        RegisterPropertyVariable("target", m_nTarget);
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

class cDNNOpenCVPlotFilter : public cOpenCVBaseFilter
{
public:
    ADTF_CLASS_ID_NAME(cDNNOpenCVPlotFilter,
        "dnn_plot.opencv.videotb.cid",
        "DNN Plot Filter");

public:

    cDNNOpenCVPlotFilter()
    {
        SetDescription("Simple DNN Result Plot");
        object_ptr<IStreamType> pStreamType = make_object_ptr<cStreamType>(stream_meta_type_mat());
        m_pDNNPin = CreateInputPin("dnn", pStreamType);
    }

    ~cDNNOpenCVPlotFilter()
    {

    }

    tResult OnStagePreConnect() override
    {
        
        RETURN_NOERROR;
    }

    tResult ProcessInput(ISampleReader* pReader,
        const iobject_ptr<const ISample>& pSample) override
    {
        if (pReader == m_pDNNPin)
        {
            m_pDNNData = pSample;
            RETURN_NOERROR;
        }

        return cOpenCVBaseFilter::ProcessInput(pReader, pSample);
    }

    cv::Mat ProcessMat(const cv::Mat & oMat)
    {
        cv::Mat oResult = oMat;

        if(m_pDNNData)
        {
            auto dnnMat = m_pDNNData->GetMat();

            int w = dnnMat.cols;
            int h = dnnMat.rows;

            float* data = (float*)dnnMat.data;
            for (size_t i = 0; i < dnnMat.total(); i += w)
            {

                float confidence = 0.0;
                size_t classid = 0;

                for (size_t j = 5; j < w; j++)
                {
                    float c = data[i + j] + 0.0;

                    if (confidence < c)
                    {
                        //log.info("Confidence: " + c + " " + j + " " + root.classes[j]);
                        confidence = c;
                        classid = j - 5;
                    }
                }

                if (confidence > 0.2)
                {
                    int center_x = (int) (data[i + 0] * oMat.cols);
                    int center_y = (int) (data[i + 1] * oMat.rows);
                    int width = (int) (data[i + 2] * oMat.cols);
                    int height = (int) (data[i + 3] * oMat.rows);
                    
                    DrawPred(classid, confidence, 
                        center_x - width/2, 
                        center_y - height/2, 
                        center_x + width/2,
                        center_y + height/2,
                        oResult);
                }
            }
        }
        return oResult;
    }

    void DrawPred(int classId, float conf, int left, int top, int right, int bottom, Mat& frame)
    {
        rectangle(frame, Point(left, top), Point(right, bottom), Scalar(0, 255, 0));

        std::string label = format("%.2f", conf);
        if (!lstClasses.empty())
        {
            CV_Assert(classId < (int)lstClasses.size());
            label = lstClasses[classId] + ": " + label;
        }

        int baseLine;
        Size labelSize = getTextSize(label, FONT_HERSHEY_SIMPLEX, 0.5, 1, &baseLine);

        top = max(top, labelSize.height);
        rectangle(frame, Point(left, top - labelSize.height),
            Point(left + labelSize.width, top + baseLine), Scalar::all(255), FILLED);
        putText(frame, label, Point(left, top), FONT_HERSHEY_SIMPLEX, 0.5, Scalar());
    }


private:
    cPinReader* m_pDNNPin;
    object_ptr<const IOpenCVSample> m_pDNNData;

    std::vector<std::string> lstClasses = { "person","bicycle","car","motorbike","aeroplane","bus","train","truck","boat","traffic light","fire hydrant","stop sign","parking meter","bench","bird","cat","dog","horse","sheep","cow","elephant","bear","zebra","giraffe","backpack","umbrella","handbag","tie","suitcase","frisbee","skis","snowboard","sports ball","kite","baseball bat","baseball glove","skateboard","surfboard","tennis racket","bottle","wine glass","cup","fork","knife","spoon","bowl","banana","apple","sandwich","orange","broccoli","carrot","hot dog","pizza","donut","cake","chair","sofa","pottedplant","bed","diningtable","toilet","tvmonitor","laptop","mouse","remote","keyboard","cell phone","microwave","oven","toaster","sink","refrigerator","book","clock","vase","scissors","teddy bear","hair drier","toothbrush" };

};

ADTF_PLUGIN("OpenCV DNN Filter Plugin",
    cDNNOpenCVFilter,
    cDNNOpenCVPlotFilter)