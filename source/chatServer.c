/****************************************************************************/
/*****                                                                   ****/
/***** Texas A&M University                                              ****/
/***** Developer : Ramakrishna Prabhu                                    ****/
/***** UIN       : 725006454                                             ****/
/***** Filename  : chatServer.c                                          ****/
/*****                                                                   ****/
/****************************************************************************/

#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>
#include <sys/select.h>
#include "sbcp.h"
#include "chatServer.h" 

// Description : Prints the error message as per errnno
// Parameters  : funcName - Name of the funcrion which returned error
// Return     : NONE
void printErrMsg (const char* funcName)
{
    char *errMsg = NULL;

    // strerror function gets appropriate error message as per the errno set
    errMsg = strerror (errno);
    if (NULL != funcName)
    {
        printf ("\n!!!!    '%s' returned with error \n", funcName);
    }
    printf ("\n!!!!    Error : %s  !!!!\n", errMsg);

    return;
}


// Description : Gets a free slot for the user
// Parameters  : user - A pointer to UserInfo which holds all the connected user database
//               maxClients - Maximum number of clients allowed to connect
// Return      : If found then index number else -1
int getEmptyUserBlock (struct UserInfo *user, int maxClients)
{
    int index = 1;

    if (user != NULL)
    {
        for (index = 1; index < maxClients + 1 + 1; index++)
        {
            if (user[index].fd == 0)
            {
                return index;
            }
        }
    }

    return -1;
}

// Description : Checks wether user name is in use
// Parameters  : user - A pointer to UserInfo which holds all the connected user database
//               userName - A pointer to the user name provided by the client in JOIN message
//               maxClients - Maximum number of clients allowed to connect
// Return     :  1 - If username exist
//               0 - If username doesn't exist
int isUsrNameInUse (struct UserInfo *user, char* username, int maxClients)
{
    int index = 0;
    int ret = 0;

    if ((user != NULL) && (username != NULL))
    {
    	for (index = 1; index < maxClients + 1; index++)
    	{
	    if (0 == strcmp (username, user[index].username))
	    {
            	ret = 1;
            	break;
	    }
    	}
    }
    else
    {
        printf ("[%s] Error : username/usernamelist passed was null \n", __FUNCTION__);
    }

    return ret;
}


// Description : Form the NAK message
// Parameters  : sbcpMsg - Pointer to SBCP message structure
//               reason - A pointer to a string which has the reason stored for NAK
// Return      : NONE 
void formNakMsg (struct SBCPMsg *sbcpMsg, char* reason)
{
   if ((NULL != sbcpMsg) && (NULL != reason)) 
   {
      sbcpMsg->vrsn = SBCP_VRN;
      sbcpMsg->type = eNAK;
      sbcpMsg->len = sizeof (struct SBCPMsg);
      // Attributes will be created/added as per ascending order of the type
      sbcpMsg->attr_0.type = eREASON;
      sbcpMsg->attr_0.len  = sizeof (struct SBCPAttr);
      snprintf (sbcpMsg->attr_0.payload.reason, sizeof (sbcpMsg->attr_0.payload.reason), "%s", reason);
   }
}

// Description : Form the Offline message
// Parameters  : sbcpMsg - Pointer to SBCP message structure
//               userName - A pointer of  type char which holds the userName of the disconnected client
//               numOfConnClients - Number of clients connected
// Return      : NONE 
void formOfflineMsg (struct SBCPMsg *sbcpMsg, char* userName, int numOfConnClients)
{
   if ((NULL != sbcpMsg) && (NULL != userName)) 
   {
      sbcpMsg->vrsn = SBCP_VRN;
      sbcpMsg->type = eOFFLINE;
      sbcpMsg->len = sizeof (struct SBCPMsg);
      // Attributes will be created/added as per ascending order of the type
      sbcpMsg->attr_0.type = eUSERNAME;
      sbcpMsg->attr_0.len  = sizeof (struct SBCPAttr);
      snprintf (sbcpMsg->attr_0.payload.username, sizeof (sbcpMsg->attr_0.payload.username), "%s", userName);
      sbcpMsg->attr_1.type = eCLIENTCOUNT;
      sbcpMsg->attr_1.len  = sizeof (struct SBCPAttr);
      sbcpMsg->attr_1.payload.clientCount = numOfConnClients;
   }
}

// Description : Form the Online message
// Parameters  : sbcpMsg - Pointer to SBCP message structure
//               userName - A pointer of  type char which holds the userName of the connected client
//               numOfConnClients - Number of clients connected
// Return      : NONE 
void formOnLineMsg (struct SBCPMsg *sbcpMsg, char* userName, int numOfConnClients)
{
   if ((NULL != sbcpMsg) && (NULL != userName))
   {
      sbcpMsg->vrsn = SBCP_VRN;
      sbcpMsg->type = eONLINE;
      sbcpMsg->len = sizeof (struct SBCPMsg);
      // Attributes will be created/added as per ascending order of the type
      sbcpMsg->attr_0.type = eUSERNAME;
      sbcpMsg->attr_0.len  = sizeof (struct SBCPAttr);
      snprintf (sbcpMsg->attr_0.payload.username, sizeof (sbcpMsg->attr_0.payload.username), "%s", userName);
      sbcpMsg->attr_1.type = eCLIENTCOUNT;
      sbcpMsg->attr_1.len  = sizeof (struct SBCPAttr);
      sbcpMsg->attr_1.payload.clientCount = numOfConnClients;
   }
}

// Description : Form the ACK message
// Parameters  : sbcpMsg - Pointer to SBCP message structure
//               numOfConnClients - Number of clients connected
//               user - A pointer to UserInfo which holds all the connected user database
//               maxClients - Maximum number of clients allowed to connect
// Return      : NONE 
void formAckMsg (struct SBCPMsg *sbcpMsg, int numOfConnClients, struct UserInfo *user, int maxClients)
{
   char userNames [512] = {0};
   int index = 0;

   if ((NULL != sbcpMsg) && (NULL != user))
   { 
       for (index = 1; index < maxClients; index++)
       {
           if (strlen (user[index].username ) > 0)
           {
               if (0 == strlen (userNames))
               {
                   snprintf (userNames, sizeof (userNames), "%s", user[index].username);
               }
               else
               {
                   strncat (userNames, ", ", sizeof(userNames));
                   strncat (userNames, user[index].username, sizeof(userNames));
               }
           }
       }

       sbcpMsg->vrsn = SBCP_VRN;
       sbcpMsg->type = eACK;
       sbcpMsg->len = sizeof (struct SBCPMsg);
       // Attributes will be created/added as per ascending order of the type
       sbcpMsg->attr_0.type = eCLIENTCOUNT;
       sbcpMsg->attr_0.len  = sizeof (struct SBCPAttr);
       sbcpMsg->attr_0.payload.clientCount = numOfConnClients;
       sbcpMsg->attr_1.type = eMESSAGE;
       sbcpMsg->attr_1.len  = sizeof (struct SBCPAttr);
       snprintf (sbcpMsg->attr_1.payload.msg, sizeof (sbcpMsg->attr_1.payload.msg), "%s", userNames);
   }
}

// Description : Forwards/Sends the messages to all clients
// Parameters  : sbcpMsg - Pointer to SBCP message structure
//               fdIndex - Index of the user who sent the message
//               user - A pointer to UserInfo which holds all the connected user database
//               maxClients - Maximum number of clients allowed to connect
// Return      : NONE 
void sendMsgToAll (struct SBCPMsg *sbcpMsg, int fdIndex, struct UserInfo* user, int maxClients)
{
    int index = 1;

    if ((sbcpMsg != NULL) && (user != NULL))
    {
        for (index = 1; index < maxClients; index++)
        {
            if (user[index].fd > 0)
            {
                if ((index != fdIndex) && (user[index].fd != -1 ))
                {
                    if (write (user[index].fd, sbcpMsg, sizeof (struct SBCPMsg)) < 0)
                    {
                        printErrMsg ("write");
                    }
                }
            }
        }
    }
}

// Description : Remove the username from the database
// Parameters  : user - A pointer to UserInfo which holds all the connected user database
// Return      : NONE 
void removeUserName (struct UserInfo *user)
{
    if ((user != NULL))
    {
        user->fd = 0;
        memset (user->username, 0, sizeof (user->username));
    }
}

int main (int argc, char** argv)
{
    int listenFd = 0;
    int connFd = 0;
    struct sockaddr_in servAddr;
    char port[MAX_PORT_NUMBER_SIZE] = {0};   // Stores the port number in string format
    int ret = 0;                             // Stores return value
    int maxClients = 10;                     // maxClients allowed by server
    int numOfConnClients = 0;                // Number of connected clients at a point
    fd_set readFds;  	         	     // Pointer to store FDs where read needs to be executed
    fd_set activeFds;  	         	     // Pointer to store FDs where read needs to be executed
    int fdIndex = 0;
    int index = 0;
    char reason [MAX_REASON_LEN] = {0};      // Reason for any NAK
    struct SBCPMsg sbcpRecvMsg;
    struct SBCPMsg sbcpSendMsg;
    int userNameLen = 0;
    struct UserInfo user[maxClients + 1 + 1]; // One for additional client and one for listening socket
    int maxsfd = 0;

    
    // As we are expecting the command to be executed in the format "echos PORT_NUMBER" will result in 
    // exactly 2 arguments. If we get number of arguments other than 2, then the format provided is improper.
    if (argc != 3)
    {
        printf ("\nPlease provide the command in the following syntax \n"
                "\nSyntax :\n"
                "           ./chatserver PORT_NUMBER MAX_CLIENTS_ALLOWED\n"
                "\nExample : ./chatserver 8080 10\n");
        return 1;
    }
    else
    {
        // Just an helper option, this will print syntax when echos is executed as "./echos -h"
        if (0 == strcmp (argv [1], "-h"))
        {
            printf ("\nSynatx  : ./chatserver PORT_NUMBER MAX_CLIENTS_ALLOWED\n\n"
                    "\nExample : ./chatserver 8080 10\n");
        
            return 0;
        }

    }

    maxClients = atoi(argv[2]);          // maxClients allowed by server
    
    // Initialize the database where we will maintain the user information
    for (index = 0; index < maxClients + 1 + 1; index++)
    {
        user[index].fd = 0;
        memset (user[index].username, 0, sizeof (user[index].username));
    }

    // socket function call will create and endpoint for communication and returns a descriptor
    if ((listenFd = socket (AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printErrMsg ("socket");
        return 1;
    } 

    memset (&servAddr, '0', sizeof (servAddr));

    if (atoi (argv[1]) > MAX_PORT_NUMBER)
    {
        printf ("Error : Port number provided is beyond the max value 65535 \n"
                "        Please provide a port number which is within the range of 1025-65535 \n");
        return 1;
    }
    else if (atoi (argv[1]) <= 0)
    {
        printf ("Error : Port number provided is less than 1, which is invalid\n"
                "        Please provide a port number which is within the range of 1025-65535 \n");
        return 1;
    }
    
    // Server Information
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = htonl (INADDR_ANY); // Taking local IP
    servAddr.sin_port = htons(atoi(argv[1]));      // PORT number got from command line

    // This for loop is used to take care of the scenario where the PORT number provided is already in use
    while (bind(listenFd, (struct sockaddr*) &servAddr, sizeof (servAddr)) < 0)
    {
        // Check for port already in use error
        if (EADDRINUSE == errno)
        {
            printf ("Port %s is occupied by some other process, please choose any other free port\n", argv [1]);
            printf ("Port : ");
            scanf ("%s", port);
            if (MAX_PORT_NUMBER < atoi (port))
            {
                printf ("Error : Port number provided is beyond the max value 65535 \n"
                        "        Please provide a port number which is within the range of 1025-65535 \n");
                return 1;
            }
            else if (0 >= atoi (port))
            {
                printf ("Error : Port number provided is less than 1, which is invalid\n"
                        "        Please provide a port number which is within the range of 1025-65535 \n");
                return 1;
            }
            servAddr.sin_port = htons(atoi(port));
        }
        else
        {
            printErrMsg ("bind");
            return 1;
        }
    }

    if (listen (listenFd, MAX_NUM_CONNECTION) < 0)
    {
        printErrMsg ("listen");
        return 1;
    }

    // Let the listening socket be in the first position
    user[0].fd = listenFd;


    // From here onwards the server will wait for connection and will accept the incoming connections
    printf ("Server started......\n");
    // Clear the pointers once
    FD_ZERO (&activeFds);
    FD_ZERO (&readFds);
    // Intialize listenFd to keept track of connect request
    FD_SET (listenFd, &activeFds);
    maxsfd = listenFd;
    
    while (1)
    {
        readFds = activeFds;
        if ((ret = select (maxsfd+1, &readFds, NULL, NULL, 0)) < 0)
        {
            // for ret = 0, the select was timedout, so just looping back to wait for some signals in FD
            if (ret != 0)
            {
                printErrMsg("select");
            }
            continue ;
        }
        
	/*Servicing the fds*/
        for (fdIndex = 0; fdIndex < (maxClients +1 +1); fdIndex++)
	{
            if (user[fdIndex].fd > 0)
            if (FD_ISSET (user[fdIndex].fd, &readFds))
            {
                // Getting a new connection request
                if (user[fdIndex].fd == listenFd)
                {
                    // resetting the value
                    connFd = -1;
                    if ((connFd = accept (listenFd, (struct sockaddr*) NULL, NULL)) < 0)
                    {
                        printErrMsg ("accept");
                        // as accept has failed, lets wait for next connection
                        continue;
		    }
                    // Add the new connection to the read FDs, the max client will be checked once we receive JOIN
                    FD_SET (connFd, &activeFds);
                    if (connFd > maxsfd)
                    {
                        maxsfd = connFd;
                    }
                    if ((ret = getEmptyUserBlock(user, maxClients)) > 0)
                    {
                        user[ret].fd = connFd;
                    }
                    else
                    {
                        printf ("Didn't add \n");
                    }
                      
                    continue;
                }
		// Else this an old connection
                else
                {
                    memset (&sbcpRecvMsg, 0, sizeof (sbcpRecvMsg));
                    memset (&sbcpSendMsg, 0, sizeof (sbcpSendMsg));
                    if ((ret = read (user[fdIndex].fd, (void *) &sbcpRecvMsg, sizeof (sbcpRecvMsg))) < 0)
                    {
                        printErrMsg("read");
                        continue;
                    }

                    if (0 == ret)
                    {
                        // The socket is closed, remove the fd and user from the list
                        // Send that the particular user is Offline to all other clients
                        numOfConnClients --;
                        formOfflineMsg (&sbcpRecvMsg, user[fdIndex].username, numOfConnClients);
                        // Check whether close is required or not
                        sendMsgToAll (&sbcpRecvMsg, fdIndex, user, maxClients);
                        FD_CLR(user[fdIndex].fd, &activeFds);
                        removeUserName (&(user[fdIndex]));
                        
                        continue;
                    }
                    if (ret > 0)
                    {
                        switch (sbcpRecvMsg.type)
                        {
                            case eJOIN: 
                            {	
                                // resetting it to zero		
                                ret = 0;
                                if ((numOfConnClients ==  maxClients) || 
                                   (ret = isUsrNameInUse(user, sbcpRecvMsg.attr_0.payload.username, maxClients)) ||
                                   ((userNameLen = strlen (sbcpRecvMsg.attr_0.payload.username)) > 15) || 
                                   (eUSERNAME != sbcpRecvMsg.attr_0.type))
                                {
                                    // Sending NAK as the server is already dealing with maximum number of clients
                                    memset (reason, 0, sizeof (reason));
                                    if (ret == 1)
                                    {
                                        // Form the msg for user name is in use issue
                                        snprintf (reason, sizeof (reason), "%s", "The username is in use\n");
                                    }
                                    else if (userNameLen > 15)
                                    {
                                        // Form a message that the username is too long
                                        snprintf (reason, sizeof (reason), "%s", "The user name should be of max length 15 characters\n");
                                    }
                                    else if (numOfConnClients ==  maxClients)
                                    {
                                        // Form the msg for max clients
                                        snprintf (reason, sizeof (reason), "%s", "No room left for clients\n");
                                    }
                                    else
                                    {
                                        snprintf (reason, sizeof (reason), "%s", "Attribute type was wrong\n");
                                    }
                                    formNakMsg (&sbcpSendMsg, reason);
                                    write (user[fdIndex].fd, (void*) &sbcpSendMsg, sizeof (sbcpSendMsg));
                                    if ((close (user[fdIndex].fd)) < 0)
                                    {
                                        printErrMsg ("close");
                                    }
                                    FD_CLR(user[fdIndex].fd, &activeFds);
                                    removeUserName (&(user[fdIndex]));

                                    continue;
                                }
                                            
                                numOfConnClients ++;
                                // send ack to client
                                memset (user[fdIndex].username, 0, sizeof (user[fdIndex].username));
                                snprintf (user[fdIndex].username, sizeof (user[fdIndex].username), "%s", sbcpRecvMsg.attr_0.payload.username);
                                formAckMsg(&sbcpSendMsg, numOfConnClients, user, maxClients + 1);
                                write (user[fdIndex].fd, (void *)&sbcpSendMsg, sizeof (sbcpSendMsg));
                                // send the online msg the new client to all
                                memset (&sbcpSendMsg, 0, sizeof(&sbcpSendMsg));
                                formOnLineMsg(&sbcpSendMsg, user[fdIndex].username, numOfConnClients);
                                sendMsgToAll (&sbcpSendMsg, fdIndex, user, maxClients + 1);
                                memset (&sbcpSendMsg, 0, sizeof (sbcpSendMsg));
                            
                                break;
                            }

                            case eSEND: 
                            {
                                // Send modify the send msg to forward and send to everyone else
                                sbcpRecvMsg.type = eFORWARD;
                                sendMsgToAll (&sbcpRecvMsg, fdIndex, user, maxClients + 1);
                                memset (&sbcpRecvMsg, 0, sizeof (sbcpRecvMsg));

                                break;
                            }

                            case eIDLE:
                            {
                                // Just forward the IDLE message to all
                                sendMsgToAll (&sbcpRecvMsg, fdIndex, user, maxClients + 1);
                                memset (&sbcpRecvMsg, 0, sizeof (sbcpRecvMsg));
                                break;
                            }

                            default:
                            {
                                printf ("Error: Wrong type of message \n");
                            }
                        }
                    }
		}   
            }
	}
    }

    return 0;
}


