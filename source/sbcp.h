/***************************************************************************/
/*****                                                                  ****/
/***** Texas A&M University                                             ****/
/***** Developer : Ramakrishna Prabhu                                   ****/
/***** UIN       : 725006454                                            ****/
/***** Filename  : sbcp.h                                               ****/
/*****                                                                  ****/
/***************************************************************************/

#ifndef __SBCP_H__
#define __SBCP_H_

#define SBCP_VRN 3
#define MAX_USERNAME_LEN 16
#define MAX_REASON_LEN 32
#define MAX_MSG_LEN 512

union SBCPAttrFlds
{
    char username [MAX_REASON_LEN];
    char msg[MAX_MSG_LEN];
    char reason[MAX_REASON_LEN];
    short int clientCount;
};

struct SBCPAttr
{
    short int type;
    short int len;
    union SBCPAttrFlds payload;
};

struct SBCPMsg
{
    unsigned int vrsn : 9;
    unsigned int type : 7;
    unsigned int len  : 16;
    struct SBCPAttr attr_0;
    struct SBCPAttr attr_1;
};

enum SbcpType {
    eJOIN = 2,
    eFORWARD,     // type - 3
    eSEND,        // type - 4
    eNAK,         // type - 5 
    eOFFLINE,     // type - 6
    eACK,         // type - 7
    eONLINE,      // type - 8
    eIDLE         // type - 9      
};

enum SbcpAttrType {
    eREASON = 1, 
    eUSERNAME,    // Type - 2
    eCLIENTCOUNT, // Type - 3
    eMESSAGE      // Type - 4
};
#endif
