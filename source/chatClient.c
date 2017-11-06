/*****************************************************/
/*****                                            ****/
/***** Texas A&M University                       ****/
/***** Developer : Han Bee Oh                     ****/
/***** UIN       : 826008965                      ****/
/***** Filename  : chatClient.c                   ****/
/*****                                            ****/
/*****************************************************/

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>
#include "sbcp.h"


#define MAXLINE 4096
#define MAXPORTNUMBER 65535

// Description : Prints the error message as per errnno
// Parameters  : funcName - Name of the funcrion which returned error
// Return      : NONE
void printErrMsg (const char* funcName)
{
    char *errMsg = NULL;

    // strerror function gets appropriate error message as per the errno set
    if ( NULL != funcName )
    {
        errMsg = strerror (errno);
	if (NULL != errMsg)
	{
            printf ("\n!!!!    '%s' returned with error \n", funcName);
            printf ("\n!!!!    Error : %s  !!!!\n", errMsg);
	}
    }

    return;
}

// Description : Writes n bytes to server
// Parameters  : fd - socket desciptor we have to write the buffer
//               buffer - to store written data of socket
//               size - number to be written to socket
// Return      : (error) -1, (non-error) size
int writen (int fd, struct SBCPMsg *buffer, int size)
{
    int bytesLeft = 0;      // how many bytes would be go to server
    int bytesWritten = 0;   // to check whether write() function operate well or not
    struct SBCPMsg *tempPtr = buffer;

    bytesLeft = size;
    while (bytesLeft > 0)
    {
        // there is some data to write
        if ((bytesWritten = write (fd, tempPtr, bytesLeft)) <= 0)
        {
            if ((0 > bytesWritten) && (EINTR == errno))
            {
                // As it write was interrupted, need to try write again
                bytesWritten = 0;
            }
            else
            {
                // something bad happens then print error message
                printErrMsg ("write");
                return -1;
            }
        }
        bytesLeft = bytesLeft - bytesWritten;
        tempPtr = tempPtr + bytesWritten;
    }

    return (size);
}


// Description : Reads line from server
// Parameters  : fd - fild descriptor
//               read_ptr - to check whether data has only \n or not
//               max_length - size to read
// Return      : On success number of bytes read and on failure -1
int readline(int fd, struct SBCPMsg *read_ptr, int max_length){
    
    int byte_read = 0;    // to check whether read() function operates well or not

    if (NULL != read_ptr)
    {
        while (0 > (byte_read = read(fd, read_ptr, (max_length))))
       {
            // something bad happens
            if ((-1 == byte_read) && (EINTR == errno))
            {
                // something is bad because of interrupt, then try again!
                continue;
            }
            // read() function do not operate nomally
            printErrMsg ("read");

            return -1;
        }
    }

    return byte_read;
}

int main (int argc, char **argv){

    int	sockfd = 0;                                // socket description
    int byte_read = 0;                             // number of bytes to read
    int select_result = 0;
    char recvline[MAXLINE +1] = {0};               // to write and read from terminal to server
    char username[MAX_USERNAME_LEN] = {0};         // store username
    struct sockaddr_in	servaddr;     
    struct SBCPMsg sbcpSendMsg;                    // to send msg to server
    struct SBCPMsg sbcpRecvMsg;                    // to receive msg from server
    bool timeout = false;                          // to build IDLE mode
    time_t startTime = 0;
    time_t endTime = 0;
    int difference = 0;                            // how many seconds for IDLE mode
    bool isTimerSet = false;
    char tempUsername [17] = {0};

    fd_set readset;
    struct timeval tv;

    if(argc != 4)
    {    
        // anyone write wrong format then inform he/she to know
        printf("Error : It is not correct format!\n");
        printf("Format : ./client  <USERNAME> <IP adress> <Port number>\n");
        return 1; // 1 means fault	
    }

    // when username is more than 15 character (if 15 character then username would be 16 char because of \0), receive username again   
    snprintf (tempUsername, sizeof (tempUsername), "%s", argv[1]); 
    while(15<strlen(tempUsername))
    {
        printf("username should not be more than 15 characters\n");
        printf("new username : ");
        fgets(tempUsername, sizeof (tempUsername), stdin);
    }

    // store username
    snprintf (username, sizeof (username), "%s", tempUsername);

    // create socket
    if((sockfd = socket(AF_INET, SOCK_STREAM,0)) < 0)
    {
        // fail to make socket
        printErrMsg ("socket");
        return 1;
    }

    if (MAXPORTNUMBER < atoi (argv[3]))
    {
        printf ("Error : Port number provided is beyond the max value 65535 \n"
                "        Please provide a port number which is within the range of 1025-65535 \n");
        return 1;
    }
    else if (0 >= atoi (argv[3]))
    {
        printf ("Error : Port number provided is less than 1, which is invalid\n"
                "        Please provide a port number which is within the range of 1025-65535 \n");
        return 1;
    }

    memset(&servaddr,'0',sizeof(servaddr));	

    // to prepare to connect server.
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(atoi(argv[3]));

    // IP_address from command line
    if(inet_pton(AF_INET, argv[2], &servaddr.sin_addr) <= 0)
    {
        // inet_pton() function is not working well then show what is the problem.
	printf ("Error : The IPv4 address provided is invalid \n");

        return 1;		
    }

    // try to connect to server
    if(connect(sockfd,(const struct sockaddr *) & servaddr, sizeof(servaddr)) < 0)
    {
        // connect() function is not working well, then show what is the problem
        printErrMsg ("connect");
        return 1;
    }

    // initialize
    memset(&sbcpSendMsg,0,sizeof(sbcpSendMsg));
    memset(&sbcpRecvMsg,0,sizeof(sbcpRecvMsg));

    // At first, send to JOIN message to the server
    sbcpSendMsg.vrsn = SBCP_VRN;
    sbcpSendMsg.type = eJOIN;
    sbcpSendMsg.len = sizeof(sbcpSendMsg);
    sbcpSendMsg.attr_0.type = eUSERNAME;
    sbcpSendMsg.attr_0.len = sizeof(sbcpSendMsg.attr_0);
    snprintf(sbcpSendMsg.attr_0.payload.username, sizeof(sbcpSendMsg.attr_0.payload.username), "%s", username);

    // access to server for entering the chat
    if(writen(sockfd, &sbcpSendMsg, sizeof(sbcpSendMsg)) < 0)
    {
        printErrMsg("writen");
    }

    // read from server
    readline(sockfd, &sbcpRecvMsg, sizeof(sbcpRecvMsg));

    if(eNAK == sbcpRecvMsg.type)
    {
        // NAK signal is coming
        if(eREASON == sbcpRecvMsg.attr_0.type)
        {
            printf("I am sorry but you cannot connect to the server because %s\n",sbcpRecvMsg.attr_0.payload.reason);
            return 1;
        }
    }
    else if (eACK == sbcpRecvMsg.type)
    {
        // ACK signal is coming
        if((eCLIENTCOUNT == sbcpRecvMsg.attr_0.type) && (eMESSAGE == sbcpRecvMsg.attr_1.type))
        {
            printf("Congratulations! You conneted to the server!\n");
            printf("\t\t\t\t\t\t\t\t\t\t\t\t\tClinet Count - %d\n",sbcpRecvMsg.attr_0.payload.clientCount);
            printf("\t\t\t\t\t\t\t\t\t\t\t\t\tConnected Clients - %s\n", sbcpRecvMsg.attr_1.payload.msg);
        }
    }
    else 
    {
        // unavailable signal is coming
        printf("I am sorry but you cannot connect to the server. Try it again\n");
        return 1;
    }

    while(1)
    {
        // wait for 10 seconds (1 * 10)
        tv.tv_sec = 1;
        tv.tv_usec = 0;

        if (false == isTimerSet)
        {
            isTimerSet = true;
            startTime = time(NULL);
        }

        FD_ZERO(&readset);          // Initialize
        FD_SET(sockfd,&readset);    // connect server and client
        FD_SET(0,&readset);         // connect terminal and client
        
        do 
        {
            select_result = select( sockfd+1, &readset, NULL, NULL, &tv);
            // the client has to check that there is a file to read(readset) on max file(sockfd). and there is timeout(tv)
        } while(EINTR == errno);
    
        memset (&sbcpRecvMsg, 0, sizeof (sbcpRecvMsg));
        if(select_result > 0)
        {
            if(FD_ISSET(sockfd, &readset))
            {
                // read available - read from server
                byte_read = readline(sockfd, &sbcpRecvMsg, MAXLINE);
                recvline[byte_read] = 0;//null terminate
                // check that input from server is well bring to client

                if(eONLINE == sbcpRecvMsg.type)
                {
                    // ONLINE MODE - one more client is coming to the chat
                    if((eUSERNAME == sbcpRecvMsg.attr_0.type) && (eCLIENTCOUNT == sbcpRecvMsg.attr_1.type))
                    {
                        // check there is proper type or not
                        printf("\t\t\t\t\t\t\t\t\t\t\t\t\t%s - Online\n",sbcpRecvMsg.attr_0.payload.username);
                        printf("\t\t\t\t\t\t\t\t\t\t\t\t\tClient count- %d\n", sbcpRecvMsg.attr_1.payload.clientCount);
                    }
                    else 
                    {
                        // type is something wrong
                        continue;
                    }
                }
                else if (eOFFLINE == sbcpRecvMsg.type)
                {
                    // OFFLINE MODE - one client is exit from the chat
                    if((eUSERNAME == sbcpRecvMsg.attr_0.type)&&(eCLIENTCOUNT == sbcpRecvMsg.attr_1.type))
                    {
                        printf("\n\t\t\t\t\t\t\t\t\t\t\t\t\t%s - Offline\n",sbcpRecvMsg.attr_0.payload.username);
                        printf("\t\t\t\t\t\t\t\t\t\t\t\t\tClient count- %d\n", sbcpRecvMsg.attr_1.payload.clientCount);
                    }
                    else 
                    {
                        // type is something wrong
                        continue;
                    }
                }
                else if (eIDLE == sbcpRecvMsg.type)
                {
                    if(eUSERNAME == sbcpRecvMsg.attr_0.type)
                    {
                        printf ("\t\t\t\t\t\t\t\t\t\t\t\t\t%s - Idle \n", sbcpRecvMsg.attr_0.payload.username);
                    }
                    else
                    {
                        // type is something wrong
                        continue;
                    }
                }
                else if (eFORWARD == sbcpRecvMsg.type)
                {
                    // FWD MODE - normal receiving mode
                    if((eUSERNAME == sbcpRecvMsg.attr_0.type) && (eMESSAGE == sbcpRecvMsg.attr_1.type))
                    {       
                        printf ("\t\t\t\t\t\t\t%s: %s", sbcpRecvMsg.attr_0.payload.username, sbcpRecvMsg.attr_1.payload.msg);
                    }
                    else 
                    {
                        // type is something wrong
                        continue;
                    }
                }
                else 
                {
                    // Message is corrupted, so drop the packet
                    continue;
                }
                  
                byte_read = 0;
            }
            else if(FD_ISSET(0,&readset))
            {
                isTimerSet = false;
                memset (&sbcpSendMsg, 0, sizeof (sbcpSendMsg));
                timeout = false;
                memset(recvline, 0, sizeof(recvline));

                // get data from terminal
                if (NULL == fgets(recvline, sizeof(recvline), stdin))
                {
                    // encountered EOF(NULL)!
                    printf ("Encountered EOF, so closing the client\n");
                    break;
                }
    
                if ( '\n' == recvline [0] )
                {
                    // To avoid \n only stuct case, this 'if' is exist
                    continue;
                }
        
                // send to the server! (making some header)
                sbcpSendMsg.type = eSEND; //SEND
                sbcpSendMsg.len = sizeof(sbcpSendMsg);
                sbcpSendMsg.attr_0.type = eUSERNAME;
                sbcpSendMsg.attr_0.len = sizeof(sbcpSendMsg.attr_0);
                snprintf(sbcpSendMsg.attr_0.payload.username,sizeof(sbcpSendMsg.attr_0.payload.username),"%s",username);
                sbcpSendMsg.attr_1.type = eMESSAGE;
                sbcpSendMsg.attr_1.len = sizeof(sbcpSendMsg.attr_1);
                snprintf(sbcpSendMsg.attr_1.payload.msg,sizeof(sbcpSendMsg.attr_1.payload.msg),"%s",recvline);
                  
                // check that input from terminal is well bring to code.
                // write brought data to server.			
                writen(sockfd,&sbcpSendMsg,sizeof (sbcpSendMsg));
            }
        }

        else if (select_result < 0) 
        {
            // error appear
            printErrMsg ("select");
        }
        endTime = time(NULL);
        difference = endTime - startTime;
        if ((timeout != true) && ( difference > 10) && (isTimerSet == true))
        {
            memset (&sbcpSendMsg, 0, sizeof (sbcpSendMsg));
            timeout = true;
            sbcpSendMsg.type = eIDLE; // idle  
            sbcpSendMsg.len = sizeof(sbcpSendMsg);
            sbcpSendMsg.attr_0.type = eUSERNAME;
            sbcpSendMsg.attr_0.len = sizeof(sbcpSendMsg.attr_0);
            snprintf(sbcpSendMsg.attr_0.payload.username,sizeof(username),"%s",username);

            // send to server that this client is in idle state
            writen(sockfd, &sbcpSendMsg, sizeof(sbcpSendMsg));             
        }
    }

    // Since action of writing and reading to server is finish, close socket
    if (close(sockfd) < 0)
    {
        // Something bad happens to close socket!
        printErrMsg ("close");
        return 1;
    }

    printf("Goodbye~"); // Goodbye client! See you next!!

    return 0; // Finish!!
}
