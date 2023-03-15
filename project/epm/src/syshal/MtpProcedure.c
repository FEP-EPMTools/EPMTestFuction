/**************************************************************************//**
* @file     MtpProcedure.c
* @version  V1.00
* $Revision: 
* $Date: 
* @brief For EPD Burning Test   
*
* @note
* Copyright (C) 2019 Far Easy Pass LTD. All rights reserved.
*****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nuc970.h"
#include "sys.h"

/* Scheduler includes. */
#include "fepconfig.h"
#include "timelib.h"
#include "guidrv.h"
#include "halinterface.h"
#include "MtpProcedure.h"
#include "crypto.h"
#include "mtp.h"
#include "sflashrecord.h"
//#include "rs232CommDrv.h"

/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/

#define CMD_INITAIL_AUTHENTICATE            0x91
#define CMD_MUTUAL_AUTHENTICATE             0x92
#define CMD_WRITE_MTP_HASH_KEY              0x93
#define CMD_MTP_HASH_KEY_RESULT             0x94
#define CMD_MTP_PROCEDURE_EXIT              0x99
#define CMD_SWITCH_PARAMETER_MODE           0x9A
#define CMD_PARAMETER_MODE_COMMON_REPLY     0x9B
#define CMD_UPDATE_RTC_REQUEST              0x9C
#define CMD_SET_DEVICE_PARAMETER_REQUEST    0x9D
#define MAXIMUM_KEY_INDEX                   9
#define SECURITY_KEY_LENGTH_BYTES           16
#define EXPANDED_KEY_LENGTH_BYTES           8
#define RANDOM_NUMBER_LENGTH                8
#define DEFAULT_BLOCK_SIZE_BITS             64
#define WRITE_MTP_MESSAGE_LENGTH            48
#define MTP_HASHKEY_LENGTH                  32
#define MTP_MESSAGE_HEADER_LENGTH           8
#define MTP_MESSAGE_TAIL_LENGTH             7
#define TDES_CHANNEL_0                      0
#define CRYPTO_MODE_DECRYPT                 0
#define CRYPTO_MODE_ENCRYPT                 1
#define ASCII_ACK                           0x06
#define ASCII_NAK                           0x15
#define UPDATE_RTC_MESSAGE_LENGTH           12
#define SET_DEVICE_PARAMETER_MESSAGE_LENGTH 32

static uint32_t InitialVector[2] = {0x00000000, 0x00000000};
static uint8_t SECURITY_KEY[MAXIMUM_KEY_INDEX + 1][SECURITY_KEY_LENGTH_BYTES] =
        {
            {0x48, 0x0C, 0x81, 0xA3, 0x0E, 0x15, 0x03, 0x03, 0x0B, 0xB5, 0xC1, 0x25, 0x79, 0xA4, 0x97, 0x7D},       // Key Index 0
            {0xD8, 0x0F, 0x12, 0x01, 0x7D, 0xFC, 0xD2, 0xEC, 0x2D, 0xF1, 0x51, 0x2A, 0x05, 0x0A, 0xF2, 0xE6},       // Key Index 1
            {0xD3, 0xC6, 0xBE, 0x4F, 0x07, 0x40, 0xB2, 0x3D, 0x35, 0x1E, 0x14, 0xA4, 0xAB, 0x8D, 0x24, 0xC6},       // Key Index 2
            {0xBF, 0x63, 0x05, 0xDB, 0x99, 0x41, 0xF9, 0xC7, 0x8E, 0x99, 0xE4, 0xB2, 0x18, 0x71, 0xA2, 0xC1},       // Key Index 3
            {0x5D, 0xB2, 0xFB, 0xCE, 0x24, 0x4A, 0xBE, 0x88, 0xAD, 0x73, 0x39, 0x46, 0x42, 0x9B, 0x38, 0xCB},       // Key Index 4
            {0x75, 0x6E, 0x07, 0x0F, 0x44, 0x9F, 0x45, 0xBC, 0x36, 0x5B, 0x3F, 0xAE, 0x81, 0xEE, 0xD6, 0x99},       // Key Index 5
            {0xDA, 0xE7, 0x5B, 0xD8, 0x6B, 0x16, 0x64, 0xAF, 0x71, 0x8B, 0x20, 0xA9, 0xC8, 0xC5, 0x01, 0x7D},       // Key Index 6
            {0x7F, 0x8F, 0xFE, 0x91, 0x3E, 0x43, 0x88, 0x9C, 0xBF, 0xA1, 0xE0, 0x13, 0x23, 0x0C, 0xC9, 0xAA},       // Key Index 7
            {0x5E, 0xD9, 0x83, 0xFF, 0x3F, 0xF1, 0x6B, 0x50, 0x1B, 0xAF, 0x5A, 0x40, 0x43, 0x62, 0xC5, 0x56},       // Key Index 8
            {0xC0, 0xB7, 0x73, 0xD1, 0x6E, 0x2E, 0x93, 0x70, 0x20, 0xB5, 0x40, 0x48, 0xA7, 0xCE, 0x0A, 0xB8}        // Key Index 9
        };
static uint8_t MTP_MessageHeader[MTP_MESSAGE_HEADER_LENGTH] = {0x72, 0x83, 0x94, 0xA5, 0xB6, 0xC7, 0xD8, 0xE9};
static uint8_t MTP_MessageTail[MTP_MESSAGE_TAIL_LENGTH] = {0xCF, 0xBE, 0xAD, 0x9C, 0x8B, 0x7A, 0x69};

//===================== for CRCTool =====================
#define CRCCode_CRC_CCITT               1
#define CRCCode_CRC16                   2
#define CRCCode_CRC32                   3
//===================== for CRCTool =====================

/*-----------------------------------------*/
/* global file scope (static) variables    */
/*-----------------------------------------*/
static CommunicationInterface* pCommunicationInterface = NULL;
//static INT (*CheckUART0Received)(void) = sysIsKbHit;

static uint8_t RxBuffer[256];
static uint8_t TxBuffer[256];
static uint8_t AuthenSessionKey[SECURITY_KEY_LENGTH_BYTES + EXPANDED_KEY_LENGTH_BYTES];
static uint8_t MTP_HashKey[MTP_HASHKEY_LENGTH];
static int RxCounter = 0;
static int TxCounter = 0;
static uint8_t MTP_OptionData;
static BOOL EnableAuthenSessionKeyFlag = FALSE;
static BOOL isStartFlag = FALSE;
static BOOL enabledSwitchRTCMode = FALSE;
static uint8_t RTCBuffer[UPDATE_RTC_MESSAGE_LENGTH];
static uint8_t DeviceParameterBuffer[SET_DEVICE_PARAMETER_MESSAGE_LENGTH];

uint8_t CryptoInputCipher_Pool[64] __attribute__((aligned(32)));
uint8_t CryptoOutputCipher_Pool[64] __attribute__((aligned(32)));
static uint8_t *CryptoInputCipher;
static uint8_t *CryptoOutputCipher;
static volatile int CryptoDoneFlag = 0;

//===================== for CRCTool =====================
static int order = 16;
static uint32_t polynom = 0x1021;
static int direct = 1;
static uint32_t crcinit = 0xFFFF;
static uint32_t crcxor = 0x0;
static int refin = 0;
static int refout = 0;
static uint32_t crcmask;
static uint32_t crchighbit;
static uint32_t crcinit_direct;
static uint32_t crcinit_nondirect;
static uint32_t crctab[256];
//===================== for CRCTool =====================

/*-----------------------------------------*/
/* prototypes of static functions          */
/*-----------------------------------------*/

// reflects the lower 'bitnum' bits of 'crc'
static uint32_t reflect(uint32_t crc, int bitnum)
{
    uint32_t i, j = 1, crcout = 0;
    for (i = (uint32_t)1 << (bitnum - 1) ; i != 0 ; i >>= 1)
    {
        if ((crc & i) != 0) {
            crcout |= j;
        }
        j <<= 1;
    }
    return crcout;
}

// make CRC lookup table used by table algorithms
static void generate_crc_table(void)
{
    int i, j;
    uint32_t bit, crc;
    
    for (i = 0 ; i < 256 ; i++)
    {
        crc = (uint32_t)i;
        if (refin != 0) {
            crc = reflect(crc, 8);
        }
        crc <<= order - 8;
        
        for (j = 0 ; j < 8 ; j++)
        {
            bit = crc & crchighbit;
            crc <<= 1;
            if (bit != 0) {
                crc ^= polynom;
            }
        }
        
        if (refin != 0)
        {
            crc = reflect(crc, order);
        }
        crc &= crcmask;
        crctab[i]= crc;
    }
}

/************************************************************************************
*                                                                                   *
* Function: Interrupt handler for crypto service and random generation service.     *
*                                                                                   *
*************************************************************************************/
static void CryptoService_IRQHandler()
{
    if (TDES_GET_INT_FLAG())
    {
        CryptoDoneFlag = 1;
        TDES_CLR_INT_FLAG();
    }
    if (PRNG_GET_INT_FLAG())
    {
        CryptoDoneFlag = 1;
        PRNG_CLR_INT_FLAG();
    }
}

/************************************************************************************
* Function: Crypto service: decryption or encryption using TripleDES.               *
* Input   : inputLength: Must be multiple of 8                                      *
*           CryptoKey  : Must be 128-bits(16 bytes) or 192-bit(24 bytes)            *
*           expandKeyFlag: If CryptoKey is 128-bits, set this flag to TRUE, that    *
*                          will be expanded to 192-bits to run 3DES crypto.         *
* Return  : None. Please check CryptoOutputCipher after this function been called.  *
*************************************************************************************/
static void CryptoService(int CryptoMode, uint8_t *inputBuffer, int inputLength, uint8_t *CryptoKey, BOOL expandKeyFlag)
{
    uint8_t TripleDES_3Keys[3][8];
    
    memcpy(CryptoInputCipher_Pool, inputBuffer, inputLength);
    TDES_Open(TDES_CHANNEL_0, CryptoMode, TDES_MODE_ECB, TDES_IN_OUT_WHL_SWAP);
    if (expandKeyFlag)
    {
        // Expand key from 128-bits to 192-bits
        memcpy(&TripleDES_3Keys[0][0], CryptoKey, SECURITY_KEY_LENGTH_BYTES);   // Set Key1(64-bits), Key2(64-bits) from original key
        memcpy(&TripleDES_3Keys[2][0], CryptoKey, EXPANDED_KEY_LENGTH_BYTES);   // Expanded Key3(64-bits) from Key1
    }
    else {
        memcpy(&TripleDES_3Keys[0][0], CryptoKey, (SECURITY_KEY_LENGTH_BYTES + EXPANDED_KEY_LENGTH_BYTES));
    }
    TDES_SetKey(TDES_CHANNEL_0, TripleDES_3Keys);
    TDES_SetInitVect(TDES_CHANNEL_0, InitialVector[0], InitialVector[1]);
    TDES_SetDMATransfer(TDES_CHANNEL_0, (uint32_t)CryptoInputCipher, (uint32_t)CryptoOutputCipher, inputLength);
    
    CryptoDoneFlag = 0;
    TDES_Start(TDES_CHANNEL_0, CRYPTO_DMA_ONE_SHOT);
    while (!CryptoDoneFlag);
}

static void RTC_CommonReplyCommand(uint8_t replyCode)
{
    TxCounter = 0;
    TxBuffer[TxCounter++] = CMD_PARAMETER_MODE_COMMON_REPLY;
    TxBuffer[TxCounter++] = 0x01;
    TxBuffer[TxCounter++] = replyCode;
    
    // Calculate CRC Result
    uint16_t CRCResult = CRCTool_TableFast(TxBuffer, TxCounter);
    TxBuffer[TxCounter++] = (uint8_t)(CRCResult >> 8);
    TxBuffer[TxCounter++] = (uint8_t)CRCResult;
    pCommunicationInterface->writeFunc(TxBuffer, TxCounter);
}

static BOOL RTC_ExecuteUpdateRTCTime(void)
{
    //uint8_t tempBuff[256];
    //uint32_t utcTime = (uint32_t)(RTCBuffer[3] << 24) + (uint32_t)(RTCBuffer[2] << 16) + (uint32_t)(RTCBuffer[1] << 8) + RTCBuffer[0];
    //sprintf((char *)tempBuff, "UTCTime : %d\r\n", utcTime);
    //pCommunicationInterface->writeFunc(tempBuff, strlen((char *)tempBuff));
    //vTaskDelay(100);
    
    uint32_t rtcYear, rtcMonth, rtcDay, rtcDayOfWeek, rtcHour, rtcMinute, rtcSecond;
    rtcYear = (uint32_t)(RTCBuffer[5] << 8) + (uint32_t)RTCBuffer[4];
    rtcMonth = (uint32_t)RTCBuffer[6];
    rtcDay = (uint32_t)RTCBuffer[7];
    rtcDayOfWeek = (uint32_t)RTCBuffer[8];
    rtcHour = (uint32_t)RTCBuffer[9];
    rtcMinute = (uint32_t)RTCBuffer[10];
    rtcSecond = (uint32_t)RTCBuffer[11];
    return (SetOSTime(rtcYear, rtcMonth, rtcDay, rtcHour, rtcMinute, rtcSecond, rtcDayOfWeek));
}

#define MANUFACTURE_DEVICE_ID_LENGTH    (SET_DEVICE_PARAMETER_MESSAGE_LENGTH - sizeof(uint32_t) - sizeof(uint32_t))
static BOOL RTC_ExecuteSetDeviceID(void)
{
    typedef struct
    {
        uint32_t deviceID;
        uint32_t burninTime;
        uint8_t manufactureDeviceID[MANUFACTURE_DEVICE_ID_LENGTH];
    } _deviceParameter;
    _deviceParameter oldDeviceParameter, newDeviceParameter;
    
    BOOL status = SFlashLoadStorage(SFLASH_STORAGE_EPM_SERIAL_ID_INDEX_BASE, (uint8_t*)&oldDeviceParameter, sizeof(oldDeviceParameter));
    
    newDeviceParameter.deviceID = (uint32_t)(DeviceParameterBuffer[3] << 24) + (uint32_t)(DeviceParameterBuffer[2] << 16) + (uint32_t)(DeviceParameterBuffer[1] << 8) + DeviceParameterBuffer[0];
    if (newDeviceParameter.deviceID == 0) {
        newDeviceParameter.deviceID = oldDeviceParameter.deviceID;
    }
    
    newDeviceParameter.burninTime = (uint32_t)(DeviceParameterBuffer[7] << 24) + (uint32_t)(DeviceParameterBuffer[6] << 16) + (uint32_t)(DeviceParameterBuffer[5] << 8) + DeviceParameterBuffer[4];
    if (newDeviceParameter.burninTime == 0xFFFFFFFF) {
        newDeviceParameter.burninTime = oldDeviceParameter.burninTime;
    }
    
    if (DeviceParameterBuffer[8] == 0x00) {
        memcpy(newDeviceParameter.manufactureDeviceID, oldDeviceParameter.manufactureDeviceID, MANUFACTURE_DEVICE_ID_LENGTH);
    }
    else {
        memcpy(newDeviceParameter.manufactureDeviceID, &DeviceParameterBuffer[8], MANUFACTURE_DEVICE_ID_LENGTH);
    }
    return (SFlashSaveStorage(SFLASH_STORAGE_EPM_SERIAL_ID_INDEX, (uint8_t*)&newDeviceParameter, SET_DEVICE_PARAMETER_MESSAGE_LENGTH));
    
    //uint32_t deviceID = (uint32_t)(DeviceParameterBuffer[3] << 24) + (uint32_t)(DeviceParameterBuffer[2] << 16) + (uint32_t)(DeviceParameterBuffer[1] << 8) + DeviceParameterBuffer[0];
    //return (SFlashSaveStorage(SFLASH_STORAGE_EPM_SERIAL_ID_INDEX, (uint8_t*)&deviceID, SET_DEVICE_PARAMETER_MESSAGE_LENGTH));
}

/************************************************************************************
* Function: Generate a random number.                                               *
* Input   : generateType: Can be one of 64-bits, 128-bits, 192-bits or 256-bits.    *
*           outBuffer   : To store the result of random number, the length of       *
*                         outBuffer is related to generateType.                     *
* Return  : None. Please check outBuffer after this function been called.           *
*************************************************************************************/
static void RandomNumberService(int generateType, uint8_t *outBuffer)
{
    PRNG_Open(generateType, 0, 0);
    CryptoDoneFlag = 0;
    PRNG_Start();
    while (!CryptoDoneFlag);
    PRNG_Read((uint32_t *)outBuffer);
}

/************************************************************************************
* Function: Build Authenticate Session Key (192-bits).                              *
* Input   : Rnd_A: Buffer of random number A.                                       *
*           Rnd_B: Buffer of random number B.                                       *
* Return  : None. Please check AuthenSessionKey after this function been called.    *
*************************************************************************************/
static void MTP_BuildAuthenSessionKey(uint8_t *Rnd_A, uint8_t *Rnd_B)
{
    memset(AuthenSessionKey, 0x00, sizeof(AuthenSessionKey));
    memcpy(&AuthenSessionKey[0], &Rnd_B[4], 4);
    memcpy(&AuthenSessionKey[4], &Rnd_A[0], 4);
    memcpy(&AuthenSessionKey[8], &Rnd_A[4], 4);
    memcpy(&AuthenSessionKey[12], &Rnd_B[0], 4);
    memcpy(&AuthenSessionKey[16], &Rnd_A[2], 4);
    memcpy(&AuthenSessionKey[20], &Rnd_B[2], 4);
    EnableAuthenSessionKeyFlag = TRUE;
}

/************************************************************************************
* Function: Send Mutual Authenticate protocol to host.                              *
* Input   : Rnd_B: Buffer of random number B.                                       *
* Return  : None.                                                                   *
*************************************************************************************/
static void MTP_SendMutualAuthenticate(uint8_t *Rnd_B)
{
    uint16_t CRCResult;
    uint8_t KeyIndex;
    
    KeyIndex = Rnd_B[RANDOM_NUMBER_LENGTH - 1] % 10;
    TxCounter = 0;
    TxBuffer[TxCounter++] = CMD_MUTUAL_AUTHENTICATE;
    TxBuffer[TxCounter++] = RANDOM_NUMBER_LENGTH + 1;
    TxBuffer[TxCounter++] = KeyIndex;
    
    CryptoService(CRYPTO_MODE_ENCRYPT, Rnd_B, RANDOM_NUMBER_LENGTH, &SECURITY_KEY[KeyIndex][0], TRUE);
    memcpy(&TxBuffer[TxCounter], CryptoOutputCipher, RANDOM_NUMBER_LENGTH);
    TxCounter += RANDOM_NUMBER_LENGTH;
    
    // Calculate & Compare CRC Result
    CRCResult = CRCTool_TableFast(TxBuffer, TxCounter);
    TxBuffer[TxCounter++] = (uint8_t)(CRCResult >> 8);
    TxBuffer[TxCounter++] = (uint8_t)CRCResult;
    pCommunicationInterface->writeFunc(TxBuffer, TxCounter);
}

/************************************************************************************
* Function: Write hash key(256-bits) to MTP position.                               *
* Input   : None. Please prepare data buffer: MTP_HashKey and MTP_OptionData        *
* Return  : Boolean: If any errors will return FALSE.                               *
*************************************************************************************/
static BOOL MTP_ExecuteWriteHashKey(void)
{
    uint16_t CRCResult;
    uint8_t tempBuff[256];
    uint32_t *pMTPHashKey = (uint32_t *)MTP_HashKey;;
    sprintf((char *)tempBuff, "Convert2Interger : %d\r\n", *pMTPHashKey);
    pCommunicationInterface->writeFunc(tempBuff, strlen((char *)tempBuff));
    sprintf((char *)tempBuff, "MTP Key : 0x%02x 0x%02x 0x%02x 0x%02x\r\n", MTP_HashKey[0], MTP_HashKey[1], MTP_HashKey[2], MTP_HashKey[3]);
    pCommunicationInterface->writeFunc(tempBuff, strlen((char *)tempBuff));
    vTaskDelay(100);
    
    /* enable MTP clock */
	outpw(REG_CLK_PCLKEN1, inpw(REG_CLK_PCLKEN1) | (1 << 26));
	// Dealy 1 tick circle
	vTaskDelay(1);
	TxCounter = 0;
    TxBuffer[TxCounter++] = CMD_MTP_HASH_KEY_RESULT;
    TxBuffer[TxCounter++] = 0x01;
    
    if (MTP_Enable() != MTP_RET_OK) {
        TxBuffer[TxCounter++] = ASCII_NAK;
    }
    else
    {
        //MTP_Program(pMTPHashKey, MTP_OptionData);
        //while (MTP->STATUS & MTP_STATUS_BUSY);
        TxBuffer[TxCounter++] = ASCII_ACK;
    }
    
    // Calculate CRC Result
    CRCResult = CRCTool_TableFast(TxBuffer, TxCounter);
    TxBuffer[TxCounter++] = (uint8_t)(CRCResult >> 8);
    TxBuffer[TxCounter++] = (uint8_t)CRCResult;
    pCommunicationInterface->writeFunc(TxBuffer, TxCounter);
    
    /* disable MTP clock */
    outpw(REG_CLK_PCLKEN1,(inpw(REG_CLK_PCLKEN1) & ~(0x1 << 26)) | (0x0 << 26));
    
    if (TxBuffer[2] == ASCII_NAK) {
        return FALSE;
    }
    return TRUE;
}

/************************************************************************************
* Function: Wait any commands from host. Terminate condition: check 2nd byte,       *
*           please refer to protocol documents                                      *
* Input   : None.                                                                   *
* Return  : None.                                                                   *
*************************************************************************************/
static void MTP_WaitHostCommand(void)
{
    uint8_t tmpBuffer[64];
    RxCounter = 0;
    memset(RxBuffer, 0x00, sizeof(RxBuffer));
    memset(tmpBuffer, 0x00, sizeof(tmpBuffer));
    
    while (1)
    {
        BaseType_t reval = pCommunicationInterface->readWaitFunc(portMAX_DELAY);
        //BaseType_t reval = RS232CommDrvReadWait(portMAX_DELAY);
        vTaskDelay(10 / portTICK_RATE_MS);  //vTaskDelay(100 / portTICK_RATE_MS);
        
        int retval = pCommunicationInterface->readFunc(tmpBuffer, sizeof(tmpBuffer));
        //int retval = RS232CommDrvRead(tmpBuffer, sizeof(tmpBuffer));
        if(retval > 0)
        {
            memcpy(&RxBuffer[RxCounter], tmpBuffer, retval);
            RxCounter = RxCounter + retval;
            if ((RxCounter > 2) && (RxCounter >= ((int)RxBuffer[1] + 4)))
            {
                isStartFlag = TRUE;
                break;
            }
        }
    }
    //pCommunicationInterface->writeFunc(RxBuffer, RxCounter);
}

/************************************************************************************
* Function: Parser any commands from host.                                          *
* Input   : None.                                                                   *
* Return  : Boolean: If any errors will return FALSE.                               *
*************************************************************************************/
static BOOL MTP_ParseHostCommand(void)
{
    uint8_t CRCBuffer[2];
    uint16_t CRCResult;
    
    if (RxCounter < 4) {
        return FALSE;
    }
    
    // Calculate & Compare CRC Result
    CRCResult = CRCTool_TableFast(RxBuffer, RxCounter - 2);
    CRCBuffer[0] = (uint8_t)(CRCResult >> 8);
    CRCBuffer[1] = (uint8_t)CRCResult;
    //pCommunicationInterface->writeFunc(CRCBuffer, 2);
    if ((CRCBuffer[0] != RxBuffer[RxCounter - 2]) || (CRCBuffer[1] != RxBuffer[RxCounter - 1])) {
        return FALSE;
    }
    
    // Decrypted Message
    if (RxBuffer[0] == CMD_INITAIL_AUTHENTICATE)
    {
        uint8_t Random_A[RANDOM_NUMBER_LENGTH], Random_B[RANDOM_NUMBER_LENGTH];
        
        if (RxBuffer[1] != (RANDOM_NUMBER_LENGTH + 1)) {        // Wrong Message Length
            return FALSE;
        }
        if (RxBuffer[2] > MAXIMUM_KEY_INDEX) {                  // Wrong Security Key Index
            return FALSE;
        }
        CryptoService(CRYPTO_MODE_DECRYPT, &RxBuffer[3], RANDOM_NUMBER_LENGTH, &SECURITY_KEY[RxBuffer[2]][0], TRUE);
        memcpy(Random_A, CryptoOutputCipher, RANDOM_NUMBER_LENGTH);
        
        RandomNumberService(PRNG_KEY_SIZE_64, Random_B);
        MTP_BuildAuthenSessionKey(Random_A, Random_B);
        //pCommunicationInterface->writeFunc(AuthenSessionKey, sizeof(AuthenSessionKey));
        //vTaskDelay(100);
        MTP_SendMutualAuthenticate(Random_B);
    }
    else if (RxBuffer[0] == CMD_WRITE_MTP_HASH_KEY)
    {
        uint8_t DecryptedMessage[WRITE_MTP_MESSAGE_LENGTH];
        
        if (EnableAuthenSessionKeyFlag == FALSE) {
            return FALSE;
        }
        if (RxBuffer[1] != WRITE_MTP_MESSAGE_LENGTH) {          // Wrong Message Length
            return FALSE;
        }
        
        CryptoService(CRYPTO_MODE_DECRYPT, &RxBuffer[2], WRITE_MTP_MESSAGE_LENGTH, AuthenSessionKey, FALSE);
        memcpy(DecryptedMessage, CryptoOutputCipher, WRITE_MTP_MESSAGE_LENGTH);
        if (memcmp(MTP_MessageHeader, &DecryptedMessage[0], MTP_MESSAGE_HEADER_LENGTH) != 0) {
            return FALSE;
        }
        if (memcmp(MTP_MessageTail, &DecryptedMessage[MTP_MESSAGE_HEADER_LENGTH + MTP_HASHKEY_LENGTH + 1], MTP_MESSAGE_TAIL_LENGTH) != 0) {
            return FALSE;
        }
        
        memcpy(MTP_HashKey, &DecryptedMessage[MTP_MESSAGE_HEADER_LENGTH], MTP_HASHKEY_LENGTH);
        MTP_OptionData = DecryptedMessage[MTP_MESSAGE_HEADER_LENGTH + MTP_HASHKEY_LENGTH];
        if (MTP_ExecuteWriteHashKey() == FALSE) {
            return FALSE;
        }
    }
    else if (RxBuffer[0] == CMD_SWITCH_PARAMETER_MODE)
    {
        enabledSwitchRTCMode = TRUE;
        RTC_CommonReplyCommand(ASCII_ACK);
    }
    else if (RxBuffer[0] == CMD_UPDATE_RTC_REQUEST)
    {
        memcpy(RTCBuffer, &RxBuffer[2], UPDATE_RTC_MESSAGE_LENGTH);
        if (RTC_ExecuteUpdateRTCTime()) {
            RTC_CommonReplyCommand(ASCII_ACK);
        }
        else {
            RTC_CommonReplyCommand(ASCII_NAK);
        }
    }
    else if (RxBuffer[0] == CMD_SET_DEVICE_PARAMETER_REQUEST)
    {
        memcpy(DeviceParameterBuffer, &RxBuffer[2], SET_DEVICE_PARAMETER_MESSAGE_LENGTH);
        if (RTC_ExecuteSetDeviceID()) {
            RTC_CommonReplyCommand(ASCII_ACK);
        }
        else {
            RTC_CommonReplyCommand(ASCII_NAK);
        }
    }
    else {
        return FALSE;
    }
    return TRUE;
}

/*-----------------------------------------*/
/* Exported Functions                      */
/*-----------------------------------------*/

BOOL MTP_ProcedureInit(void)
{
    //pCommunicationInterface = CommunicationGetInterface(COMMUNICATION_RS232_INTERFACE_INDEX);
    pCommunicationInterface = CommunicationGetInterface(MTP_RS232_INTERFACE_INDEX);
   //pCommunicationInterface = CommunicationGetInterface(0);
    if (pCommunicationInterface == NULL)
    {
        //terninalPrintf("CommunicationGetInterface(MTP_RS232_INTERFACE_INDEX) NULL\r\n");
        return FALSE;
    }
    if (pCommunicationInterface->initFunc() == FALSE)
    //if (RS232CommDrvInit() == FALSE)
    {
        //terninalPrintf("CommunicationGetInterface(MTP_RS232_INTERFACE_INDEX) init fail\r\n");
        return FALSE;
    }
    
    /* Enable Crypto clock */
    
    outpw(REG_CLK_HCLKEN, inpw(REG_CLK_HCLKEN) | (1 << 23));
    CryptoInputCipher = (uint8_t *)((uint32_t)CryptoInputCipher_Pool | 0x80000000);
    CryptoOutputCipher = (uint8_t *)((uint32_t)CryptoOutputCipher_Pool | 0x80000000);
    sysInstallISR(HIGH_LEVEL_SENSITIVE | IRQ_LEVEL_1, CRPT_IRQn, (PVOID)CryptoService_IRQHandler);
    sysSetLocalInterrupt(ENABLE_IRQ);
    sysEnableInterrupt(CRPT_IRQn);
    TDES_ENABLE_INT();
    PRNG_ENABLE_INT();
    CRCTool_Init(CRCCode_CRC16);
    
    EnableAuthenSessionKeyFlag = FALSE;
    isStartFlag = FALSE;
    enabledSwitchRTCMode = FALSE;
    return TRUE;
}

BOOL MTP_IsExitProcedure(void)
{
    MTP_WaitHostCommand();
    if (RxBuffer[0] == CMD_MTP_PROCEDURE_EXIT)
    {
        memset(AuthenSessionKey, 0x00, sizeof(AuthenSessionKey));
        EnableAuthenSessionKeyFlag = FALSE;
        return TRUE;
    }
    if (MTP_ParseHostCommand() == FALSE)
    {
        memset(AuthenSessionKey, 0x00, sizeof(AuthenSessionKey));
        EnableAuthenSessionKeyFlag = FALSE;
        return TRUE;
    }
    return FALSE;
}

void MTP_ProcedureDeInit(void)
{
    TDES_DISABLE_INT();
    PRNG_DISABLE_INT();
    /* disable Crypto clock */
    outpw(REG_CLK_HCLKEN,(inpw(REG_CLK_HCLKEN) & ~(0x1 << 23)) | (0x0 << 23));
    // Disabled RDA_IEN bit (Receive Data Available Interrupt Enable Control)
    outpw(REG_UART0_IER, (inpw(REG_UART0_IER) & 0xFFFFFFFE));
}

void MTP_WaitingStartMessage(void)
{
    char msgBuffer[24];
    memset(msgBuffer, 0x00, sizeof(msgBuffer));
    sprintf(msgBuffer, "ReadyForMTPStart\r\n");
    pCommunicationInterface->writeFunc((uint8_t *)msgBuffer, strlen(msgBuffer) + 2);
}

BOOL MTP_GetProcedureFlag(void)
{
    return isStartFlag;
}

BOOL MTP_GetSwitchRTCFlag(void)
{
    return enabledSwitchRTCMode;
}

void CRCTool_Init(int CodingType)
{
    uint32_t bit, crc;
    int i;
    
    switch (CodingType)
    {
        case CRCCode_CRC_CCITT:
            order = 16; direct = 1; polynom = 0x1021; crcinit = 0xFFFF; crcxor = 0x0000; refin = 0; refout = 0;
            break;
        case CRCCode_CRC16:
            order = 16; direct = 1; polynom = 0x8005; crcinit = 0x0000; crcxor = 0x0000; refin = 0; refout = 0;
            break;
        case CRCCode_CRC32:
            order = 32; direct = 1; polynom = 0x4c11db7; crcinit = 0xFFFFFFFF; crcxor = 0xFFFFFFFF; refin = 1; refout = 1;
            break;
    }
    
    // Initialize all variables for seeding and builing based upon the given coding type
    // at first, compute constant bit masks for whole CRC and CRC high bit
    crcmask = ((((uint32_t)1 << (order -1 )) - 1) << 1) | 1;
    crchighbit = (uint32_t)1 << (order - 1);
    
    // generate lookup table
    generate_crc_table();
    
    if (direct == 0)
    {
        crcinit_nondirect = crcinit;
        crc = crcinit;
        for (i = 0 ; i < order ; i++)
        {
            bit = crc & crchighbit;
            crc <<= 1;
            if (bit != 0) {
                crc ^= polynom;
            }
        }
        crc &= crcmask;
        crcinit_direct = crc;
    }
    else
    {
        crcinit_direct = crcinit;
        crc = crcinit;
        for (i = 0 ; i < order ; i++)
        {
            bit = crc & 1;
            if (bit != 0) {
                crc ^= polynom;
            }
            crc >>= 1;
            if (bit != 0) {
                crc |= crchighbit;
            }
        }
        crcinit_nondirect = crc;
    }
}

/// <summary>
/// 4 ways to calculate the crc checksum. If you have to do a lot of encoding
/// you should use the table functions. Since they use precalculated values, which 
/// saves some calculating.
/// </summary>.

// fast lookup table algorithm without augmented zero bytes, e.g. used in pkzip.
// only usable with polynom orders of 8, 16, 24 or 32.
uint32_t CRCTool_TableFast(uint8_t *p, int p_len)
{
    uint32_t crc = crcinit_direct;
    int i;
    
    if (refin != 0) {
        crc = reflect(crc, order);
    }
    if (refin == 0)
    {
        for (i = 0 ; i < p_len ; i++ ) {
            crc = (crc << 8) ^ crctab[((crc >> (order - 8)) & 0xff) ^ p[i]];
        }
    }
    else
    {
        for (i = 0 ; i < p_len ; i++) {
            crc = (crc >> 8) ^ crctab[(crc & 0xff) ^ p[i]];
        }
    }
    if ((refout ^ refin) != 0) {
        crc = reflect(crc, order);
    }
    crc ^= crcxor;
    crc&= crcmask;
    return crc;
}

// normal lookup table algorithm with augmented zero bytes.
// only usable with polynom orders of 8, 16, 24 or 32.
uint32_t CRCTool_Table(uint8_t *p, int p_len)
{
    uint32_t crc = crcinit_nondirect;
    int i;
    
    if (refin != 0) {
        crc = reflect(crc, order);
    }
    if (refin == 0)
    {
        for (i = 0 ; i < p_len ; i++) {
            crc = ((crc << 8) | p[i]) ^ crctab[(crc >> (order - 8)) & 0xff];
        }
    }
    else
    {
        for (i = 0 ; i < p_len ; i++) {
            crc = (uint32_t)(((uint32_t)(crc >> 8) | (p[i] << (order - 8))) ^ (int)crctab[crc & 0xff]);
        }
    }
    if (refin == 0)
    {
        for (i = 0 ; i < order / 8 ; i++) {
            crc = (crc << 8) ^ crctab[(crc >> (order - 8)) & 0xff];
        }
    }
    else
    {
        for (i = 0 ; i < order / 8 ; i++) {
            crc = (crc >> 8) ^ crctab[crc & 0xff];
        }
    }
    
    if ((refout ^ refin) != 0) {
        crc = reflect(crc, order);
    }
    crc ^= crcxor;
    crc &= crcmask;
    return crc;
}
