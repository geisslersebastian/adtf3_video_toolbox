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