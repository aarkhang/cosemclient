#ifndef EXAMPLE_COSEM_CLIENT_H
#define EXAMPLE_COSEM_CLIENT_H

#include <unistd.h>
#include <cstdint>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <list>

#include "csm_services.h"
#include "hdlc.h"
#include "Configuration.h"
#include "Transport.h"


struct Compare
{
    bool enabled;
    std::string data;
};


class CosemClient
{
public:
    CosemClient();

    bool Initialize(const std::string &commFile, const std::string &objectsFile);

    void SetStartDate(const std::string &date);
    void SetEndDate(const std::string &date);

    void WaitForStop();

    int Test();
    int Dial(const std::string &phone);

    bool PerformTask();

    std::string ResultToString(csm_data_access_result result);


    std::string GetLls() { return mConf.cosem.lls; }

private:
    ModemState mModemState;
    CosemState mCosemState;

    static const uint32_t cBufferSize = 40U*1024U;
    char mSndBuffer[cBufferSize];
    char mRcvBuffer[cBufferSize];
    csm_array mRcvArray;

    uint8_t mScratch[cBufferSize];

    static const uint32_t cAppBufferSize = 200U*1024U;
    uint8_t mAppBuffer[cAppBufferSize];

    std::uint32_t mReadIndex;
    Configuration mConf;
    Transport mTransport;
    hdlc_t mHdlc;

    int ConnectHdlc();
    bool HdlcProcess(hdlc_t &hdlc, std::string &data, int timeout);
    bool PerformCosemRead();
    int ConnectAarq();
    int ReadObject(const Object &obj);

};

#endif // EXAMPLE_COSEM_CLIENT_H
