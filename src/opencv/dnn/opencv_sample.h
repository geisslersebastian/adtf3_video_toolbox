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

#include <opencv2/core/mat.hpp>
#include <adtf_streaming3.h>

namespace adtf
{
namespace videotb
{
namespace opencv
{

class IOpenCVSample : public adtf::ucom::IObject
{
public:
    ADTF_IID(IOpenCVSample, "sample.opencv.videotb.iid");

public:
    virtual const cv::Mat & GetMat() const = 0;
};

class cOpenCVSample : public adtf::ucom::object<adtf::streaming::cSample, IOpenCVSample>
{
private: 
    cv::Mat m_oMat;
public: 
    ADTF_CLASS_ID(cOpenCVSample, "sample.opencv.videotb.cid");

public:
    cOpenCVSample(const cv::Mat & oMat) : m_oMat(oMat)
    {

    }

    const cv::Mat & GetMat() const override
    {
        return m_oMat;
    }

public:
    tResult Lock(adtf::ucom::ant::iobject_ptr_shared_locked<const adtf::streaming::ant::ISampleBuffer>& oSampleBuffer) const override;
};

struct stream_meta_type_mat
{
    static constexpr const tChar *const MetaTypeName = "opencv/mat";

    static constexpr const tChar *const FormatName = "format_name";
    static constexpr const tChar *const MaxByteSize = "max_byte_size";
    static constexpr const tChar *const PixelWidth = "pixel_width";
    static constexpr const tChar *const PixelHeight = "pixel_height";
    static constexpr const tChar *const DataEndianess = "data_endianess";

    static tVoid SetProperties(const adtf::ucom::iobject_ptr<adtf::base::IProperties>& pProperties)
    {
        pProperties->SetProperty(adtf::base::property<adtf_util::cString>(FormatName, ""));
        pProperties->SetProperty(adtf::base::property<tUInt32>(MaxByteSize, 0));
        pProperties->SetProperty(adtf::base::property<tUInt>(PixelWidth, 0));
        pProperties->SetProperty(adtf::base::property<tUInt>(PixelHeight, 0));
        pProperties->SetProperty(adtf::base::property<tUInt>(DataEndianess, PLATFORM_BYTEORDER));
    }
};

tResult set_stream_type_mat_format(adtf::streaming::IStreamType& oType, const adtf::streaming::tStreamImageFormat& sFormat);
tResult get_stream_type_mat_format(adtf::streaming::tStreamImageFormat& sFormat, const adtf::streaming::IStreamType& oType);

}
}
}