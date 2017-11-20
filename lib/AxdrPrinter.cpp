
#include "os_util.h"
#include "hdlc.h"
#include "csm_axdr_codec.h"
#include "csm_array.h"
#include "JsonWriter.h"
#include "JsonValue.h"
#include "AxdrPrinter.h"
#include "clock.h"
#include "Util.h"
#include "date.h"

#include <iomanip>
#include <ctime>
#include <chrono>
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <bitset>

struct tag_t
{
    uint8_t tag;
    std::string name;
};

static const tag_t tags[] = {
        { AXDR_TAG_NULL,            "Null" },
        { AXDR_TAG_ARRAY,           "Array"},
        { AXDR_TAG_STRUCTURE,       "Structure"},
        { AXDR_TAG_BOOLEAN,         "Boolean"},
        { AXDR_TAG_BITSTRING,       "BitString"},
        { AXDR_TAG_INTEGER32,       "Integer32"},
        { AXDR_TAG_UNSIGNED32,      "Unsigned32"},
        { AXDR_TAG_OCTETSTRING,     "OctetString"},
        { AXDR_TAG_VISIBLESTRING,   "VisibleString"},
        { AXDR_TAG_UTF8_STRING,     "UTF8String"},
        { AXDR_TAG_BCD,             "BCD"},
        { AXDR_TAG_INTEGER8,        "Integer8"},
        { AXDR_TAG_INTEGER16,       "Integer16"},
        { AXDR_TAG_UNSIGNED8,       "Unsigned8"} ,
        { AXDR_TAG_UNSIGNED16,      "Unsigned16"},
        { AXDR_TAG_INTEGER64,       "Integer64"},
        { AXDR_TAG_UNSIGNED64,      "Unsigned64"},
        { AXDR_TAG_ENUM,            "Enum"},
        { AXDR_TAG_UNKNOWN,         "Unknown"}
};

static const uint32_t tags_size = sizeof(tags) / sizeof(tags[0]);

static const tag_t units[] = {
        { 27,            "W" },
        { 28,            "VA" },
        { 29,            "var" },
        { 30,            "Wh" },
        { 31,            "VAh" },
        { 32,            "varh" },
        { 33,            "A" },
        { 34,            "C" },
        { 35,            "V" },
        { 44,            "Hz" }
};

static const uint32_t units_size = sizeof(units) / sizeof(units[0]);

std::string TagName(uint8_t tag, uint32_t size)
{
    std::string name = "UnkownTag";

    for (uint32_t i = 0U; i < tags_size; i++)
    {
        if (tags[i].tag == tag)
        {
            name = tags[i].name;

            if (name == "OctetString")
            {
                if (size == 6)
                {
                    name = "OBIS";
                }
                if (size == 12)
                {
                    name = "DateTime";
                }
            }
            break;
        }
    }
    return name;
}

std::string UnitName(uint8_t tag)
{
    std::stringstream ss;

    ss << "UnknownUnit: " << (int)tag;

    std::string name = ss.str();

    for (uint32_t i = 0U; i < units_size; i++)
    {
        if (units[i].tag == tag)
        {
            name = units[i].name;
            break;
        }
    }
    return name;
}

//JsonObject root;
//std::vector<JsonValue> levels;
//JsonValue current;

std::string DateFromCosem(uint32_t size, uint8_t *data)
{
    clk_datetime_t clk;
    std::string dateTime = "InvalidDateTimeFormat";
    csm_array array;

    csm_array_init(&array, data, size, size, 0);

    if (clk_datetime_from_cosem(&clk, &array))
    {
        /*
        std::tm tm;
        std::memset(&tm, 0, sizeof(tm));
        tm.tm_hour = clk.time.hour;
        tm.tm_min = clk.time.minute;
        tm.tm_sec = clk.time.second;
        tm.tm_year = clk.date.year - 1900;
        tm.tm_mon = clk.date.month -1;
        tm.tm_mday = clk.date.day;
        tm.tm_isdst = -1;
//        std::time_t t;
//        std::string s = date::format(format, time_point);
//        std::chrono::system_clock::time_point tp = from_time_t(t);
//            = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        std::cout << std::put_time( &tm, "%FT%T%Z" );
        */
        std::stringstream ss;

        ss <<  clk.date.year << "-" << (int)clk.date.month << "-"  << (int)clk.date.day << "T"  << (int)clk.time.hour << ":"  << (int)clk.time.minute << ":"  << (int)clk.time.second;
        dateTime = ss.str();

    }
    return dateTime;
}


void AxdrPrinter::PrintIndent()
{
    for (uint32_t i = 0U; i < mLevels.size(); i++)
    {
        mStream << "    ";
    }
}

std::string AxdrPrinter::DataToString(uint8_t type, uint32_t size, uint8_t *data)
{
    std::stringstream ss;
    switch (type)
    {
        case AXDR_TAG_NULL:
            ss << "null";
            break;
        case AXDR_TAG_BOOLEAN:
            if (*data == 0)
            {
                ss << "false";
            }
            else
            {
                ss << "true";
            }
            break;
        case AXDR_TAG_BITSTRING:
        {
            uint32_t bytes = BITFIELD_BYTES(size);
            for (uint32_t i = 0U; i < bytes; i++)
            {
                for (int j = 0; j < 8; j++)
                {
                    uint8_t bit = data[i] >> (7-j) & 0x01U;
                    if (bit)
                    {
                        ss << "1";
                    }
                    else
                    {
                        ss << "0";
                    }
                }
            }
            break;
        }
        case AXDR_TAG_INTEGER32:
        {
            uint32_t value = GET_BE32(data);
            ss << static_cast<long>(value);
            break;
        }
        case AXDR_TAG_UNSIGNED32:
        {
            uint32_t value = GET_BE32(data);
            ss << static_cast<unsigned long>(value);
            break;
        }
        case AXDR_TAG_UTF8_STRING:
        case AXDR_TAG_VISIBLESTRING:
        {
            for (uint32_t i = 0U; i < size; i++)
            {
                ss << (char)data[i];
            }
            break;
        }
        case AXDR_TAG_OCTETSTRING:
        {
            if (size == 6)
            {
                // Maybe an OBIS code
                for (uint32_t i = 0U; i < size; i++)
                {
                    ss << static_cast<unsigned long>(data[i]);
                    if (i < (size-1))
                    {
                        ss << ".";
                    }
                }
            }
            else if (size == 12)
            {
                // Maybe a DateTime
                ss << DateFromCosem(size, data);
            }
            else
            {
                char out[2];
                for (uint32_t i = 0U; i < size; i++)
                {
                    byte_to_hex(data[i], &out[0]);

                    ss << out[0] << out[1];
                }
            }
            break;
        }
        case AXDR_TAG_INTEGER8:
        {
            ss << static_cast<int16_t>(data[0]);
            break;
        }
        case AXDR_TAG_INTEGER16:
        {
            int16_t value = GET_BE16(data);
            ss << static_cast<int16_t>(value);
            break;
        }
        case AXDR_TAG_BCD:
        case AXDR_TAG_UNSIGNED8:
        case AXDR_TAG_ENUM:
        {
            //ss << static_cast<int16_t>(data[0]);

            // maybe a unit
            ss << UnitName(data[0]);

            break;
        }
        case AXDR_TAG_UNSIGNED16:
        {
            uint16_t value = GET_BE16(data);
            ss << static_cast<uint16_t>(value);
            break;
        }
        case AXDR_TAG_INTEGER64:
        {
            uint64_t value = GET_BE64(data);
            ss << static_cast<uint64_t>(value);
            break;
        }
        case AXDR_TAG_UNSIGNED64:
        {
            uint64_t value = GET_BE64(data);
            ss << static_cast<uint64_t>(value);
            break;
        }
        case AXDR_TAG_UNKNOWN:
            break;

        default:
            break;
    }

    return ss.str();
}

std::string AxdrPrinter::Get()
{
    return mStream.str();
}

void AxdrPrinter::Start()
{
    mStream.str("");
    mStream  << "<Root>" << std::endl;
}

void AxdrPrinter::End()
{
    mStream  << "</Root>" << std::endl;
}

void AxdrPrinter::Append(uint8_t type, uint32_t size, uint8_t *data)
{
    std::string name = TagName(type, size);

    PrintIndent();

    if ((type == AXDR_TAG_ARRAY) ||
        (type == AXDR_TAG_STRUCTURE))
    {
        mStream  << "<" << name << " size=\"" << size << "\">" << std::endl;

        if (mLevels.size() > 0)
        {
            mLevels.back().counter++;
        }

        Element curr;

        curr.counter = 0;
        curr.size = size;
        curr.type = type;

        mLevels.push_back(curr);
    }
    else
    {
        mStream  << "<" << name << " value=\"" << DataToString(type, size, data) << "\" />" << std::endl;
        if (mLevels.size() > 0)
        {
            mLevels.back().counter++;
        }
    }

    // at least current level is finished, go back N levels
    while (mLevels.size() > 0)
    {
        if (mLevels.back().counter >= mLevels.back().size)
        {
            uint8_t prev_type = mLevels.back().type;
            uint8_t prev_size = mLevels.back().size;

            mLevels.pop_back();

            if ((prev_type == AXDR_TAG_ARRAY) ||
                (prev_type == AXDR_TAG_STRUCTURE))
            {
                PrintIndent();
                mStream  << "</" << TagName(prev_type, prev_size) << ">" << std::endl;
            }
        }
        else
        {
            break;
        }
    }
}

