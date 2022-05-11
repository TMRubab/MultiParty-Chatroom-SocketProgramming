TARGET=client server
CC=gcc
CFLAGS= -lpthread
normal: $(TARGET)
client: client.c
	$(CC) client.c $(CFLAGS) -o client
server: server.c
	$(CC) server.c -o server
clean:
	$(RM) $(TARGET)
