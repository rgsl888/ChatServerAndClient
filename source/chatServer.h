/***************************************************************************/
/*****                                                                  ****/
/***** Texas A&M University                                             ****/
/***** Developer : Ramakrishna Prabhu                                   ****/
/***** UIN       : 725006454                                            ****/
/***** Filename  : chatServer.h                                         ****/
/*****                                                                  ****/
/***************************************************************************/

#ifndef __CHATSERVER_H__
#define __CHATSERVER_H__

#define MAX_PORT_NUMBER 65535
#define MAX_PORT_NUMBER_SIZE 6
#define MAX_NUM_CONNECTION 200
#define MAX_USERNAME_LEN 16

struct UserInfo
{
    int fd;
    char username[MAX_USERNAME_LEN];
};

// Funcions
void printErrMsg (const char* funcName);
int getEmptyUserBlock (struct UserInfo *user, int maxClients);
int isUsrNameInUse (struct UserInfo *user, char* username, int maxClients);
void formNakMsg (struct SBCPMsg *sbcpMsg, char* reason);
void formOfflineMsg (struct SBCPMsg *sbcpMsg, char* userName, int numOfConnClients);
void formOnLineMsg (struct SBCPMsg *sbcpMsg, char* userName, int numOfConnClients);
void formAckMsg (struct SBCPMsg *sbcpMsg, int numOfConnClients, struct UserInfo *user, int maxClients);
void sendMsgToAll (struct SBCPMsg *sbcpMsg, int fdIndex, struct UserInfo* user, int maxClients);
void removeUserName (struct UserInfo *user);

#endif
