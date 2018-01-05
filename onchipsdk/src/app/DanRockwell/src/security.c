/*
 *	Copyright (c) 2013, Dust Networks.  All rights reserved.
 */

#include "dn_common.h"
#include "string.h"
#include "stdio.h"
#include "cli_task.h"
#include "loc_task.h"
#include "dn_security.h"
#include "dn_exe_hdr.h"
#include "Ver.h"
#include "dnm_ucli.h"


//=========================== prototypes ======================================

dn_error_t cli_cbcCmdHandler(const char* arg, unsigned len);
dn_error_t cli_ctrCmdHandler(const char* arg, unsigned len);
dn_error_t cli_ccm1CmdHandler(const char* arg, unsigned len);
dn_error_t cli_ccm2CmdHandler(const char* arg, unsigned len);
dn_error_t cli_ccm3CmdHandler(const char* arg, unsigned len);


//=========================== const ===========================================

// Test vectors for the CTR and CBC-MAC modes
const unsigned char security_key[] =
{
   0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 
   0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,
};

const unsigned char security_inBuf[] =
{
   0x00, 0x11, 0x22, 0x34, 0x44, 0x55, 0x66, 0x77,
   0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff,
};

// Test vectors for the CCM* mode
// Section "C.2.1 MAC beacon frame": authentication only (8-byte MIC)
// Buffer to authenticate only
const unsigned char ccm1_a[] =
{
   0x08, 0xD0, 0x84, 0x21, 0x43, 0x01, 0x00, 0x00,
   0x00, 0x00, 0x48, 0xDE, 0xAC, 0x02, 0x05, 0x00,
   0x00, 0x00, 0x55, 0xCF, 0x00, 0x00, 0x51, 0x52,
   0x53, 0x54
};

// Buffer to authenticate and encrypt
const unsigned char ccm1_nonce[] =
{
   0xAC, 0xDE, 0x48, 0x00, 0x00, 0x00, 0x00, 0x01,
   0x00, 0x00, 0x00, 0x05, 0x02
};

// Section "C.2.2 MAC data frame": encryption only
// Buffer to authenticate only
const unsigned char ccm2_a[] =
{
   0x69, 0xDC, 0x84, 0x21, 0x43, 0x02, 0x00, 0x00,
   0x00, 0x00, 0x48, 0xDE, 0xAC, 0x01, 0x00, 0x00,
   0x00, 0x00, 0x48, 0xDE, 0xAC, 0x04, 0x05, 0x00,
   0x00, 0x00
};

// Buffer to authenticate and encrypt
const unsigned char ccm2_m[] =
{
   0x61, 0x62, 0x63, 0x64
};

const unsigned char ccm2_nonce[] =
{
   0xAC, 0xDE, 0x48, 0x00, 0x00, 0x00, 0x00, 0x01,
   0x00, 0x00, 0x00, 0x05, 0x04
};

// Section "C.2.3 MAC command frame": encryption and authentication (8-byte MIC)
// Buffer to authenticate only
const unsigned char ccm3_a[] =
{
   0x2B, 0xDC, 0x84, 0x21, 0x43, 0x02, 0x00, 0x00,
   0x00, 0x00, 0x48, 0xDE, 0xAC, 0xFF, 0xFF, 0x01,
   0x00, 0x00, 0x00, 0x00, 0x48, 0xDE, 0xAC, 0x06,
   0x05, 0x00, 0x00, 0x00, 0x01
};

// Buffer to authenticate and encrypt
const unsigned char ccm3_m[] =
{
   0xCE
};

const unsigned char ccm3_nonce[] =
{
   0xAC, 0xDE, 0x48, 0x00, 0x00, 0x00, 0x00, 0x01,
   0x00, 0x00, 0x00, 0x05, 0x06
};


const dnm_ucli_cmdDef_t cliCommandDefs[] = 
{
   {&cli_ctrCmdHandler,      "ctr",    "",       DN_CLI_ACCESS_LOGIN},
   {&cli_cbcCmdHandler,      "cbc",    "",       DN_CLI_ACCESS_LOGIN},
   {&cli_ccm1CmdHandler,     "ccm1",   "",       DN_CLI_ACCESS_LOGIN},
   {&cli_ccm2CmdHandler,     "ccm2",   "",       DN_CLI_ACCESS_LOGIN},
   {&cli_ccm3CmdHandler,     "ccm3",   "",       DN_CLI_ACCESS_LOGIN},
   {NULL,                    NULL,     NULL,     DN_CLI_ACCESS_NONE},
};


//=========================== variables =======================================

// Variables local to this application.
typedef struct 
{
	unsigned char           inBuf[16];
	unsigned char           outBuf[16];
	unsigned char           key[16];
	unsigned char           iv[16];
	unsigned char           mic[16];
} security_app_vars_t;

security_app_vars_t security_app_vars;


//=========================== initialization ==================================

/**
 *	This is the entry point for the application code.
 */
int InitSecurity(void) 
{
   // Initialize helper tasks
   cli_task_init(
      "security",                           // appName
      cliCommandDefs            
   );
   
   // local interface task
   loc_task_init(
      JOIN_NO,                              // fJoin
      NETID_NONE,                           // netId
      UDPPORT_NONE,                         // udpPort
      NULL,                                 // joinedSem
      BANDWIDTH_NONE,                       // bandwidth
      NULL                                  // serviceSem
   );
   
   return 0;
}


/**
 *	ctr': CTR AES mode
 */
dn_error_t cli_ctrCmdHandler(const char* arg, unsigned len)
{
   // prepare paramaters
   memcpy(security_app_vars.inBuf,     &security_inBuf,    16);
   memcpy(security_app_vars.key,       &security_key,      16);
   memset(security_app_vars.iv,        0,                  16);
   
   // print
   dnm_ucli_printf("params:\r\r\n");
   dnm_ucli_printf(" - key:        ");
   dnm_ucli_printBuf(security_app_vars.key,     sizeof(security_app_vars.key));
   dnm_ucli_printf(" - iv:         ");
   dnm_ucli_printBuf(security_app_vars.iv,      sizeof(security_app_vars.iv));
   
   // print
   dnm_ucli_printf("input:\r\r\n");
   dnm_ucli_printf(" - plaintext:  ");
   dnm_ucli_printBuf(security_app_vars.inBuf,   sizeof(security_app_vars.inBuf));
   
   // encrypt
   dn_sec_aesCtr(
      security_app_vars.inBuf,         // inBuf
      security_app_vars.outBuf,        // outBuf
      sizeof(security_app_vars.inBuf), // len
      security_app_vars.key,           // key
      security_app_vars.iv             // iv
   );
   
   // print
   dnm_ucli_printf("output:\r\r\n");
   dnm_ucli_printf(" - ciphertext: ");
   dnm_ucli_printBuf(security_app_vars.outBuf,  sizeof(security_app_vars.outBuf));
   
   security_app_vars.outBuf[5] ^= 0x01;
   
   // decrypt (same operation as encrypt, inBuf and outBuf inverted)
   dn_sec_aesCtr(
      security_app_vars.outBuf,        // inBuf
      security_app_vars.inBuf,         // outBuf
      sizeof(security_app_vars.inBuf), // len
      security_app_vars.key,           // key
      security_app_vars.iv             // iv
   );
   
   // print
   dnm_ucli_printf("output:\r\r\n");
   dnm_ucli_printf(" - decrypted:  ");
   dnm_ucli_printBuf(security_app_vars.inBuf,  sizeof(security_app_vars.inBuf));
   
   return DN_ERR_NONE;
}


/*
 *	'cbc': CBC-MAC AES mode
 */
dn_error_t cli_cbcCmdHandler(const char* arg, unsigned len)
{
   // prepare paramaters
   memcpy(security_app_vars.inBuf,     &security_inBuf,    16);
   memcpy(security_app_vars.key,       &security_key,      16);
   memset(security_app_vars.iv,        0,                  16);
   
   // print
   dnm_ucli_printf("params:\r\r\n");
   dnm_ucli_printf(" - key:        ");
   dnm_ucli_printBuf(security_app_vars.key,     sizeof(security_app_vars.key));
   dnm_ucli_printf(" - iv:         ");
   dnm_ucli_printBuf(security_app_vars.iv,      sizeof(security_app_vars.iv));
   
   // print
   dnm_ucli_printf("input:\r\r\n");
   dnm_ucli_printf(" - inBuf:      ");
   dnm_ucli_printBuf(security_app_vars.inBuf,   sizeof(security_app_vars.inBuf));
   
   // create MIC
   dn_sec_aesCbcMac(
      security_app_vars.inBuf,         // inBuf
      sizeof(security_app_vars.inBuf), // len
      security_app_vars.key,           // key
      security_app_vars.iv,            // iv
      security_app_vars.mic            // mic
   );
   
   // print
   dnm_ucli_printf("output:\r\r\n");
   dnm_ucli_printf(" - mic:        ");
   dnm_ucli_printBuf(security_app_vars.mic,     sizeof(security_app_vars.mic));
   
   return DN_ERR_NONE;
}


/**
 *	'ccm': CCM* AES mode
 */
dn_error_t cli_ccm1CmdHandler(const char* arg, unsigned len)
{
   dn_sec_ccmopt_t	dn_sec_ccmopt;
   unsigned char	aBuf[sizeof(ccm1_a)];
   
   dnm_ucli_printf("Test vector from \"C.2.1 MAC beacon frame\"\r\r\n");
   
   // Size of CCM authentication field (MIC); range: 4-16.
   dn_sec_ccmopt.M = 8;
   // Size of CCM length field; range: 2-8.
   dn_sec_ccmopt.L = 2;
   // buffer to authenticate only
   memcpy(aBuf,                        &ccm1_a,            sizeof(ccm1_a));
   // key
   memcpy(security_app_vars.key,       &security_key,      sizeof(security_key));
   // nonce/initialization vector
   memcpy(security_app_vars.iv,        &ccm1_nonce,        sizeof(ccm1_nonce));
   
   // print
   dnm_ucli_printf("params:\r\r\n");
   dnm_ucli_printf(" - M:          %d\r\r\n",dn_sec_ccmopt.M);
   dnm_ucli_printf(" - L:          %d\r\r\n",dn_sec_ccmopt.L);
   dnm_ucli_printf(" - key:       ");
   dnm_ucli_printBuf(security_app_vars.key,     sizeof(security_app_vars.key));
   dnm_ucli_printf(" - nonce:     ");
   dnm_ucli_printBuf(security_app_vars.iv,      sizeof(security_app_vars.iv));
   
   // print
   dnm_ucli_printf("input:\r\r\n");
   dnm_ucli_printf(" - aBuf:      ");
   dnm_ucli_printBuf(aBuf,                      sizeof(aBuf));
   
   // encrypt and authenticate
   dn_sec_aesCcmEncrypt(
      &dn_sec_ccmopt,             // opt
      aBuf,                       // aBuf
      sizeof(aBuf),               // aLen
      NULL,                       // mBuf
      0,                          // mLen
      security_app_vars.key,      // key
      security_app_vars.iv,       // nonce
      security_app_vars.mic       // mic
   );
   
   // print
   dnm_ucli_printf("output:\r\r\n");
   dnm_ucli_printf(" - aBuf:      ");
   dnm_ucli_printBuf(aBuf,                      sizeof(aBuf));
   dnm_ucli_printf(" - mic:       ");
   dnm_ucli_printBuf(security_app_vars.mic,     8);
   
   return DN_ERR_NONE;
}


dn_error_t cli_ccm2CmdHandler(const char* arg, unsigned len)
{
   dn_sec_ccmopt_t	dn_sec_ccmopt;
   unsigned char	aBuf[sizeof(ccm2_a)];
   unsigned char	mBuf[sizeof(ccm2_m)];
   
   dnm_ucli_printf("Test vector from \"C.2.2 MAC data frame\"\r\r\n");
   
   // Size of CCM authentication field (MIC); range: 4-16.
   dn_sec_ccmopt.M = 4;
   // Size of CCM length field; range: 2-8.
   dn_sec_ccmopt.L = 2;
   // buffer to authenticate only
   memcpy(aBuf,                        &ccm2_a,            sizeof(ccm2_a));
   // buffer to authenticate and encrypt
   memcpy(mBuf,                        &ccm2_m,            sizeof(ccm2_m));
   // key
   memcpy(security_app_vars.key,       &security_key,      sizeof(security_key));
   // nonce/initialization vector
   memcpy(security_app_vars.iv,        &ccm2_nonce,        sizeof(ccm1_nonce));
   
   // print
   dnm_ucli_printf("params:\r\r\n");
   dnm_ucli_printf(" - M:          %d\r\r\n",dn_sec_ccmopt.M);
   dnm_ucli_printf(" - L:          %d\r\r\n",dn_sec_ccmopt.L);
   dnm_ucli_printf(" - key:       ");
   dnm_ucli_printBuf(security_app_vars.key,     sizeof(security_app_vars.key));
   dnm_ucli_printf(" - nonce:     ");
   dnm_ucli_printBuf(security_app_vars.iv,      sizeof(security_app_vars.iv));
   
   // print
   dnm_ucli_printf("input:\r\r\n");
   dnm_ucli_printf(" - aBuf:      ");
   dnm_ucli_printBuf(aBuf,                      sizeof(aBuf));
   dnm_ucli_printf(" - mBuf:      ");
   dnm_ucli_printBuf(mBuf,                      sizeof(mBuf));
   
   // encrypt and authenticate
   dn_sec_aesCcmEncrypt(
      &dn_sec_ccmopt,             // opt
      aBuf,                       // aBuf
      sizeof(aBuf),               // aLen
      mBuf,                       // mBuf
      sizeof(mBuf),               // mLen
      security_app_vars.key,      // key
      security_app_vars.iv,       // nonce
      security_app_vars.mic       // mic
   );
    
   // print
   dnm_ucli_printf("output:\r\r\n");
   dnm_ucli_printf(" - aBuf:      ");
   dnm_ucli_printBuf(aBuf,                      sizeof(aBuf));
   dnm_ucli_printf(" - mBuf:      ");
   dnm_ucli_printBuf(mBuf,                      sizeof(mBuf));
   
   return DN_ERR_NONE;
}


dn_error_t cli_ccm3CmdHandler(const char* arg, unsigned len)
{
   dn_sec_ccmopt_t	dn_sec_ccmopt;
   unsigned char	aBuf[sizeof(ccm3_a)];
   unsigned char	mBuf[sizeof(ccm3_m)];
   
   dnm_ucli_printf("Test vector from \"C.2.3 MAC command frame\"\r\r\n");
   
   // Size of CCM authentication field (MIC); range: 4-16.
   dn_sec_ccmopt.M = 8;
   // Size of CCM length field; range: 2-8.
   dn_sec_ccmopt.L = 2;
   // buffer to authenticate only
   memcpy(aBuf,                        &ccm3_a,            sizeof(ccm3_a));
   // buffer to authenticate and encrypt
   memcpy(mBuf,                        &ccm3_m,            sizeof(ccm3_m));
   // key
   memcpy(security_app_vars.key,       &security_key,      sizeof(security_key));
   // nonce/initialization vector
   memcpy(security_app_vars.iv,        &ccm3_nonce,        sizeof(ccm3_nonce));
   
   // print
   dnm_ucli_printf("params:\r\r\n");
   dnm_ucli_printf(" - M:          %d\r\r\n",dn_sec_ccmopt.M);
   dnm_ucli_printf(" - L:          %d\r\r\n",dn_sec_ccmopt.L);
   dnm_ucli_printf(" - key:       ");
   dnm_ucli_printBuf(security_app_vars.key,     sizeof(security_app_vars.key));
   dnm_ucli_printf(" - nonce:     ");
   dnm_ucli_printBuf(security_app_vars.iv,      sizeof(security_app_vars.iv));
   
   // print
   dnm_ucli_printf("input:\r\r\n");
   dnm_ucli_printf(" - aBuf:      ");
   dnm_ucli_printBuf(aBuf,                      sizeof(aBuf));
   dnm_ucli_printf(" - mBuf:      ");
   dnm_ucli_printBuf(mBuf,                      sizeof(mBuf));
   
   // encrypt and authenticate
   dn_sec_aesCcmEncrypt(
      &dn_sec_ccmopt,             // opt
      aBuf,                       // aBuf
      sizeof(aBuf),               // aLen
      mBuf,                       // mBuf
      sizeof(mBuf),               // mLen
      security_app_vars.key,      // key
      security_app_vars.iv,       // nonce
      security_app_vars.mic       // mic
   );
   
   // print
   dnm_ucli_printf("output:\r\r\n");
   dnm_ucli_printf(" - aBuf:      ");
   dnm_ucli_printBuf(aBuf,                      sizeof(aBuf));
   dnm_ucli_printf(" - mBuf:      ");
   dnm_ucli_printBuf(mBuf,                      sizeof(mBuf));
   dnm_ucli_printf(" - mic:       ");
   dnm_ucli_printBuf(security_app_vars.mic,     8);
   
   return DN_ERR_NONE;
}
