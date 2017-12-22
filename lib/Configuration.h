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

#ifndef CONFIGURATION_H_
#define CONFIGURATION_H_

#include <string>
#include <cstdint>
#include <iostream>

#include "JsonReader.h"
#include "hdlc.h"
#include "Transport.h"

enum ModemState
{
    DISCONNECTED,
    DIAL,
    CONNECTED
};

enum CosemState
{
    CONNECT_HDLC,
    ASSOCIATION_PENDING,
    ASSOCIATED
};

enum TransportType
{
    HDLC,
    TCP_IP,
    UDP_IP
};

struct Modem
{
    Modem()
        : useModem(false)
    {

    }

    bool useModem;
    std::string phone;
    std::string init;
};

struct Cosem
{
    Cosem()
        : client(1U)
        , logical_device(1U)
    {

    }

    std::string auth_value;
    std::string auth_level;
    uint16_t client;
    uint16_t logical_device;
};


struct Object
{
    Object()
        : class_id(0U)
        , attribute_id(0)
    {

    }

    void Print()
    {
        std::cout << "Object " << name << ": " << ln << " Class ID: " << class_id << " Attribute: " << (int)attribute_id << std::endl;
    }

    std::string name;
    std::string ln;
    std::uint16_t class_id;
    std::int8_t attribute_id;
};

struct Meter
{
    Cosem cosem;
    hdlc_t hdlc;
    std::string meterId;
    bool testHdlcAddr;
    TransportType transport;
};


struct Configuration
{
    std::vector<Meter> mMeters;
    std::vector<Object> list;
    Modem modem;
    uint32_t timeout_connect;
    uint32_t timeout_dial;
    uint32_t timeout_request;
    uint32_t retries;
    std::string start_date;
    std::string end_date;

    Configuration();

    void ParseSessionFile(const std::string &file);
    void ParseComFile(const std::string &file, Transport::Params &comm);
    void ParseObjectsFile(const std::string &file);
};



#endif /* CONFIGURATION_H_ */
