#pragma once

#include "gstreamer_reflection.h"

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

        LOG_ERROR("%s", error->message);
        g_error_free(error);

        break;
    }

    case GST_MESSAGE_WARNING:
    {
        gchar  *debug;
        GError *warning;

        gst_message_parse_warning(msg, &warning, &debug);
        g_free(debug);

        LOG_WARNING("%s", warning->message);
        g_error_free(warning);

        break;
    }

    case GST_MESSAGE_INFO:
    {
        gchar  *debug;
        GError *info;

        gst_message_parse_info(msg, &info, &debug);
        g_free(debug);

        LOG_INFO("%s", info->message);
        g_error_free(info);

        break;
    }

    default:
        break;
    }

    return TRUE;
}

static void on_pad_added(GstElement* element, GstPad* pad, gpointer data) {
    GstPad* sinkpad;
    GstElement* decoder = (GstElement*)data;

    LOG_INFO("Pad added %s::%s [linking] %s", gst_pad_get_name(pad), gst_element_get_name(element), gst_element_get_name(decoder));

    sinkpad = gst_element_get_static_pad(decoder, "sink");
    GstPadLinkReturn ret = gst_pad_link(pad, sinkpad);
    if (ret != GST_PAD_LINK_OK)
    {
        LOG_ERROR(" failed [return code]:%d", ret);
    }
    else
    {
        LOG_INFO(" successful");
    }
    gst_object_unref(sinkpad);
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
    property_variable<cString> m_strElementFactory = { "$(THIS_OBJECT_NAME)" };
    property_variable<cString> m_strName = { "$(THIS_OBJECT_NAME)" };
    property_variable<tBool> m_bLastPipeElement = tFalse;

    property_variable<tBool> m_bDynamicPad = tFalse;

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
        RegisterPropertyVariable("dynamic_pad", m_bDynamicPad);

        SetDescription("Use this filter to create one instance of a GStreamer Element.");
    }

    ~cGStreamerBaseFilter()
    {
        if (m_pPipeline)
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
            
            CreateElement();
            InitProperties();
        }
        break;
        case tInitStage::StagePostConnect:
        {
            if (m_bLastPipeElement)
            {
                
                LOG_INFO("--- Create GStreamer Pipeline ---");

                m_pPipeline = gst_pipeline_new("adtf_pipeline");

                RETURN_IF_POINTER_NULL_DESC(m_pPipeline, "Error while creating pipeline");

                auto pBus = gst_pipeline_get_bus(GST_PIPELINE(m_pPipeline));
                gst_bus_add_watch(pBus, bus_call, this);
                gst_object_unref(pBus);

                if (!gst_bin_add(GST_BIN(m_pPipeline), m_pElement))
                {
                    RETURN_ERROR_DESC(ERR_NOT_CONNECTED, "gst_bin_add failed");
                }

                if (m_pGStreamerPipeClient.IsValid())
                {
                    RETURN_IF_FAILED(m_pGStreamerPipeClient->Connect(this, this));
                }

                RETURN_IF_FAILED(InitElement(m_pElement));
                /*if (GST_STATE_CHANGE_SUCCESS != gst_element_set_state(m_pPipeline, GST_STATE_PLAYING))
                {
                    LOG_ERROR("Failed to go to ready");
                    RETURN_ERROR_DESC(ERR_INVALID_STATE, "Failed to go to ready");
                }*/

                LOG_INFO("--- Start Playing Gstreamer Pipeline ---");
                gst_element_set_state(m_pPipeline, GST_STATE_PLAYING);
            }
        }
        break;
        }

        RETURN_NOERROR;
    }

    virtual void CreateElement()
    {
        m_pElement = gst_element_factory_make((*m_strElementFactory).GetPtr(), ("my_" + (*m_strElementFactory)).GetPtr());

        if (!m_pElement)
        {
            THROW_ERROR_DESC(ERR_FAILED, "Could not create GStreamer Element %s ", (*m_strName).GetPtr());
        }
    }

    virtual void Link(cGStreamerBaseFilter * pDestFilter)
    {
        if (!gst_element_link(m_pElement, pDestFilter->m_pElement))
        {
            //RETURN_ERROR_DESC(ERR_NOT_CONNECTED, "gst_element_link failed %s %s", m_strName->GetPtr(), pParentFilter->m_strName->GetPtr());
            LOG_ERROR("gst_element_link failed %s %s", m_strName->GetPtr(), pDestFilter->m_strName->GetPtr());
        }
        else
        {
            LOG_DUMP("gst_element_link success %s %s", m_strName->GetPtr(), pDestFilter->m_strName->GetPtr());
        }
    }

    virtual void InitProperties()
    {
        m_oGStreamerReflection.ParseElement(m_pElement);

        for (auto & oProperty : m_oGStreamerReflection.GetProperties())
        {
            cString strName = ("gst_" + oProperty.first);
            cString strValue = get_property<cString>(*this, strName.GetPtr(), "");

            if (oProperty.first == "name") continue;

            if (strValue == "")
            {
                set_property<cString>(*this, strName.GetPtr(), oProperty.second.value.GetPtr());
            }
            else
            {
                if (oProperty.first == "caps")
                {
                    LOG_INFO("set properties %s from %s = %s", oProperty.first.GetPtr(), m_strName->GetPtr(), strValue.GetPtr());
                    GstCaps *pCaps = gst_caps_from_string(strValue.GetPtr());
                    if (!pCaps)
                    {
                        THROW_ERROR_DESC(ERR_FAILED, "Could not create caps %s", strValue.GetPtr());
                    }
                    LOG_INFO(gst_caps_to_string(pCaps));
                    g_object_set(m_pElement, "caps", pCaps, NULL);
                    gst_caps_unref(pCaps);
                }
                else
                {
                    if (strValue == "true")
                    {
                        g_object_set(m_pElement, oProperty.first, true, NULL);
                    }
                    else if (strValue == "false")
                    {
                        g_object_set(m_pElement, oProperty.first, false, NULL);
                    }
                    else
                    {
                        LOG_INFO("set properties %s from %s = %s", oProperty.first.GetPtr(), m_strName->GetPtr(), strValue.GetPtr());
                    
                        g_object_set(m_pElement, oProperty.first, strValue.GetPtr(), NULL);
                    }
                }
            }
        }
    }


    virtual tResult InitElement(GstElement* pElement)
    {
        RETURN_NOERROR;
    }

    virtual tResult AddGStreamerFilter(cGStreamerBaseFilter * pParentFilter, cGStreamerBaseFilter * pRootFilter)
    {
        RETURN_IF_FAILED(InitElement(m_pElement));

        if (!gst_bin_add(GST_BIN(pRootFilter->m_pPipeline), m_pElement))
        {
            RETURN_ERROR_DESC(ERR_NOT_CONNECTED, "gst_bin_add failed");
        }

        if (m_pGStreamerPipeClient.IsValid())
        {
            RETURN_IF_FAILED(m_pGStreamerPipeClient->Connect(this, pRootFilter));
        }

        if (pParentFilter->m_pElement && m_pElement)
        {
            if (m_bDynamicPad)
            {
                LOG_DUMP("Signal pad-added to %s", this->m_strName->GetPtr());
                g_signal_connect(m_pElement, "pad-added", G_CALLBACK(on_pad_added), pParentFilter->m_pElement);
            }
            else
            {
                Link(pParentFilter);
            }
        }

        RETURN_NOERROR;
    }

private:
    interface_client<IGStreamerPipe> m_oInterfaceClient;
};