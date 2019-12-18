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
 
#include <opencv_base_filter/opencv_sample.h>
#include <opencv_base_filter/opencv_base_filter.h>

using namespace adtf::util;
using namespace adtf::ucom;
using namespace adtf::base;
using namespace adtf::streaming;
using namespace adtf::filter;

using namespace cv::dnn;
using namespace cv;

using namespace adtf::videotb::opencv;

namespace adtf
{
namespace videotb
{
namespace opencv
{

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

cOpenCVBaseFilter::cOpenCVBaseFilter()
{
    object_ptr<IStreamType> pStreamType = make_object_ptr<cStreamType>(stream_meta_type_mat());
    m_pOutput = CreateOutputPin("mat_out", pStreamType);
    m_pInput = CreateInputPin("mat_in", pStreamType);
    m_pInput->SetAcceptTypeCallback([this](const auto & pStreamType) -> tResult
    {
        RETURN_IF_FAILED(get_stream_type_mat_format(m_sCurrentFormat, *pStreamType.Get()));
        m_sCurrentFormat = this->ConvertImageFormat(m_sCurrentFormat);
        object_ptr<IStreamType> pImageStreamType = make_object_ptr<cStreamType>(stream_meta_type_mat());
        RETURN_IF_FAILED(set_stream_type_mat_format(*pImageStreamType.Get(), m_sCurrentFormat));
        RETURN_IF_FAILED(m_pOutput->ChangeType(pImageStreamType));
        RETURN_NOERROR;
    });
}

tStreamImageFormat cOpenCVBaseFilter::ConvertImageFormat(const tStreamImageFormat & oImageFormat)
{
    return oImageFormat;
}

tResult cOpenCVBaseFilter::ProcessInput(ISampleReader* pReader,
    const iobject_ptr<const ISample>& pSample)
{
    object_ptr<const IOpenCVSample> pMatSample = pSample;
    if (pMatSample)
    {
        cv::Mat oMat = ProcessMat(pMatSample->GetMat());
        if (!oMat.empty())
        {
            object_ptr<ISample> pSample = make_object_ptr<cOpenCVSample>(oMat);
            m_pOutput->Write(pSample);
        }
    }
    RETURN_NOERROR;
}

tResult cOpenCVBaseFilter::Init(tInitStage eStage)
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

}
}
}