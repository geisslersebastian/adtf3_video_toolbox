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
#include <gst/gst.h>

#include "gstreamer_reflection.h"

using namespace adtf::util;
using namespace adtf::ucom;
using namespace adtf::base;
using namespace adtf::streaming;
using namespace adtf::filter;
using namespace adtf::services;

static gboolean bus_call(GstBus *bus, GstMessage *msg, gpointer data)
{
    switch (GST_MESSAGE_TYPE(msg)) 
    {

        case GST_MESSAGE_EOS:
            LOG_INFO("End of stream\n");
            break;

        case GST_MESSAGE_ERROR: 
        {
            gchar  *debug;
            GError *error;

            gst_message_parse_error(msg, &error, &debug);
            g_free(debug);

            LOG_ERROR("Error: %s\n", error->message);
            g_error_free(error);

            break;
        }
        default:
            break;
    }

    return TRUE;
}

class cGStreamerBaseFilter;

class IGStreamerPipe : public IObject
{
public:
    ADTF_IID(IGStreamerPipe, "gstreamer_pipe.gstreamer.adtf.iid");

public:
    virtual tResult Connect(cGStreamerBaseFilter * pParentFilter, cGStreamerBaseFilter * pRootFilter) = 0;
};

class cGStreamerBaseFilter : public cFilter
{
public:
    property_variable<cString> m_strElementFactory = { "$(THIS_OBJECT_NAME)"};
    property_variable<cString> m_strName = { "$(THIS_OBJECT_NAME)" };
    property_variable<tBool> m_bLastPipeElement = tFalse;

    GstElement* m_pElement = nullptr;
    GstElement* m_pPipeline = nullptr;

    cGStreamerReflection m_oGStreamerReflection;
    
    object_ptr<IGStreamerPipe> m_pGStreamerPipeServer;
    interface_client<IGStreamerPipe> m_pGStreamerPipeClient;
    

public:

    class cGStreamerPipe : public adtf::ucom::object<IGStreamerPipe>
    {

    public:
        cGStreamerBaseFilter* m_pGStreamerFilter;
        

    public:
        cGStreamerPipe(cGStreamerBaseFilter* pGStreamerFilter) : m_pGStreamerFilter(pGStreamerFilter)
        {

        }

        tResult Connect(cGStreamerBaseFilter * pParentFilter, cGStreamerBaseFilter * pRootFilter)
        {
            return m_pGStreamerFilter->AddGStreamerFilter(pParentFilter, pRootFilter);
        }
    };

public:
    
    cGStreamerBaseFilter()
    {
        m_pGStreamerPipeClient = CreateInterfaceClient<IGStreamerPipe>("in");
        m_pGStreamerPipeServer = make_object_ptr<cGStreamerPipe>(this);
        CreateInterfaceServer("out", m_pGStreamerPipeServer);

        m_strElementFactory.SetDescription("Name of the GStreamer Factory");
        RegisterPropertyVariable("element_factory", m_strElementFactory);
        RegisterPropertyVariable("name", m_strName);

        RegisterPropertyVariable("last_pipeline_element", m_bLastPipeElement);

        SetDescription("Use this filter to create one instance of a GStreamer Element.");
    }
    
    ~cGStreamerBaseFilter()
    {
        if(m_pPipeline)
        {
            gst_element_set_state(m_pPipeline, GST_STATE_NULL);
            gst_object_unref(GST_OBJECT(m_pPipeline));
        }
    }

    tResult Init(tInitStage eStage)
    {
        RETURN_IF_FAILED(cFilter::Init(eStage));

        switch (eStage)
        {
            case tInitStage::StageFirst:
            {

            }
            break;
            case tInitStage::StagePreConnect:
            {
                m_pElement = gst_element_factory_make((*m_strElementFactory).GetPtr(), ("my_"+(*m_strElementFactory)).GetPtr());
                
                RETURN_IF_POINTER_NULL_DESC(m_pElement, "Could not create GStreamer Element %s ", (*m_strName).GetPtr());
                
                RETURN_IF_FAILED(InitProperties());

                /*GstElementFactory * pFactory = gst_element_factory_find((*m_strElementFactory).GetPtr());

                gchar** strKeys = gst_element_factory_get_metadata_keys(pFactory);

                tInt nIdx = 0;
                while (strKeys && strKeys[nIdx] != 0)
                {
                    cString strKey = strKeys[nIdx++];
                    cString strValue = gst_element_factory_get_metadata(pFactory, strKey.GetPtr());
                    LOG_INFO("Keys: %s = %s", strKey.GetPtr(), strValue.GetPtr());
                }*/

                RETURN_IF_FAILED(InitGStreamerElement(m_pElement));

            }
            break;
            case tInitStage::StagePostConnect:
            {
                if(m_bLastPipeElement)
                {
                    if(m_pGStreamerPipeClient.IsValid())
                    {
                        LOG_INFO("Create GStreamer Pipeline");

                        m_pPipeline = gst_pipeline_new("adtf_pipeline");

                        RETURN_IF_POINTER_NULL_DESC(m_pPipeline, "Error while creating pipeline");

                        auto pBus = gst_pipeline_get_bus(GST_PIPELINE(m_pPipeline));
                        gst_bus_add_watch(pBus, bus_call, this);
                        gst_object_unref(pBus);

                        if (!gst_bin_add(GST_BIN(m_pPipeline), m_pElement))
                        {
                            RETURN_ERROR_DESC(ERR_NOT_CONNECTED, "gst_bin_add failed");
                        }

                        RETURN_IF_FAILED(m_pGStreamerPipeClient->Connect(this, this));
                       
                        gst_element_set_state(m_pPipeline, GST_STATE_PLAYING);
                    }
                }
            }
            break;
        }

        RETURN_NOERROR;
    }

    tResult InitProperties()
    {
        m_oGStreamerReflection.ParseElement(m_pElement);

        for (auto & oProperty : m_oGStreamerReflection.GetProperties())
        {
            cString strName = ("element_" + oProperty.first);
            cString strValue = get_property<cString>(*this, strName.GetPtr(), "");

            if (oProperty.first == "name") continue;

            if (strValue == "")
            {
                set_property<cString>(*this, strName.GetPtr(), oProperty.second.value.GetPtr());
            }
            else
            {
                if(oProperty.first == "caps")
                { 
                    LOG_DUMP("set properties %s from %s = %s", oProperty.first.GetPtr(), m_strName->GetPtr(), strValue.GetPtr());
                    GstCaps *pCaps = gst_caps_from_string(strValue.GetPtr());
                    RETURN_IF_POINTER_NULL_DESC(pCaps, "Could not create caps %s", strValue.GetPtr());
                    g_object_set(m_pElement, "caps", pCaps, NULL);
                    gst_caps_unref(pCaps);
                }
            }
        }
        RETURN_NOERROR;
    }

    virtual tResult InitGStreamerElement(GstElement* pElement)
    {
        
        RETURN_NOERROR;
    }

    tResult AddGStreamerFilter(cGStreamerBaseFilter * pParentFilter, cGStreamerBaseFilter * pRootFilter)
    {
        if (!gst_bin_add(GST_BIN(pRootFilter->m_pPipeline), m_pElement))
        {
            RETURN_ERROR_DESC(ERR_NOT_CONNECTED, "gst_bin_add failed");
        }

        if (m_pGStreamerPipeClient.IsValid())
        {
            RETURN_IF_FAILED(m_pGStreamerPipeClient->Connect(this, pRootFilter));
        }

        LOG_DUMP("gst_element_link failed %s %s", m_strName->GetPtr(), pParentFilter->m_strName->GetPtr());

        if (pParentFilter->m_pElement && m_pElement)
        {
            if (!gst_element_link(m_pElement, pParentFilter->m_pElement))
            {
                //RETURN_ERROR_DESC(ERR_NOT_CONNECTED, "gst_element_link failed %s %s", m_strName->GetPtr(), pParentFilter->m_strName->GetPtr());
                LOG_ERROR("gst_element_link failed %s %s", m_strName->GetPtr(), pParentFilter->m_strName->GetPtr());
            }
        }

        RETURN_NOERROR;
    }

private:
    interface_client<IGStreamerPipe> m_oInterfaceClient;
};

class cGStreamerFilter : public cGStreamerBaseFilter
{
public:
    ADTF_CLASS_ID_NAME(cGStreamerFilter,
        "gstreamerfilter.gstreamer.videotb.cid",
        "GStreamer Filter");
    ADTF_CLASS_DEPENDENCIES(REQUIRE_INTERFACE(IGStreamerPipe));
};

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
        object_ptr<IStreamType> pType = make_object_ptr<cStreamType>(stream_meta_type_image());
        set_stream_type_image_format(*pType, m_sFormat);
        m_pWriter = CreateOutputPin("outpin", pType);

        THROW_IF_FAILED(_runtime->GetObject(m_pClock));
    }

    tResult InitGStreamerElement(GstElement* pElement) override;

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

    tResult SampleType(tInt32 nWidth, tInt32 nHeight, tInt32 nDeep, tInt32 nPitch)
    {
        if(m_sFormat.m_ui32Width != nWidth || m_sFormat.m_ui32Height != nHeight)
        {
            m_sFormat.m_ui32Width = nWidth;
            m_sFormat.m_ui32Height = nHeight;

            if(nDeep == 8)
            {
                m_sFormat.m_strFormatName = ADTF_IMAGE_FORMAT(GREYSCALE_8);
                m_sFormat.m_szMaxByteSize = nWidth * nHeight;
            }
            else if (nDeep == 12)
            {
                m_sFormat.m_strFormatName = ADTF_IMAGE_FORMAT(YUV420P);
                m_sFormat.m_szMaxByteSize = (nWidth * nHeight * 12) / 8;
            }
            else if(nDeep == 24)
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

/* The appsink has received a buffer */
GstFlowReturn new_sample(GstElement* pSink, cAppSinkFilter* pFilter) {

    GstSample *pSample;
    /* Retrieve the buffer */
    g_signal_emit_by_name(pSink, "pull-sample", &pSample);
    if (pSample) {
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
        const uint32_t nSize = oMap.size; //GST_BUFFER_SIZE(gstBuffer);

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

        // get width & height of the buffer
        int nWidth = 0;
        int nHeight = 0;

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

        pFilter->SampleType(nWidth, nHeight, (nSize * 8) / (nWidth * nHeight), nSize / nHeight);
        pFilter->SendData(pData, nSize);
        //gst_buffer_unref(pBuffer);
        gst_sample_unref(pSample);
    }
    return GST_FLOW_OK;
}

tResult cAppSinkFilter::InitGStreamerElement(GstElement* pElement)
{
    g_object_set(m_pElement, "emit-signals", TRUE, NULL);
    g_signal_connect(m_pElement, "new-sample", G_CALLBACK(new_sample), this);
    RETURN_NOERROR;
}

#include <adtf_systemsdk.h>
using namespace adtf::system;

class cGStreamerService : public cADTFService
{
public:
    ADTF_CLASS_ID_NAME(cGStreamerService, 
        "gstreamerservice.gstreamer.videotb.cid",
        "GStreamer Service");

    ADTF_CLASS_DEPENDENCIES(
        PROVIDE_INTERFACE(IGStreamerPipe));

    cGStreamerService()
    {
        SetDefaultRunlevel(tADTFRunLevel::RL_Session);
        SetDescription("Use this System Service to print signal updates (e.g. send by 'Demo Signal Provider', see Filters.");
    }

public: // overrides cService

    virtual tResult ServiceInit() override 
    {
        GError* vGErr = NULL;
        if (!gst_init_check(NULL, NULL, &vGErr))
        {
            RETURN_ERROR_DESC(ERR_NOT_INITIALIZED, "Can not initialize GStreamer.");
        }

        m_pMainLoop = g_main_loop_new(NULL, FALSE);

        RETURN_NOERROR;
    }
    virtual tResult ServiceShutdown() override
    {
        g_main_loop_unref(m_pMainLoop);
        RETURN_NOERROR;
    }
    
private:
    GMainLoop* m_pMainLoop;
};

ADTF_PLUGIN("GStreamer Filter", cGStreamerFilter, cAppSinkFilter, cGStreamerService)