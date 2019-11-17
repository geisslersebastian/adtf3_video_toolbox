#pragma once

#include <opencv2/core/mat.hpp>
#include <adtfstreaming3/streamtype_intf.h>

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

tResult set_stream_type_mat_format(adtf::streaming::IStreamType& oType, const adtf::streaming::tStreamImageFormat& sFormat)
{
    adtf_util::cString strMetaType;
    oType.GetMetaTypeName(adtf_string_intf(strMetaType));
    if (strMetaType == stream_meta_type_mat::MetaTypeName)
    {
        adtf::streaming::set_property<adtf_util::cString>(oType, stream_meta_type_mat::FormatName, sFormat.m_strFormatName);
        adtf::streaming::set_property<tInt32>(oType, stream_meta_type_mat::MaxByteSize, static_cast<tInt32>(sFormat.m_szMaxByteSize));
        adtf::streaming::set_property<tUInt>(oType, stream_meta_type_mat::PixelWidth, sFormat.m_ui32Width);
        adtf::streaming::set_property<tUInt>(oType, stream_meta_type_mat::PixelHeight, sFormat.m_ui32Height);
        adtf::streaming::set_property<tUInt>(oType, stream_meta_type_mat::DataEndianess, sFormat.m_ui8DataEndianess);
        RETURN_NOERROR;
    }
    else
    {
        RETURN_ERROR(ERR_INVALID_TYPE);
    }
}

tResult get_stream_type_mat_format(adtf::streaming::tStreamImageFormat& sFormat, const adtf::streaming::IStreamType& oType)
{
    adtf_util::cString strMetaType;
    oType.GetMetaTypeName(adtf_string_intf(strMetaType));
    if (strMetaType == stream_meta_type_mat::MetaTypeName)
    {
        sFormat.m_strFormatName = adtf::streaming::get_property<adtf_util::cString>(oType, stream_meta_type_mat::FormatName, "");
        sFormat.m_szMaxByteSize = adtf::streaming::get_property<tInt32>(oType, stream_meta_type_mat::MaxByteSize, 0);
        sFormat.m_ui32Width = adtf::streaming::get_property<tUInt>(oType, stream_meta_type_mat::PixelWidth, 0);
        sFormat.m_ui32Height = adtf::streaming::get_property<tUInt>(oType, stream_meta_type_mat::PixelHeight, 0);
        sFormat.m_ui8DataEndianess = adtf::streaming::get_property<tUInt>(oType, stream_meta_type_mat::DataEndianess, 0);
        RETURN_NOERROR;
    }
    else
    {
        RETURN_ERROR(ERR_INVALID_TYPE);
    }
}

}
}
}