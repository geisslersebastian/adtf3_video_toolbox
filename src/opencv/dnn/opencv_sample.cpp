#pragma once

#include "opencv_sample.h"
#include <adtf_streaming3.h>

using namespace adtf::ucom;
using namespace adtf::streaming;
using namespace adtf::videotb::opencv;

namespace adtf
{
namespace videotb
{
namespace opencv
{

    class cOpencvSampleBuffer : public adtf::ucom::object<cSharedLockedObject, ISampleBuffer>
    {

    private:

        const cOpenCVSample * m_pSample;

    public:
        cOpencvSampleBuffer(const cOpenCVSample * pSample) :
            m_pSample(pSample)
        {

        }

        virtual tResult Write(const adtf::base::ant::IRawMemory& oBufferWrite)
        {
            RETURN_ERROR(ERR_NOT_IMPL);
        };
        virtual tResult Read(adtf::base::ant::IRawMemory&& oBufferRead) const
        {
            RETURN_ERROR(ERR_NOT_IMPL);
        };
        virtual tVoid*  GetPtr()
        {
            if (m_pSample)
            {
                return m_pSample->GetMat().data;
            }
            return nullptr;
        };
        virtual const tVoid* GetPtr() const
        {
            if (m_pSample)
            {
                return m_pSample->GetMat().data;
            }
            return nullptr;
        }
        virtual tSize   GetSize() const
        {
            if (m_pSample && !m_pSample->GetMat().empty())
            {
                return m_pSample->GetMat().total() * m_pSample->GetMat().elemSize();
            }
            return 0;
        };
        virtual tSize   GetCapacity() const
        {
            return GetSize();
        };
        virtual tResult   Reserve(tSize szSize)
        {
            RETURN_ERROR(ERR_NOT_IMPL);
        };
        virtual tResult   Resize(tSize szSize)
        {
            RETURN_ERROR(ERR_NOT_IMPL);
        };

        tResult Lock() const override
        {
            RETURN_NOERROR;
        }

        tResult Unlock() const override
        {
            RETURN_NOERROR;
        }

        tResult LockShared() const override
        {
            RETURN_NOERROR;
        }

        tResult UnlockShared() const override
        {
            RETURN_NOERROR;
        }
    };

    tResult cOpenCVSample::Lock(adtf::ucom::ant::iobject_ptr_shared_locked<const adtf::streaming::ant::ISampleBuffer>& oSampleBuffer) const
    {
        object_ptr<const ISampleBuffer> pSample = make_object_ptr<cOpencvSampleBuffer>(this);
        oSampleBuffer.Reset(pSample);

        RETURN_NOERROR;
    }

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