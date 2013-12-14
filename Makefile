
SERVER = server
SERVER_C = $(SERVER).c
SERVER_H = head.h

CLIENT = client
CLIENT_C = $(CLIENT).c
CLIENT_H = head.h

CC = gcc
CFLAGS = -g -Wall

ALL:$(SERVER) $(CLIENT)

$(SERVER):$(SERVER_C)  $(SERVER_H)
	$(CC) $(FLAGS) -o $(SERVER) $(SERVER_C) -lpthread
	
$(CLIENT):$(CLIENT_C) $(CLIENT_H)
	$(CC) $(FLASG) -o $(CLIENT) $(CLIENT_C) -lpthread

clean:
	rm  $(SERVER) $(CLIENT)
	
