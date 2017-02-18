all: client server
client: client.c
	gcc -Wall -o client client.c common.c commands.c -lpthread -lrt
server: server.c	
	gcc -Wall -o server server.c common.c commands.c user_data.c room_data.c -lpthread -lrt
.PHONY: clean
clean:
	-rm client server
