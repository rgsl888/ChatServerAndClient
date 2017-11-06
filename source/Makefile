default: clearall server client

clearall:
	rm -rf server client

server: chatServer.c
	gcc chatServer.c -W -o server

client: chatClient.c
	gcc chatClient.c -W -o client
	
