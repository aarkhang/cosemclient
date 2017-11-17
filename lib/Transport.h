/*
 * Transport.h
 *
 *  Created on: 12 nov. 2017
 *      Author: anthony
 */

#ifndef TRANSPORT_H_
#define TRANSPORT_H_

#include <string>
#include <cstdint>
#include <iostream>
#include <thread>
#include <condition_variable>
#include <queue>
#include <mutex>

enum PrintFormat
{
    NO_PRINT,
    PRINT_RAW,
    PRINT_HEX
};


class Transport
{
public:

    enum Type
    {
        SERIAL,
        TCP_IP
    };

    struct Params
    {
        Params()
            : type(SERIAL)
            , baudrate(9600)
        {

        }
        Type type;
        std::string address;
        std::string port;
        unsigned int baudrate;
    };

    Transport();

    void Initialize(const Params &params);
    void WaitForStop();

    bool Open();
    int Send(const std::string &data, PrintFormat format);
    bool WaitForData(std::string &data, int timeout);

    static void Printer(const char *text, int size, PrintFormat format);

private:

    static const uint32_t cBufferSize = 40U*1024U;
    char mRcvBuffer[cBufferSize];

    Params mConf;
    bool mUseTcpGateway;
    int mSerialHandle;
    bool mTerminate;
    std::string mData;

    std::thread mThread;
    std::condition_variable mCv;
    std::mutex mMutex;

    static void EntryPoint(void *pthis);
    void Reader();
};



#endif /* COSEMCLIENT_LIB_TRANSPORT_H_ */
