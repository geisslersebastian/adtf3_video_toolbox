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

using namespace adtf::util;
using namespace adtf::ucom;
using namespace adtf::base;
using namespace adtf::streaming;
using namespace adtf::filter;
using namespace adtf::services;

#include <gst/gst.h>

#include "gstreamer_base.h"
#include "gstreamer_appsink.h"
#include "gstreamer_appsource.h"
#include "gstreamer_service.h"

class cGStreamerFilter : public cGStreamerBaseFilter
{
public:
    ADTF_CLASS_ID_NAME(cGStreamerFilter,
        "gstreamerfilter.gstreamer.videotb.cid",
        "GStreamer Filter");
    ADTF_CLASS_DEPENDENCIES(REQUIRE_INTERFACE(IGStreamerPipe));
};

class cGStreamerLauncherFilter : public cGStreamerBaseFilter
{
public:
    ADTF_CLASS_ID_NAME(cGStreamerLauncherFilter,
        "gstreamerlauncherfilter.gstreamer.videotb.cid",
        "GStreamer Launcher Filter");
    ADTF_CLASS_DEPENDENCIES(REQUIRE_INTERFACE(IGStreamerPipe));

public:
    void CreateElement() override
    {
        GError * pError = nullptr;
        m_pElement = gst_parse_bin_from_description_full((*m_strElementFactory).GetPtr(), TRUE, NULL, GST_PARSE_FLAG_NO_SINGLE_ELEMENT_BINS, &pError);
        //m_pElement = gst_parse_launch((*m_strElementFactory).GetPtr(), &pError);

        if (pError)
        {
            LOG_ERROR(pError->message);
        }

        if (!m_pElement)
        {
            THROW_ERROR_DESC(ERR_FAILED, "Could not create GStreamer Element %s ", (*m_strName).GetPtr());
        }
    }
private:
    
};

ADTF_PLUGIN("GStreamer Filter", cGStreamerLauncherFilter, cGStreamerFilter, cAppSinkFilter, cAppSourceFilter, cGStreamerService)