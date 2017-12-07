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
    HDLC,
    ASSOCIATION_PENDING,
    ASSOCIATED
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
    {

    }
    std::string lls;
    std::string start_date;
    std::string end_date;
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


struct Configuration
{
    Modem modem;
    Cosem cosem;
    hdlc_t hdlc;
    std::vector<Object> list;
    std::string meterId;
    bool testHdlc;
    uint32_t retries;

    Configuration();

    void ParseMeterFile(const std::string &file);
    void ParseComFile(const std::string &file, Transport::Params &comm);
    void ParseObjectsFile(const std::string &file);
};



#endif /* CONFIGURATION_H_ */
