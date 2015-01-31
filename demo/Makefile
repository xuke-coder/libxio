CC = gcc
AR = ar
DEBUG = -g
CFLAG =-Wall -Werror
LIB = -L ./ -lxio
TARGET = demo

SRC = demo.c

.PHONY:all clean

all: 
	$(CC) -o $(TARGET) $(SRC) $(CFLAG) $(DEBUG) $(LIB)
    
clean:
	rm -rf *.o $(TARGET)
