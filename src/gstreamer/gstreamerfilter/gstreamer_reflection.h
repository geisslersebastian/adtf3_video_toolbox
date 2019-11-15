/**
 * Copyright 2019 Sebastian Geißler <mail@sebastian.geissler.de>
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


#include <gst/gst.h>
#include <string.h>
#include <stdio.h>
#include <locale.h>

using namespace adtf::util;

class cGStreamerReflection
{
public:
    class cGStreamerProperty
    {
    public:
        cString name;
        cString value;
        cString defaultValue;
        tFloat64 minimum;
        tFloat64 maximum;
    };

    std::map<cString, cGStreamerProperty> m_mapProperties;

    std::map<cString, cGStreamerProperty>& GetProperties()
    {
        return m_mapProperties;
    }

    void ParseElement(GstElement * element)
    {
        GParamSpec  **propertySpecs;
        
        g_return_if_fail(element != NULL);

        guint properties;
        propertySpecs = g_object_class_list_properties(
            G_OBJECT_GET_CLASS(element),
            &properties);
               
        for (quint i = 0; i < properties; i++)
        {
            GValue      value = { 0, };
            GParamSpec  *param = propertySpecs[i];

            bool readable = false;
            bool writeable = false;
            bool controlable = false;

            g_value_init(&value, param->value_type);

            if (param->flags & G_PARAM_READABLE)
            {
                g_object_get_property(G_OBJECT(element), param->name, &value);
                readable = true;
            }

            if (param->flags & G_PARAM_WRITABLE)
            {
                writeable = true;
            }
            if (param->flags & GST_PARAM_CONTROLLABLE)
            {
                controlable = true;
            }
            
            cString name = g_param_spec_get_name(param);

            cString value;
            cString defaultValue;
            cString type;

            tFloat64 minimum = 0.0;
            tFloat64 maximum = 0.0;

            switch (G_VALUE_TYPE(&value))
            {
                case G_TYPE_STRING:
                {
                    GParamSpecString *pString = G_PARAM_SPEC_STRING(param);
                    if (readable)                                        
                    {
                         value = g_value_get_string(&value);
                        
                    }
                    defaultValue = pString->default_value;
                    type = "cString";
                    break;
                }

                case G_TYPE_BOOLEAN: //  Boolean
                {
                    GParamSpecBoolean *pBoolean = G_PARAM_SPEC_BOOLEAN(param);
                    if (readable)
                        value = g_value_get_boolean(&value) ? "true" : "false";
                    defaultValue = pBoolean->default_value ? "true" : "false";
                    type = "tBool";
                    break;
                }

                case G_TYPE_ULONG:  //  Unsigned Long
                {
                    GParamSpecULong *pULong = G_PARAM_SPEC_ULONG(param);
                    if (readable)                                        /* current */
                        value = cString::FromType((tUInt64)g_value_get_ulong(&value));
                    
                    minimum = pULong->minimum;
                    maximum = pULong->maximum;           
                    defaultValue = pULong->default_value;  
                    type = "tUInt64";
                    break;
                }

                case G_TYPE_LONG:  //  Long
                {
                    GParamSpecLong *pLong = G_PARAM_SPEC_LONG(param);
                    if (readable)                                        /* current */
                        value = cString::FromType(g_value_get_long(&value));
                    
                    minimum = pLong->minimum;
                    maximum = pLong->maximum;
                    defaultValue = pLong->default_value;

                    type = "tInt64";
                    break;
                }

                case G_TYPE_UINT:  //  Unsigned Integer
                {
                    GParamSpecUInt *pUInt = G_PARAM_SPEC_UINT(param);
                    if (readable)                                        /* current */
                        value = cString::FromType(g_value_get_uint(&value));
                    
                    minimum = pUInt->minimum;
                    maximum = pUInt->maximum;
                    defaultValue = pUInt->default_value;

                    type = "tUInt32";
                    break;
                }

                case G_TYPE_INT:  //  Integer
                {
                    GParamSpecInt *pInt = G_PARAM_SPEC_INT(param);
                    if (readable)                                        /* current */
                        value = cString::FromType(g_value_get_int(&value));
                    
                    minimum = pInt->minimum;
                    maximum = pInt->maximum;
                    defaultValue = pInt->default_value;
                    type = "tInt32";
                    break;
                }

                case G_TYPE_UINT64: //  Unsigned Integer64.
                {
                    GParamSpecUInt64 *pUInt64 = G_PARAM_SPEC_UINT64(param);
                    if (readable)                                        /* current */
                        value = g_value_get_uint64(&value);
                    
                    minimum = pUInt64->minimum;
                    maximum = pUInt64->maximum;
                    defaultValue = pUInt64->default_value;

                    type = "tUInt64";
                    break;
                }

                case G_TYPE_INT64: // Integer64
                {
                    GParamSpecInt64 *pInt64 = G_PARAM_SPEC_INT64(param);
                    if (readable)                                        /* current */
                        value = cString::FromType(g_value_get_int64(&value));
                    
                    minimum = pInt64->minimum;
                    maximum = pInt64->maximum;
                    defaultValue = pInt64->default_value;

                    type = "tInt64";
                    break;
                }

                case G_TYPE_FLOAT:  //  Float.
                {
                    GParamSpecFloat *pFloat = G_PARAM_SPEC_FLOAT(param);
                    if (readable)                                        /* current */
                        value = cString::FromType(g_value_get_float(&value));
                    
                    minimum = pFloat->minimum;
                    maximum = pFloat->maximum;
                    defaultValue = pFloat->default_value;

                    type = "tFloat32";
                    break;
                }

                case G_TYPE_DOUBLE:  //  Double
                {
                    GParamSpecDouble *pDouble = G_PARAM_SPEC_DOUBLE(param);
                    if (readable)                                        /* current */
                        value = cString::FromType(g_value_get_double(&value));
                    
                    minimum = pDouble->minimum;
                    maximum = pDouble->maximum;
                    defaultValue = pDouble->default_value;

                    type = "tFloat64";
                    break;
                }

                default:
                    if (param->value_type == GST_TYPE_CAPS)
                    {
                        const GstCaps *caps = gst_value_get_caps(&value);
                        //if (!caps)
                            
                        
                    }
                break;
            }

            cGStreamerProperty oProperty;
            oProperty.name = name;
            oProperty.value = value;
            oProperty.defaultValue = defaultValue;
            oProperty.maximum = maximum;
            oProperty.minimum = minimum;

            m_mapProperties[name] = oProperty;

            g_value_reset(&value);
        }

        g_free(property_specs);
    }

};