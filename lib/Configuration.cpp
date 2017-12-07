/**
 * Configuration file for Cosem client engine
 *
 * Copyright (c) 2016, Anthony Rabine
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms of the BSD license.
 * See LICENSE.txt for more details.
 *
 */

#include <iostream>
#include <cstdint>
#include "Configuration.h"


Configuration::Configuration()
	: testHdlc(false)
	, retries(0)
{

}

// Very tolerant, use default values of classes if corresponding parameter is not found
void Configuration::ParseMeterFile(const std::string &file)
{
    JsonReader reader;
    JsonValue json;

    if (reader.ParseFile(json, file))
    {
    	JsonValue meterObj = json.FindValue("meter");
		if (meterObj.IsObject())
		{
			JsonValue val = meterObj.FindValue("id");
			if (val.IsString())
			{
				meterId = val.GetString();
			}

			val = meterObj.FindValue("test_hdlc");
			if (val.IsBoolean())
			{
				testHdlc = val.GetBool();
			}

			val = meterObj.FindValue("retries");
			if (val.IsInteger())
			{
				retries = static_cast<uint32_t>(val.GetInteger());
			}
		}

        JsonValue cosemObj = json.FindValue("cosem");
        if (cosemObj.IsObject())
        {
            JsonValue val = cosemObj.FindValue("lls");
            if (val.IsString())
            {
                cosem.lls = val.GetString();
            }
        }

        JsonValue hdlcObj = json.FindValue("hdlc");
        if (hdlcObj.IsObject())
        {
            JsonValue val = hdlcObj.FindValue("phy_addr");
            if (val.IsInteger())
            {
                hdlc.phy_address = static_cast<unsigned int>(val.GetInteger());
            }

            val = hdlcObj.FindValue("logical_device");
            if (val.IsInteger())
            {
                hdlc.logical_device = static_cast<unsigned int>(val.GetInteger());
            }

            val = hdlcObj.FindValue("address_size");
            if (val.IsInteger())
            {
                hdlc.addr_len = static_cast<unsigned int>(val.GetInteger());
            }

            val = hdlcObj.FindValue("client");
            if (val.IsInteger())
            {
                hdlc.client_addr = static_cast<unsigned int>(val.GetInteger());
            }
        }
    }
    else
    {
        std::cout << "** Error opening file: " << file << std::endl;
    }
}


// Very tolerant, use default values of classes if corresponding parameter is not found
void Configuration::ParseComFile(const std::string &file, Transport::Params &comm)
{
    JsonReader reader;
    JsonValue json;

    if (reader.ParseFile(json, file))
    {
        JsonValue dev = json.FindValue("device");
        if (dev.IsString())
        {
            std::string devString = dev.GetString();
            if (devString == "modem")
            {
                modem.useModem = true;
            }
            else
            {
                modem.useModem = false;
            }
        }

        JsonValue modemObj = json.FindValue("modem");
        if (modemObj.IsObject())
        {
            JsonValue val = modemObj.FindValue("phone");
            if (val.IsString())
            {
                modem.phone = val.GetString();

                val = modemObj.FindValue("init");
                if (val.IsString())
                {
                    modem.init = val.GetString();
                }
            }
        }

        JsonValue portObj = json.FindValue("serial");
        if (portObj.IsObject())
        {
            JsonValue val = portObj.FindValue("port");
            if (val.IsString())
            {
                comm.port = val.GetString();
                val = portObj.FindValue("baudrate");
                if (val.IsInteger())
                {
                    comm.baudrate = static_cast<unsigned int>(val.GetInteger());
                }
            }
        }
    }
    else
    {
        std::cout << "** Error opening file: " << file << std::endl;
    }

}

void Configuration::ParseObjectsFile(const std::string &file)
{
    JsonReader reader;
    JsonValue json;

    if (reader.ParseFile(json, file))
    {
        JsonValue val = json.FindValue("objects");
        if (val.IsArray())
        {
            JsonArray arr = val.GetArray();
            for (std::uint32_t i = 0U; i < arr.Size(); i++)
            {
                Object object;
                JsonValue obj = arr.GetEntry(i);

                val = obj.FindValue("name");
                if (val.IsString())
                {
                    object.name = val.GetString();
                }
                val = obj.FindValue("logical_name");
                if (val.IsString())
                {
                    object.ln = val.GetString();
                }
                val = obj.FindValue("class_id");
                if (val.IsInteger())
                {
                    object.class_id = static_cast<std::uint16_t>(val.GetInteger());
                }
                val = obj.FindValue("attribute_id");
                if (val.IsInteger())
                {
                    object.attribute_id = static_cast<std::int8_t>(val.GetInteger());
                }

                object.Print();
                list.push_back(object);
            }
        }
    }
    else
    {
        std::cout << "** Error opening file: " << file << std::endl;
    }
}

