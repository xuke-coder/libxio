CC = gcc
AR = ar
CP = cp
DEBUG = -g
CFLAG =-Wall -Werror
LIB = -lpthread
SO_TARGET = lib/libxio.so
ODIR = obj
SDIR = src
INC = -Iinc
SYSINC = /usr/local/include
SYSLIB = /usr/local/lib


_OBJS = xio.o		\
	xio_data.o		\
	xio_memory.o	\
	xio_signal.o	\
	xio_spinlock.o	\
	xio_thread.o	\
	xio_worker.o

OBJS = $(patsubst %,$(ODIR)/%,$(_OBJS))

$(ODIR)/%.o: $(SDIR)/%.c
	$(CC) -fPIC -c $(INC) -o $@ $< $(CFLAGS) $(DEBUG)

.PHONY:all clean

all: so

so: $(OBJS)
	$(CC) -o $(SO_TARGET) $(OBJS) $(LIB) -shared 

install:
	$(CP) $(SO_TARGET) $(SYSLIB)
	$(CP) inc/xio.h $(SYSINC)
	
clean:
	rm -rf  $(ODIR)/*.o  $(SO_TARGET)
