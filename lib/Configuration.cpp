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
#include <fstream>
#include <cstdint>
#include <json/json.h>
#include "Configuration.h"

Configuration::Configuration()
	: timeout_connect(3U)
    , timeout_dial(70U)
    , timeout_request(5U)
	, retries(0)
{

}

/*
{
    "version": "1.0.0",

    "session": {
        "phy_layer": "serial",

        "modem": {
            "enable": true,
            "init": "AT",
            "phone": "0631500899"
        },

        "retries": 1,

        "timeouts": {
            "dial": 90,
            "connect": 5,
            "request": 5
        }
    },

    "meters": [
        {
            "id": "saphir0899",
            "transport": "hdlc",
            "hdlc": {
                "phy_addr": 17,
                "address_size": 4,
                "test_addr": false
            },
            "cosem": {
                "auth_level": "LOW_LEVEL_SECURITY",
                "auth_password": "ABCDEFGH",
                "auth_hls_secret": "000102030405060708090A0B0C0D0E0F",
                "client": 1,
                "logical_device": 1
            }
        }
    ]
}

*/

// Very tolerant, use default values of classes if corresponding parameter is not found
bool Configuration::ParseSessionFile(const std::string &file)
{
    std::ifstream ifs(file, std::ifstream::binary);
    if(!ifs)
    {
        std::cerr << "** Error opening file: " << file << std::endl;
        return false;
    }

    Json::CharReaderBuilder builder;
    JSONCPP_STRING errs;
    builder["collectComments"] = true;
    Json::Value json;
    Json::Value val;

    if (!parseFromStream(builder, ifs, &json, &errs)) {
        std::cerr << "** Error parsing " << file
                  << " : "  << errs << std::endl;
        return false;
    }

    Json::Value session = json.get("session", Json::Value::null);
    if (session.isObject())
    {
        val = session.get("retries", Json::Value::null);
        if (val.isInt())
        {
            retries = static_cast<uint32_t>(val.asInt());
        }

        // *********************************   MODEM   *********************************

        Json::Value modemObj = session.get("modem", Json::Value::null);
        if (modemObj.isObject())
        {
            val = modemObj.get("phone", Json::Value::null);
            if (val.isString())
            {
                modem.phone = val.asString();
            }

            val = modemObj.get("enable", Json::Value::null);
            if (val.isBool())
            {
                modem.useModem = val.asBool();
            }

            val = modemObj.get("init", Json::Value::null);
            if (val.isString())
            {
                modem.init = val.asString();
            }
        }

        // *********************************   TIMEOUTS   *********************************
        Json::Value timeoutsObj = session.get("timeouts", Json::Value::null);
        if (timeoutsObj.isObject())
        {
            val = timeoutsObj.get("dial", Json::Value::null);
            if (val.isInt())
            {
                timeout_dial = static_cast<uint32_t>(val.asInt());
            }

            val = timeoutsObj.get("connect", Json::Value::null);
            if (val.isInt())
            {
                timeout_connect = static_cast<uint32_t>(val.asInt());
            }

            val = timeoutsObj.get("request", Json::Value::null);
            if (val.isInt())
            {
                timeout_request = static_cast<uint32_t>(val.asInt());
            }
        }
    }

    Json::Value meterObj = json.get("meters", Json::Value::null);
    if (meterObj.isArray())
    {
        for (Json::Value::const_iterator iter = meterObj.begin(); iter != meterObj.end(); ++iter)
        {
            //iter.key() << iter->asInt() << '\n';
            Meter meter;
            if (iter->isObject())
            {
                val = iter->get("id", Json::Value::null);
                if (val.isString())
                {
                    meter.meterId = val.asString();
                }

                val = iter->get("transport", Json::Value::null);
                if (val.isString())
                {
                    std::string transport = val.asString();
                    if (transport == "hdlc")
                    {
                        meter.transport = HDLC;
                    }
                    else if (transport == "tcp")
                    {
                        meter.transport = TCP_IP;
                    }
                    else
                    {
                        meter.transport = UDP_IP;
                    }
                }

                // *********************************   COSEM   *********************************
                Json::Value cosemObj = iter->get("cosem", Json::Value::null);
                if (cosemObj.isObject())
                {
                    val = cosemObj.get("auth_password", Json::Value::null);
                    if (val.isString())
                    {
                        meter.cosem.auth_password = val.asString();
                    }

                    val = cosemObj.get("auth_hls_secret", Json::Value::null);
                    if (val.isString())
                    {
                        meter.cosem.auth_hls_secret = val.asString();
                    }

                    val = cosemObj.get("auth_level", Json::Value::null);
                    if (val.isString())
                    {
                        meter.cosem.auth_level = val.asString();
                    }

                    val = cosemObj.get("client", Json::Value::null);
                    if (val.isInt())
                    {
                        meter.cosem.client = static_cast<unsigned int>(val.asInt());
                    }

                    val = cosemObj.get("logical_device", Json::Value::null);
                    if (val.isInt())
                    {
                        meter.cosem.logical_device = static_cast<unsigned int>(val.asInt());
                    }
                }

                // *********************************   HDLC   *********************************
                Json::Value hdlcObj = iter->get("hdlc", Json::Value::null);
                if (hdlcObj.isObject())
                {
                    val = hdlcObj.get("phy_addr", Json::Value::null);
                    if (val.isInt())
                    {
                        meter.hdlc.phy_address = static_cast<unsigned int>(val.asInt());
                    }

                    val = hdlcObj.get("address_size", Json::Value::null);
                    if (val.isInt())
                    {
                        meter.hdlc.addr_len = static_cast<unsigned int>(val.asInt());
                    }

                    val = hdlcObj.get("test_addr", Json::Value::null);
                    if (val.isBool())
                    {
                        meter.testHdlcAddr = val.asBool();
                    }
                }

                // Add meter to the list
                meters.push_back(meter);
            }
        }
    }
    return true;
}


// Very tolerant, use default values of classes if corresponding parameter is not found
bool Configuration::ParseComFile(const std::string &file, Transport::Params &comm)
{
    std::ifstream comm_ifs(file, std::ifstream::binary);
    if(!comm_ifs)  // operator! is used here
    {
        std::cerr << "** Error opening file: " << file << std::endl;
        return false;
    }

    Json::CharReaderBuilder builder;
    JSONCPP_STRING errs;
    Json::Value jscomm;
    builder["collectComments"] = true;

    if (!parseFromStream(builder, comm_ifs, &jscomm, &errs)) {
        std::cerr << "** Error parsing " << file
                  << " : "  << errs << std::endl;
        return false;
    }
    //std::cout << "Communication parameters = " <<  jscomm << std::endl; //This will print the entire json object.
    Json::Value portObj = jscomm.get("serial", Json::Value::null);
    if (portObj.isObject())
    {
        Json::Value val = portObj.get("port", Json::Value::null);
        if (val.isString())
        {
            comm.port = val.asString();
            val = portObj.get("baudrate", 9600);
            if (val.isInt())
                comm.baudrate = static_cast<unsigned int>(val.asInt());
        }
    }
    std::cout << "Port "  << comm.port << std::endl;
    std::cout << "Baudrate "  << comm.baudrate << std::endl;
    return true;
}

bool Configuration::ParseObjectsFile(const std::string &file)
{
    std::ifstream ifs(file, std::ifstream::binary);
    if(!ifs)
    {
        std::cerr << "** Error opening file: " << file << std::endl;
        return false;
    }

    Json::CharReaderBuilder builder;
    JSONCPP_STRING errs;
    Json::Value json;
    builder["collectComments"] = true;

    if (!parseFromStream(builder, ifs, &json, &errs)) {
        std::cerr << "** Error parsing " << file
                  << " : "  << errs << std::endl;
        return false;
    }
    Json::Value arrval = json.get("objects", Json::Value::null);
    if (arrval.isArray())
    {
        for (Json::Value::const_iterator iter = arrval.begin(); iter != arrval.end(); ++iter)
        {
            if (iter->isObject())
            {
                Object object;
                Json::Value val = iter->get("name", Json::Value::null);
                if (val.isString())
                {
                    object.name = val.asString();
                }
                val = iter->get("logical_name", Json::Value::null);
                if (val.isString())
                {
                    object.ln = val.asString();
                }
                val = iter->get("class_id", Json::Value::null);
                if (val.isInt())
                {
                    object.class_id = static_cast<std::uint16_t>(val.asInt());
                }
                val = iter->get("attribute_id", Json::Value::null);
                if (val.isInt())
                {
                    object.attribute_id = static_cast<std::int8_t>(val.asInt());
                }

                object.Print();
                list.push_back(object);
            }
        }
    }
    return true;
}
