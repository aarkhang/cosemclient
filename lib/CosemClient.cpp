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

#include <Util.h>
#include <iostream>
#include <iomanip>
#include <ctime>
#include <cstring>
#include <cstdlib>
#include <fstream>


#include "CosemClient.h"
#include "serial.h"
#include "os_util.h"
#include "AxdrPrinter.h"

#include "hdlc.h"
#include "csm_array.h"
#include "csm_services.h"
#include "csm_axdr_codec.h"
#include "clock.h"

int StringToBin(const std::string &in, char *out)
{
    uint32_t sz = in.size();
    int ret = 0;

    if (!(sz % 2))
    {
        hex2bin(in.c_str(), out, sz);
        ret = sz/2;
    }
    return ret;
}

CosemClient::CosemClient()
    : mModemState(DISCONNECTED)
    , mCosemState(HDLC)
    , mReadIndex(0U)
    , mModemTimeout(70U)
{

}


bool CosemClient::Initialize(const std::string &commFile, const std::string &objectsFile, const std::string &meterFile)
{
    bool ok = false;

    Transport::Params params;

    hdlc_init(&mConf.hdlc);
    mConf.hdlc.sender = HDLC_CLIENT;

    mConf.ParseComFile(commFile, params);
    mConf.ParseObjectsFile(objectsFile);
    mConf.ParseMeterFile(meterFile);

    std::cout << "** Using LLS: " << mConf.cosem.lls << std::endl;
    std::cout << "** Using HDLC address: " << mConf.hdlc.phy_address << std::endl;
    std::cout << "** Meter ID: " << mConf.meterId << std::endl;

    if (mConf.meterId.size() == 0U)
    {
    	std::cout << "** Please specify a valid meter ID" << std::endl;
    }
    else
    {
		if (mConf.modem.useModem)
		{
			std::cout << "** Using Modem device" << std::endl;
			mModemState = DISCONNECTED;
		}
		else
		{
			// Skip Modem state chart when no modem is in use
			mModemState = CONNECTED;
		}

		ok = mTransport.Open(params);
		if (ok)
		{
			mTransport.Start();
		}
    }
    return ok;
}

void CosemClient::SetStartDate(const std::string &date)
{
    mConf.cosem.start_date = date;
}

void CosemClient::SetEndDate(const std::string &date)
{
    mConf.cosem.end_date = date;
}

void CosemClient::WaitForStop()
{
    mTransport.WaitForStop();
}


bool CosemClient::HdlcProcess(const std::string &send, std::string &rcv, int timeout)
{
    bool retCode = false;

    csm_array_init(&mRcvArray, (uint8_t*)&mRcvBuffer[0], cBufferSize, 0U, 0U);

    bool loop = true;

    std::string dataToSend = send;
    std::string dataSent;
    do
    {
        if (dataToSend.size() > 0)
        {
            if (mTransport.Send(dataToSend, PRINT_HEX))
            {
                if (mConf.hdlc.type == HDLC_PACKET_TYPE_I)
                {
                    if (mConf.hdlc.sss == 7U)
                    {
                        mConf.hdlc.sss = 0U;
                    }
                    else
                    {
                        mConf.hdlc.sss++;
                    }
                }
                dataSent = dataToSend;
                dataToSend.clear();
            }
            else
            {
                loop = false;
                retCode = false;
            }
        }

        std::string data;
        if (mTransport.WaitForData(data, timeout))
        {
            // We have something, add buffer
            if (!csm_array_write_buff(&mRcvArray, (const uint8_t*)data.c_str(), data.size()))
            {
                loop = false;
                retCode = false;
            }

            if (loop)
            {
                uint8_t *ptr = csm_array_rd_data(&mRcvArray);
                uint32_t size = csm_array_unread(&mRcvArray);


//                printf("Ptr: 0x%08X, Unread: %d\r\n", (unsigned long)ptr, size);
//
//                puts("Sent frame: ");
//                print_hex(mSendCopy.c_str(), mSendCopy.size());
//                puts("\r\n");

                // the frame seems correct, check echo
                if (dataSent.compare(0, dataSent.size(), (char*)ptr, size) == 0)
                {
                    // remove echo from the string
                    csm_array_reader_jump(&mRcvArray, size);
                    ptr = csm_array_rd_data(&mRcvArray);
                    size = csm_array_unread(&mRcvArray);
                    puts("Echo canceled!\r\n");
                }

//                puts("Decoding: ");
//                print_hex((const char *)ptr, size);
//                puts("\r\n");

                do
                {
                    hdlc_t hdlc;
                    hdlc.sender = HDLC_SERVER;
                    int ret = hdlc_decode(&hdlc, ptr, size);
                    if (ret == HDLC_OK)
                    {
                        puts("Good packet\r\n");

                        // God packet! Copy to cosem data
                        rcv.append((const char*)&ptr[hdlc.data_index], hdlc.data_size);

                        // Continue with next one
                        csm_array_reader_jump(&mRcvArray, hdlc.frame_size);

                        if (hdlc.type == HDLC_PACKET_TYPE_I)
                        {
                            // ack last hdlc frame
                            if (hdlc.sss == 7U)
                            {
                                mConf.hdlc.rrr = 0U;
                            }
                            else
                            {
                                mConf.hdlc.rrr = hdlc.sss + 1;
                            }
                        }

                        // Test if it is a last HDLC packet
                        if ((hdlc.segmentation == 0U) &&
                            (hdlc.poll_final == 1U))
                        {
                            puts("Final packet\r\n");

                            retCode = true; // good Cosem packet
                            loop = false; // quit
                        }
                        else if (hdlc.segmentation == 1U)
                        {
                            puts("Segmentation packet: ");
                            hdlc_print_result(&hdlc, HDLC_OK);
                            // There are remaining frames to be received.
                            if (hdlc.poll_final == 1U)
                            {
                                // Send RR
                                hdlc.sender = HDLC_CLIENT;
                                size = hdlc_encode_rr(&mConf.hdlc, (uint8_t*)&mSndBuffer[0], cBufferSize);
                                dataToSend.assign(&mSndBuffer[0], size);
                            }
                        }

                        // go to next frame, if any
                        ptr = csm_array_rd_data(&mRcvArray);
                        size = csm_array_unread(&mRcvArray);
                    }
                    else
                    {
                        // Maybe a partial packet, re-try later
                        size = 0U;
                    }
                } while (size);
            }
        }
        else
        {
            // Timeout, we can't wait further for HDLC packets
            retCode = false;
            loop = false;
        }
    }
    while (loop);

    return retCode;
}


bool CosemClient::SendModem(const std::string &command, const std::string &expected)
{
    bool retCode = false;

    if (mTransport.Send(command, PRINT_RAW))
    {
        std::string modemReply;

        bool loop = true;
        do {
            std::string data;
            if (mTransport.WaitForData(data, mModemTimeout))
            {
                Transport::Printer(data.c_str(), data.size(), PRINT_RAW);
                modemReply += data;

                if (modemReply.find(expected) != std::string::npos)
                {
                    loop = false;
                    retCode = true;
                }
            }
            else
            {
                loop = false;
                retCode = false;
            }
        }
        while (loop);
    }

    std::this_thread::sleep_for(std::chrono::seconds(2U));

    return retCode;
}

int CosemClient::ConnectHdlc()
{
    int ret = -1;

    int size = hdlc_encode_snrm(&mConf.hdlc, (uint8_t *)&mSndBuffer[0], cBufferSize);

    std::string snrmData(&mSndBuffer[0], size);
    std::string data;

    if (HdlcProcess(snrmData, data, 10))
    {
        ret = data.size();
        Transport::Printer(data.c_str(), data.size(), PRINT_HEX);

        // Decode UA
        ret = hdlc_decode_info_field(&mConf.hdlc, (const uint8_t *)data.c_str(), data.size());
        if (ret == HDLC_OK)
        {
            hdlc_print_result(&mConf.hdlc, ret);
            ret = 1U;
        }
    }

    return ret;
}

int CosemClient::ConnectAarq()
{
    int ret = 0;

    csm_array scratch_array;
    csm_array_init(&scratch_array, &mScratch[0], cBufferSize, 0, 3);

    mAssoState.auth_level = CSM_AUTH_LOW_LEVEL;
    mAssoState.ref = LN_REF;

    ret = csm_asso_encoder(&mAssoState, &scratch_array, CSM_ASSO_AARQ);

    if (ret)
    {
        std::string request_data = EncapsulateRequest(&scratch_array);
        std::string data;

        if (HdlcProcess(request_data, data, 5))
        {
            ret = data.size();
            Transport::Printer(data.c_str(), data.size(), PRINT_HEX);
        }
    }
    return ret;
}

std::string CosemClient::ResultToString(csm_data_access_result result)
{
    std::stringstream ss;
    switch (result)
    {
        case CSM_ACCESS_RESULT_SUCCESS:
            ss << "success!";
            break;
        case CSM_ACCESS_RESULT_HARDWARE_FAULT:
            ss << "Hardware fault";
            break;
        case CSM_ACCESS_RESULT_TEMPORARY_FAILURE:
            ss << "temporary failure";
            break;
        case CSM_ACCESS_RESULT_READ_WRITE_DENIED:
            ss << "read write denied";
            break;
        case CSM_ACCESS_RESULT_OBJECT_UNDEFINED:
            ss << "object undefined";
            break;
        case CSM_ACCESS_RESULT_OBJECT_CLASS_INCONSISTENT:
            ss << "object class inconsistent";
            break;
        case CSM_ACCESS_RESULT_OBJECT_UNAVAILABLE:
            ss << "object unavailable";
            break;
        case CSM_ACCESS_RESULT_TYPE_UNMATCHED:
            ss << "type unmatched";
            break;
        case CSM_ACCESS_RESULT_SCOPE_OF_ACCESS_VIOLATED:
            ss << "scope of access violated";
            break;
        case CSM_ACCESS_RESULT_DATA_BLOCK_UNAVAILABLE:
            ss << "data block unavailable";
            break;
        case CSM_ACCESS_RESULT_LONG_GET_ABORTED:
            ss << "long get aborted";
            break;
        case CSM_ACCESS_RESULT_NO_LONG_GET_IN_PROGRESS:
            ss << "no long get in progress";
            break;
        case CSM_ACCESS_RESULT_LONG_SET_ABORTED:
            ss << "long set aborted";
            break;
        case CSM_ACCESS_RESULT_NO_LONG_SET_IN_PROGRESS:
            ss << "no long set in progress";
            break;
        case CSM_ACCESS_RESULT_DATA_BLOCK_NUMBER_INVALID:
            ss << "data block number invalid";
            break;
        case CSM_ACCESS_RESULT_OTHER_REASON:
            ss << "other reason";
            break;
        default:
            break;
    }
    return ss.str();
}

static AxdrPrinter gPrinter;

static void AxdrData(uint8_t type, uint32_t size, uint8_t *data)
{
    gPrinter.Append(type, size, data);
}


std::string CosemClient::EncapsulateRequest(csm_array *request)
{
    std::string request_data;

    if (request->offset != 3U)
    {
        std::cout << "Cosem array must have room for LLC" << std::endl;
    }
    else
    {
        // remove offset
        request->offset = 0;
        request->wr_index += 3U; // adjust size written

        csm_array_set(request, 0U, 0xE6U);
        csm_array_set(request, 1U, 0xE6U);
        csm_array_set(request, 2U, 0x00U);

        // Encode HDLC
        mConf.hdlc.sender = HDLC_CLIENT;
        int send_size = hdlc_encode_data(&mConf.hdlc, (uint8_t *)&mSndBuffer[0], cBufferSize, request->buff, csm_array_written(request));

        request_data.assign((char *)&mSndBuffer[0], send_size);
    }

    return request_data;
}


void DateToCosem(std::tm &date, csm_array *array)
{
    clk_datetime_t clk;

    // 1. Transform standard date into cosem format

    clk.date.year = date.tm_year + 1900;
    clk.date.month = date.tm_mon + 1;
    clk.date.day = date.tm_mday;
    clk.date.dow = 0xFFU; // not specified
    clk.time.hour = date.tm_hour;
    clk.time.minute = date.tm_min;
    clk.time.second = date.tm_sec;
    clk.time.hundredths = 0U;

    clk.deviation = static_cast<std::int16_t>(0x8000);
    clk.status = 0xFFU;

    // 2. Serialize
    clk_datetime_to_cosem(&clk, array);
}


int CosemClient::ReadObject(const Object &obj)
{
    int ret = 1;
    bool allowSelectiveAccess = false;
    bool hasEndDate = false;

    std::tm tm_start = {};
    std::tm tm_end = {};

    // Allow selective access only on profile get buffer attribute
    if ((obj.attribute_id == 2) && (obj.class_id == 7U))
    {
        allowSelectiveAccess = true;
    }

    if (allowSelectiveAccess)
    {
        if (mConf.cosem.start_date.size() > 0)
        {
            // Try to decode start date
            std::stringstream ss(mConf.cosem.start_date);
            ss >> std::get_time(&tm_start, "%Y-%m-%d.%H:%M:%S");

            if (ss.fail())
            {
                std::cout << "** Parse start date failed\r\n";
                allowSelectiveAccess = false;
            }
        }
        else
        {
            // No start date, disable selective access
            allowSelectiveAccess = false;
        }

        if (mConf.cosem.end_date.size() > 0)
        {
            std::stringstream ss2(mConf.cosem.end_date);
            ss2 >> std::get_time(&tm_end, "%Y-%m-%d.%H:%M:%S");
            if (ss2.fail())
            {
                std::cout << "** Parse end date failed\r\n";
                allowSelectiveAccess = false;
            }
            else
            {
            	hasEndDate = true;
            }
        }
        else
        {
            // No end date
        	hasEndDate = false;
            std::cout << "** No end date defined" << std::endl;
        }
    }

    // For reception
    csm_array app_array;
    csm_array_init(&app_array, &mAppBuffer[0], cAppBufferSize, 0, 0);

    csm_request request;

    if (allowSelectiveAccess)
    {
        // Setup selective access options
        request.db_request.use_sel_access = TRUE;
        csm_array_init(&request.db_request.access_params, &mSelectiveAccessBuff[0], cSelectiveAccessBufferSize, 0, 0);
        uint8_t clockBuffStart[12];
        uint8_t clockBuffEnd[12];
        csm_array clockStartArray;
        csm_array clockEndArray;

        csm_array_init(&clockStartArray, &clockBuffStart[0], 12, 0, 0);
        csm_array_init(&clockEndArray, &clockBuffEnd[0], 12, 0, 0);


        DateToCosem(tm_start, &clockStartArray);

        if (hasEndDate)
        {
        	DateToCosem(tm_end, &clockEndArray);
        }
        else
        {
        	clk_datetime_t clk;

        	// Set all Cosem DateTime fields as undefined
        	clk_set_undefined(&clk);
        	clk_datetime_to_cosem(&clk, &clockEndArray);
        }

        csm_object_t clockObj;

        clockObj.class_id = 8;
        clockObj.data_index = 0;
        clockObj.id = 2;
        clockObj.obis.A = 0;
        clockObj.obis.B = 0;
        clockObj.obis.C = 1;
        clockObj.obis.D = 0;
        clockObj.obis.E = 0;
        clockObj.obis.F = 255;

        csm_client_encode_selective_access_by_range(&request.db_request.access_params, &clockObj, &clockStartArray, &clockEndArray);
    }
    else
    {
        request.db_request.use_sel_access = FALSE;
    }

    request.type = SVC_GET_REQUEST_NORMAL;
    request.sender_invoke_id = 0xC1U;
    request.db_request.data.class_id = obj.class_id;

    std::vector<std::string> obis = Util::Split(obj.ln, ".");

    if (obis.size() == 6)
    {
        request.db_request.data.obis.A = strtol(obis[0].c_str(), NULL, 10);
        request.db_request.data.obis.B = strtol(obis[1].c_str(), NULL, 10);
        request.db_request.data.obis.C = strtol(obis[2].c_str(), NULL, 10);
        request.db_request.data.obis.D = strtol(obis[3].c_str(), NULL, 10);
        request.db_request.data.obis.E = strtol(obis[4].c_str(), NULL, 10);
        request.db_request.data.obis.F = strtol(obis[5].c_str(), NULL, 10);
    }
    else
    {
        ret = 0;
    }
    request.db_request.data.id = obj.attribute_id;

    csm_array scratch_array;
    csm_array_init(&scratch_array, &mScratch[0], cBufferSize, 0, 3);

    if (ret && svc_get_request_encoder(&request, &scratch_array))
    {
        std::cout << "** Sending request for object: " << obj.name << std::endl;

        std::string request_data = EncapsulateRequest(&scratch_array);
        std::string data;
        csm_response response;
        bool loop = true;
        bool dump = false;

        do
        {
            data.clear();

            if (HdlcProcess(request_data, data, 5))
            {
                Transport::Printer(data.c_str(), data.size(), PRINT_HEX);
                csm_array_init(&scratch_array, &mScratch[0], cBufferSize, 0, 0);

                csm_array_write_buff(&scratch_array, (const uint8_t *)data.c_str(), data.size());

                uint8_t llc1, llc2, llc3;
                int valid = csm_array_read_u8(&scratch_array, &llc1);
                valid = valid && csm_array_read_u8(&scratch_array, &llc2);
                valid = valid && csm_array_read_u8(&scratch_array, &llc3);

                if (valid && (llc1 == 0xE6U) &&
                    (llc2 == 0xE7U) &&
                    (llc3 == 0x00U))
                {
                    // Good Cosem server packet
                    if (csm_client_decode(&response, &scratch_array))
                    {
                        if (response.access_result == CSM_ACCESS_RESULT_SUCCESS)
                        {
                            if (response.type == SVC_GET_RESPONSE_NORMAL)
                            {
                                // We have the data, copy it to the application buffer and stop
                                csm_array_write_buff(&app_array, csm_array_rd_data(&scratch_array), csm_array_unread(&scratch_array));
                                loop = false;
                                dump = true;
                            }
                            else if (response.type == SVC_GET_RESPONSE_WITH_DATABLOCK)
                            {
                            	// Copy data into app data
								uint32_t size = 0U;
								if (csm_axdr_decode_block(&scratch_array, &size))
								{
									std::cout << "** Block of data of size: " << size << std::endl;
									// FIXME: Test the size indicated in the packet and the real size received
									// Add it
									csm_array_write_buff(&app_array, csm_array_rd_data(&scratch_array), csm_array_unread(&scratch_array));

									// Check if last block
									if (csm_client_has_more_data(&response))
									{
										// Send next block
										request.type = SVC_GET_REQUEST_NEXT;
										request.db_request.block_number = response.block_number;
										request.sender_invoke_id = response.invoke_id;

										csm_array_init(&scratch_array, &mScratch[0], cBufferSize, 0, 3);

										svc_get_request_encoder(&request, &scratch_array);

										hdlc_print_result(&mConf.hdlc, HDLC_OK);
										request_data = EncapsulateRequest(&scratch_array);

										printf("** Sending ReadProfile next...\r\n");
									}
									else
									{
										puts("No more data\r\n");
										loop = false;
										dump = true;
									}
								}
								else
								{
									puts("** ERROR: must be a block of data\r\n");
									loop = false;
								}
                            }
                            else
                            {
                                puts("Service not supported\r\n");
                                loop = false;
                            }
                        }
                        else
                        {
                            std::cout << "** Data access result: " << ResultToString(response.access_result) << std::endl;
                            loop = false;

                            // Try to save work anyway
                            dump = true;
                        }
                    }
                    else
                    {
                        puts("Cannot decode Cosem response\r\n");
                        loop = false;
                    }
                }
                else
                {
                    puts("Not a compliant HDLC LLC\r\n");
                    loop = false;
                }
            }
            else
            {
                puts("Cannot get HDLC data\r\n");
                loop = false;
            }
        }
        while(loop);

        if (dump)
        {

            std::string infos = "Object=\"" + obj.name + "\"";
            gPrinter.Start(infos);
            csm_axdr_decode_tags(&app_array, AxdrData);
            gPrinter.End();

            std::string xml_data = gPrinter.Get();
            std::cout << xml_data << std::endl;

            std::string dirName = mConf.meterId;
            std::string fileName = dirName + Util::DIR_SEPARATOR + obj.name + ".xml";

            std::cout << "Dumping into file: " << fileName << std::endl;

            std::fstream f;

            Util::Mkdir(dirName);
            f.open(fileName, std::ios_base::out | std::ios_base::binary);

            if (f.is_open())
            {
                f << xml_data << std::endl;
                f.close();
            }
            else
            {
                std::cout << "Cannot open file!" << std::endl;
            }
           // print_hex((const char *)&mAppBuffer[0], csm_array_written(&app_array));
        }

    }

    return ret;
}

bool  CosemClient::PerformCosemRead()
{
    bool ret = false;
    static uint32_t retries = 0U;

    switch(mCosemState)
    {
        case HDLC:
            printf("** Sending HDLC SNRM (addr: %d)...\r\n", mConf.hdlc.phy_address);
            if (ConnectHdlc() > 0)
            {
               printf("** HDLC success!\r\n");
               ret = true;
               mCosemState = ASSOCIATION_PENDING;
            }
            else
            {
                retries++;
                if (retries > mConf.retries)
                {
                    retries = 0;
                    if (mConf.testHdlc)
                    {
                        // Keep this state, no error, scan next HDLC address
                        mConf.hdlc.phy_address++;
                        ret = true;
                    }
                    else
                    {
                        printf("** Cannot connect to meter.\r\n");
                    }
                }
                else
                {
                    ret = true;
                }
            }
         break;
        case ASSOCIATION_PENDING:

            printf("** Sending AARQ...\r\n");
            if (ConnectAarq() > 0)
            {
               printf("** AARQ success!\r\n");
               ret = true;
               mReadIndex = 0U;
               mCosemState = ASSOCIATED;
            }
            else
            {
               printf("** Cannot AARQ to meter.\r\n");
            }
            break;
        case ASSOCIATED:
        {
            if (mReadIndex < mConf.list.size())
            {
                Object obj = mConf.list[mReadIndex];

                (void) ReadObject(obj);
                ret = true;

                mReadIndex++;
            }
            break;
        }
        default:
            break;

    }

    return ret;
}


// Global state chart
bool CosemClient::PerformTask()
{
    bool ret = false;

    switch (mModemState)
    {
        case DISCONNECTED:
        {
            if (SendModem(mConf.modem.init + "\r\n", "OK") > 0)
            {
                printf("** Modem test success!\r\n");

                mModemState = DIAL;
                ret = true;
            }
            else
            {
                printf("** Modem test failed.\r\n");
            }

            break;
        }

        case DIAL:
        {
            std::cout << "** Dial: " << mConf.modem.phone << std::endl;
            std::string dialRequest = std::string("ATD") + mConf.modem.phone + std::string("\r\n");
            if (SendModem(dialRequest, "CONNECT"))
            {
               printf("** Modem dial success!\r\n");
               ret = true;
               mModemState = CONNECTED;
            }
            else
            {
               printf("** Dial failed.\r\n");
            }

            break;
        }

        case CONNECTED:
        {
            ret = PerformCosemRead();
            break;
        }
        default:
            break;
    }
    return ret;
}

