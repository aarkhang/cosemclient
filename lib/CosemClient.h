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

    bool SendModem(const std::string &command, const std::string &expected);

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

    static const uint32_t cSelectiveAccessBufferSize = 256U;
    uint8_t mSelectiveAccessBuff[cSelectiveAccessBufferSize];

    std::uint32_t mReadIndex;
    Configuration mConf;
    Transport mTransport;
    csm_asso_state mAssoState;
    uint32_t mModemTimeout;

    int ConnectHdlc();
    bool HdlcProcess(const std::string &send, std::string &rcv, int timeout);
    std::string EncapsulateRequest(csm_array *request);
    bool PerformCosemRead();
    int ConnectAarq();
    int ReadObject(const Object &obj);

};

#endif // EXAMPLE_COSEM_CLIENT_H
