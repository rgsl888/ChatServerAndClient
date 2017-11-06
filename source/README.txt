/**************************************************************************/
/**                                                                      **/
/**    Chat Server and Client                                            **/
/**    Assignment - 2 for ECEN-602 Computer Communication and Networks   **/
/**    Developers : Client - Han Bee Oh                                  **/
/**                 UIN : 826008965                                      **/
/**                 Server - Ramakrishna Prabhu                          **/
/**                 UIN : 725006454                                      **/
/**                                                                      **/
/**************************************************************************/

Both the developers reviewed each other's code and improved the quality of
it. We also helped each other by providing the different test cases to make
the code as robust as possible.

General Info:
-------------
To compile and generate binary just run make command in the source directory as shown below

	make
	
This will generate two binaries

	client     - chat client
	server     - chat server
	
---------------******** Client - Start *******----------------

Description :
-------------

The client is simple and it has been designed to communicate
with server by using sbcp message format.

Client is using select to toggle between the communication from 
other clients and the terminal input from the user

Other than that, client can deal with JOIN/FORWARD/SEND/NAK/
OFFLINE/ACK/ONLINE/IDLE(10 seconds hold) case.

Simple flow of the program has been depicted below,

              Client
	     ________
	   
	      socket
	        |
	     connect
	        |  JOIN
                |-----------------------------
             Y  |                            \/
      close<---NAK?<---ACK/NAK----------->(SERVER)--
                |                             ^    |
	       EOF?<---------------           |    |
	        |                  |          |    |
	  true  | false            |          |    |
             ___|___               | --------      |
            |       |              |               |
          close     |              |               |
                    |    read/send |               |
        read----> select<--------->|               |
	            ^                              |
		    |______________________________|
				
				
Usage : 
-------

Syntax : 
		./client USERNAME IPV4_ADDR PORT_NUMBER 
	  
        USERNAME can be less than 16 characters

        PORT_NUMBER can be from the range of 1025-65536
		
		Example : ./client HAN 192.168.1.17 8080

        After executing the binary user can send to messages to the server


---------------******** Client - End *******------------------


---------------******** Server - Start *******------------------

Description :
-------------
		
This server handle the chat sessions of the groups of clients.
Server can define maximum number of clients and only those set of clients
can only get access to it. 

Server accepts the connection and checks all the restrcitions such as same 
username and maximum clients. Then server forwards the message coming from 
a client to all other clients. Apart from this, server should send status 
messages like ONLINE, OFFLINE, IDLE, Number of clients connected and their list.

Server is using SBCP message format to communicate and it deals with 
ACK, NAK, JOIN, FORWARD, IDLE, OFFLINE, ONLINE and SEND.

Simple flow of the programs has been depicted below,

                    Server
	           ________
		    socket
		      |
		     bind
		      |
		    listen
		      |
------------------- select<-------------------------- (Clients)
|		      |                                   ^
|         (Check which socket is set)                     |
|       	      |                                   |
|       listening     |    Old Socket                     |
|         socket      |                                   |
|           ----------------------                        |
|	   |                       |                      |
|        accept      (Check for message type and respond) |
|	   |                 appropriately                |
|  Add the socket to               |                      |
|      structure                   |                      |
|	   |                       |                      |
|	   -------------------------    All msgs          |
|                     |-----------------------------------
----------------------

Usage :
-------

Syntax : Please run the server "server" as shown below

      ./server PORT_NUMBER MAX_CLIENTS
	
      PORT_NUMBER can be from the range of 1025-65536
	  MAX_CLIENTS is the number of clients supported by server

Example : ./server 8080 10

After this point server will listen for clients and responds as per the message type

-------------******** Server - End *******------------------