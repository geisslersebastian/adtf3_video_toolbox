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

#include <gst/app/gstappsrc.h>

class cAppSourceFilter : public cGStreamerBaseFilter
{
public:
    ADTF_CLASS_ID_NAME(cAppSourceFilter,
        "app_source_filter.gstreamer.videotb.cid",
        "AppSource GStreamer Filter");
    ADTF_CLASS_DEPENDENCIES(REQUIRE_INTERFACE(IGStreamerPipe));

    tStreamImageFormat m_sFormat;
    cPinReader* m_pReader;

    tUInt32 m_nChannel = 1;
    GstElement* m_pCapsFilter = nullptr;

    property_variable<tBool> m_bBinary = tFalse;

    
public:
    cAppSourceFilter()
    {
        m_sFormat.m_strFormatName = ADTF_IMAGE_FORMAT(RGB_24);
        m_sFormat.m_ui32Width = 640;
        m_sFormat.m_ui32Height = 480;
        m_sFormat.m_szMaxByteSize = m_sFormat.m_ui32Width * m_sFormat.m_ui32Height * 3;
        m_sFormat.m_ui8DataEndianess = PLATFORM_BYTEORDER;

        object_ptr<IStreamType> pType = make_object_ptr<cStreamType>(stream_meta_type_image());
        set_stream_type_image_format(*pType, m_sFormat);
        m_pReader = CreateInputPin("input", pType);
        m_pReader->SetAcceptTypeCallback([&](const adtf::ucom::iobject_ptr<const IStreamType>& pStreamType)
        {
            return InitStreamType(pStreamType);
        });

        RegisterPropertyVariable("binary", m_bBinary);
    }

    void CreateElement() override
    {
        m_pElement = gst_element_factory_make("appsrc", ("my_" + (*m_strName)).GetPtr());
        if (!m_pElement)
        {
            THROW_ERROR_DESC(ERR_FAILED, "Could not create GStreamer Element %s ", (*m_strName).GetPtr());
        }
        
        if (m_bBinary)
        {
            m_pCapsFilter = gst_element_factory_make("capsfilter", "appsrccaps");
            if (!m_pCapsFilter)
            {
                THROW_ERROR_DESC(ERR_FAILED, "Could not create GStreamer Element capsfilter");
            }
        }
    }

    tResult ProcessInput(adtf::streaming::flash::ISampleReader* pReader,
        const adtf::ucom::iobject_ptr<const adtf::streaming::ISample>& pSample)
    {
        adtf::ucom::object_ptr_shared_locked<const adtf::streaming::ISampleBuffer> pSampleBuffer;
        if (IS_OK(pSample->Lock(pSampleBuffer)))
        {
            tInt32 nSize = m_sFormat.m_ui32Width * m_sFormat.m_ui32Height * m_nChannel;
            auto pBuffer = gst_buffer_new_allocate(NULL, nSize, NULL);

            gst_buffer_fill(pBuffer, 0, pSampleBuffer->GetPtr(), pSampleBuffer->GetSize());

            GstFlowReturn oResult = gst_app_src_push_buffer(GST_APP_SRC(m_pElement), pBuffer);
            if (oResult != GST_FLOW_OK)
            {
                LOG_ERROR("App src send sample failed: %s", gst_flow_get_name(oResult));
            }
        }
        RETURN_NOERROR;
    }

    tResult AddGStreamerFilter(cGStreamerBaseFilter * pParentFilter, cGStreamerBaseFilter * pRootFilter) override
    {
        RETURN_IF_FAILED(InitElement(m_pElement));

        if (!gst_bin_add(GST_BIN(pRootFilter->m_pPipeline), m_pElement))
        {
            RETURN_ERROR_DESC(ERR_NOT_CONNECTED, "gst_bin_add failed");
        }

        if(m_pCapsFilter)
        {
            if (!gst_bin_add(GST_BIN(pRootFilter->m_pPipeline), m_pCapsFilter))
            {
                RETURN_ERROR_DESC(ERR_NOT_CONNECTED, "gst_bin_add failed");
            }
        }

        if (m_pGStreamerPipeClient.IsValid())
        {
            RETURN_IF_FAILED(m_pGStreamerPipeClient->Connect(this, pRootFilter));
        }

        if (pParentFilter->m_pElement && m_pElement)
        {
            if (!m_bDynamicPad)
            {
                if (!gst_element_link(m_pElement, m_pCapsFilter ? m_pCapsFilter : pParentFilter->m_pElement))
                {
                    LOG_ERROR("gst_element_link failed app_sink -> capsfilter");
                }
                else
                {
                    LOG_DUMP("gst_element_link success %s %s", m_strName->GetPtr(), pParentFilter->m_strName->GetPtr());
                }

                if(m_pCapsFilter)
                {
                    if (!gst_element_link(m_pCapsFilter, pParentFilter->m_pElement))
                    {
                        LOG_ERROR("gst_element_link failed %s %s", m_strName->GetPtr(), pParentFilter->m_strName->GetPtr());
                    }
                    else
                    {
                        LOG_DUMP("gst_element_link success %s %s", m_strName->GetPtr(), pParentFilter->m_strName->GetPtr());
                    }
                }
            }
            else
            {
                LOG_DUMP("Signal pad-added to %s", this->m_strName->GetPtr());
                g_signal_connect(m_pElement, "pad-added", G_CALLBACK(on_pad_added), pParentFilter->m_pElement);
            }
        }

        RETURN_NOERROR;
    }

    tResult InitStreamType(const adtf::ucom::iobject_ptr<const IStreamType>& pStreamType)
    {
        if (IS_OK(get_stream_type_image_format(m_sFormat, *pStreamType.Get())))
        {
            if (m_sFormat.m_strFormatName == ADTF_IMAGE_FORMAT(GREYSCALE_8))
            {
                m_nChannel = 1;
            }
            else if (m_sFormat.m_strFormatName == ADTF_IMAGE_FORMAT(YUV420P))
            {
                //strFormat = 
            }
            else if (m_sFormat.m_strFormatName == ADTF_IMAGE_FORMAT(RGB_24))
            {
                m_nChannel = 3;
            }
            else if (m_sFormat.m_strFormatName == ADTF_IMAGE_FORMAT(RGBA_32))
            {
                m_nChannel = 4;
            }

            tInt32 nSize = m_sFormat.m_ui32Width * m_sFormat.m_ui32Height * m_nChannel;
            g_object_set(G_OBJECT(m_pElement), "blocksize", nSize,
                NULL);

            g_object_set(G_OBJECT(m_pElement), "caps", StreamTypeToCap(m_sFormat), NULL);
        }
        else
        {
            g_object_set(G_OBJECT(m_pElement), "caps", StreamTypeToCap(pStreamType), NULL);
        }

        RETURN_NOERROR;
    }

    tResult InitElement(GstElement* pElement) override
    {
        //tInt32 nSize = m_sFormat.m_ui32Width * m_sFormat.m_ui32Height * m_nChannel;
        /*g_object_set(G_OBJECT(pElement), "blocksize", nSize,
            NULL);*/

        g_object_set(G_OBJECT(pElement), "do-timestamp", true, NULL);
        g_object_set(G_OBJECT(pElement), "format", GST_FORMAT_TIME, NULL);
        g_object_set(G_OBJECT(pElement), "caps", StreamTypeToCap(m_sFormat), NULL);
        RETURN_NOERROR;
    }

    GstCaps * StreamTypeToCap(const adtf::ucom::iobject_ptr<const IStreamType>& pStreamType)
    {
        cString strMetaType;
        THROW_IF_FAILED_DESC(pStreamType->GetMetaTypeName(adtf_string_intf(strMetaType)), "Failing get meta type");

        object_ptr<const IProperties> pProperties;
        if (IS_OK(pStreamType->GetConfig(pProperties)))
        {
            cString strGstStructure = adtf::base::get_property<cString>(*pProperties, "gst_structure");

            GstStructure* pGstStructure = gst_structure_from_string(strGstStructure.GetPtr(), NULL);

            return gst_caps_new_full(pGstStructure, NULL);
        }

        THROW_ERROR_DESC(ERR_UNEXPECTED, "Streamtype has no properties");
    }

    GstCaps * StreamTypeToCap(const tStreamImageFormat & sFormat)
    {
        std::string strFormat;
        if (sFormat.m_strFormatName == ADTF_IMAGE_FORMAT(GREYSCALE_8))
        {
            strFormat = "GRAY8";
        }
        /*else if (sFormat.m_strFormatName == ADTF_IMAGE_FORMAT(YUV420P))
        {
            //strFormat = 
        }*/
        else if (sFormat.m_strFormatName == ADTF_IMAGE_FORMAT(RGB_24))
        {
            strFormat = "RGB";
        }
        else if (sFormat.m_strFormatName == ADTF_IMAGE_FORMAT(RGBA_32))
        {
            strFormat = "RGBA";
        }
        else
        {
            THROW_ERROR_DESC(ERR_NOT_SUPPORTED, "The image fromat %s is not supported", sFormat.m_strFormatName.GetPtr());
        }

        auto pCaps = gst_caps_new_simple("video/x-raw",
            "format", G_TYPE_STRING, strFormat.c_str(),
            "framerate", GST_TYPE_FRACTION, 25, 1,
            "pixel-aspect-ratio", GST_TYPE_FRACTION, 1, 1,
            "width", G_TYPE_INT, sFormat.m_ui32Width,
            "height", G_TYPE_INT, sFormat.m_ui32Height,
            NULL);
        LOG_INFO(gst_caps_to_string(pCaps));
        return pCaps;
    }
};
