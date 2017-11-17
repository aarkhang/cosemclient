/*
 * Configuration.h
 *
 *  Created on: 12 nov. 2017
 *      Author: anthony
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

    Configuration();

    void ParseComFile(const std::string &file, Transport::Params &comm);
    bool ParseObjectsFile(const std::string &file);
};



#endif /* CONFIGURATION_H_ */
