/**
 * Cosem client engine
 *
 * Copyright (c) 2016, Anthony Rabine
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms of the BSD license.
 * See LICENSE.txt for more details.
 *
 */

#ifndef COSEM_CLIENT_H
#define COSEM_CLIENT_H

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

    bool Initialize(Meter &meter, const std::string &commFile, const std::string &objectsFile, const std::string &meterFile);

    void SetStartDate(const std::string &date);
    void SetEndDate(const std::string &date);

    void WaitForStop();

    bool SendModem(const std::string &command, const std::string &expected, std::string &modemReply, uint32_t timeout);

    bool PerformTask();

    std::string ResultToString(csm_data_access_result result);


    std::string GetLls() { return mConf.mMeters[mMeterIndex].cosem.auth_value; }

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
    uint32_t mMeterIndex;
    Configuration mConf;
    Transport mTransport;
    csm_asso_state mAssoState;

    int ConnectHdlc(Meter &meter);
    bool HdlcProcess(Meter &meter, const std::string &send, std::string &rcv, int timeout);
    std::string EncapsulateRequest(csm_array *request);
    bool PerformCosemRead(Meter &meter);
    int ConnectAarq(Meter &meter);
    int ReadObject(const Object &obj);

};

#endif // COSEM_CLIENT_H
