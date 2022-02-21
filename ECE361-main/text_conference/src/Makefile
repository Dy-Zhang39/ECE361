CC=gcc -lm
all: server deliver
server: server.o user.o message.o deliverHelper.o
deliver: deliver.o user.o message.o deliverHelper.o
clean:
		rm -f *.o  server deliver
