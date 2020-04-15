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

#include <adtfstreaming3/sample_serialization_intf.h>

class cAppSinkFilter : public cGStreamerBaseFilter
{
public:
    ADTF_CLASS_ID_NAME(cAppSinkFilter,
        "app_sink_filter.gstreamer.videotb.cid",
        "AppSink GStreamer Filter");
    ADTF_CLASS_DEPENDENCIES(REQUIRE_INTERFACE(IGStreamerPipe));

    tStreamImageFormat m_sFormat;
    ISampleWriter* m_pWriter;
    object_ptr<adtf::services::IReferenceClock> m_pClock;

public:
    cAppSinkFilter()
    {
        m_sFormat.m_strFormatName = ADTF_IMAGE_FORMAT(RGB_24);
        m_sFormat.m_ui32Width = 1024;
        m_sFormat.m_ui32Height = 726;
        m_sFormat.m_szMaxByteSize = 1024 * 726 * 3;
        m_sFormat.m_ui8DataEndianess = PLATFORM_BYTEORDER;
        m_bLastPipeElement = tTrue;
        object_ptr<IStreamType> pType = make_object_ptr<cStreamType>(stream_meta_type_image());
        set_stream_type_image_format(*pType, m_sFormat);
        m_pWriter = CreateOutputPin("outpin", pType);

        THROW_IF_FAILED(_runtime->GetObject(m_pClock));
    }

    tResult InitElement(GstElement* pElement) override;

    tResult SendData(void* pData, tUInt32 nSize)
    {
        auto tmNow = m_pClock->GetStreamTimeNs();

        object_ptr<ISample> pSample;
        if (IS_OK(alloc_sample(pSample, tmNow)))
        {
            object_ptr_locked<ISampleBuffer> pBuffer;
            if (IS_OK(pSample->WriteLock(pBuffer, nSize)))
            {
                adtf_util::cMemoryBlock::MemCopy(pBuffer->GetPtr(), pData, pBuffer->GetSize());
            }

            m_pWriter->Write(pSample);
            m_pWriter->ManualTrigger(tmNow);
        }

        RETURN_NOERROR;
    }

    tResult SampleType(const cString & strName, const std::map<adtf::util::cString, adtf::util::cVariant> & oProperties)
    {
        if (strName == "video/x-h264")
        {
            auto pAdtfStreamtype = make_object_ptr<adtf::streaming::cCamelionStreamType>(oStreamType.getMetaTypeName().c_str());
            object_ptr<IProperties> pProperties;
            pAdtfStreamtype->GetConfig(pProperties);

            for (auto strPropertyName : oStreamType.getPropertyNames())
            {
                auto strType = oStreamType.getPropertyType(strPropertyName);
                auto strValue = oStreamType.getProperty(strPropertyName);

                {
                    adtf::base::set_property<cString>(*pProperties.Get(), strPropertyName.c_str(), strValue.c_str());
                }
            }
        }
        else
        {
            int nWidth = oProperties.at("width").GetInt32();
            int nHeight = oProperties.at("height").GetInt32();

            /*SampleType(nWidth, nHeight,
                static_cast<tInt32>(3),
                static_cast<tInt32>(nSize / nHeight));*/
        }
        RETURN_NOERROR;
    }

    tResult SampleType(tInt32 nWidth, tInt32 nHeight, tInt32 nDeep, tInt32 nPitch)
    {
        if (m_sFormat.m_ui32Width != nWidth || m_sFormat.m_ui32Height != nHeight)
        {
            m_sFormat.m_ui32Width = nWidth;
            m_sFormat.m_ui32Height = nHeight;

            if (nDeep == 8)
            {
                m_sFormat.m_strFormatName = ADTF_IMAGE_FORMAT(GREYSCALE_8);
                m_sFormat.m_szMaxByteSize = nWidth * nHeight;
            }
            else if (nDeep == 12)
            {
                m_sFormat.m_strFormatName = ADTF_IMAGE_FORMAT(YUV420P);
                m_sFormat.m_szMaxByteSize = (nWidth * nHeight * 12) / 8;
            }
            else if (nDeep == 24)
            {
                m_sFormat.m_strFormatName = ADTF_IMAGE_FORMAT(RGB_24);
                m_sFormat.m_szMaxByteSize = nWidth * nHeight * 3;
            }
            else if (nDeep == 32)
            {
                m_sFormat.m_strFormatName = ADTF_IMAGE_FORMAT(RGBA_32);
                m_sFormat.m_szMaxByteSize = nWidth * nHeight * 4;
            }

            m_sFormat.m_ui8DataEndianess = PLATFORM_BYTEORDER;
            object_ptr<IStreamType> pType = make_object_ptr<cStreamType>(stream_meta_type_image());
            set_stream_type_image_format(*pType, m_sFormat);

            m_pWriter->ChangeType(pType);
        }
        RETURN_NOERROR;
    }
};

gboolean foreach(GQuark field_id,
    const GValue * value,
    gpointer user_data)
{
    std::map<cString, cVariant>* pMap = static_cast<std::map<cString, cVariant>*>(user_data);
    gchar *str = gst_value_serialize(value);
    (*pMap)[g_quark_to_string(field_id)] = str;
    g_free(str);
    return true;
}

std::map<cString, cVariant> GetProperties(GstStructure* pCapsStruct)
{
    std::map<cString, cVariant> oMap;
    gst_structure_foreach(pCapsStruct, foreach, &oMap);
    return oMap;
}

/* The appsink has received a buffer */
GstFlowReturn new_sample(GstElement* pSink, cAppSinkFilter* pFilter) {

    GstSample *pSample;
    /* Retrieve the buffer */
    g_signal_emit_by_name(pSink, "pull-sample", &pSample);
    if (pSample)
    {
        /* The only thing we do in this example is print a * to indicate a received buffer */
        GstBuffer * pBuffer = gst_sample_get_buffer(pSample);
        if (!pBuffer)
        {
            LOG_ERROR("gst_sample_get_buffer() returned NULL");
            return GST_FLOW_OK;
        }

        GstMapInfo oMap;

        if (!gst_buffer_map(pBuffer, &oMap, GST_MAP_READ))
        {
            LOG_ERROR("gst_buffer_map() failed");
            return GST_FLOW_OK;
        }

        void* pData = oMap.data; //GST_BUFFER_DATA(gstBuffer);
        const gsize nSize = oMap.size; //GST_BUFFER_SIZE(gstBuffer);

        if (!pData)
        {
            LOG_ERROR("gst_buffer had NULL data pointer");
            return GST_FLOW_OK;
        }

        // retrieve caps
        GstCaps* pCaps = gst_sample_get_caps(pSample);

        if (!pCaps)
        {
            LOG_ERROR("gst_buffer had NULL caps");
            return GST_FLOW_OK;
        }

        GstStructure* pCapsStruct = gst_caps_get_structure(pCaps, 0);

        if (!pCapsStruct)
        {
            LOG_ERROR("gst_caps had NULL structure");
            return GST_FLOW_OK;
        }

        LOG_INFO("Struct %s", gst_structure_get_name(pCapsStruct));

        
        /*
        // get width & height of the buffer
        int nWidth = 0;
        int nHeight = 0;

        //v_framerate = gst_structure_get_value(structure, "framerate");
        //v_par = gst_structure_get_value(structure, "pixel-aspect-ratio");

        auto capsString = gst_structure_to_string(pCapsStruct);

        LOG_INFO(capsString);

        if (!gst_structure_get_int(pCapsStruct, "width", &nWidth) ||
            !gst_structure_get_int(pCapsStruct, "height", &nHeight))
        {
            LOG_ERROR("gst_caps missing width/height");
            return GST_FLOW_OK;
        }

        if (nWidth < 1 || nHeight < 1)
        {
            return GST_FLOW_OK;
        }
        */

        auto oProperties = GetProperties(pCapsStruct);
        pFilter->SampleType(gst_structure_get_name(pCapsStruct), oProperties);

        /*pFilter->SampleType(nWidth, nHeight, 
            static_cast<tInt32>((nSize * 8) / (nWidth * nHeight)), 
            static_cast<tInt32>(nSize / nHeight));*/
        pFilter->SendData(pData, static_cast<tInt32>(nSize));

        //@TODO Make sure all return are memory leak free
        gst_buffer_unmap(pBuffer, &oMap);
        //gst_buffer_unref(pBuffer);
        gst_sample_unref(pSample);
    }
    return GST_FLOW_OK;
}

tResult cAppSinkFilter::InitElement(GstElement* pElement)
{
    g_object_set(m_pElement, "emit-signals", TRUE, NULL);
    g_signal_connect(m_pElement, "new-sample", G_CALLBACK(new_sample), this);
    RETURN_NOERROR;
}