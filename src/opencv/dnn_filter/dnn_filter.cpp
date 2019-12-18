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

ADTF_PLUGIN("OpenCV DNN Filter Plugin",
    cDNNOpenCVFilter)