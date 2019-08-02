# LBeacon
#---------------------------------------------------------------------------
CC = gcc -std=gnu99
OBJS = BeDIS.o Tag.o
LIB = -L /usr/local/lib

#---------------------------------------------------------------------------
all: Tag
Tag: $(OBJS)
	$(CC) $(OBJS) $(CFLAGS) -o Tag $(LIB) -lrt -lpthread -lbfb -lbluetooth -lwiringPi -lzlog 
	@mv Tag ../bin/
	chown pi:pi ../bin/Tag
Tag.o: Tag.c Tag.h
	$(CC) Tag.c Tag.h $(LIB) -c
BeDIS.o: BeDIS.c BeDIS.h
	$(CC) $(CFLAGS) BeDIS.c -c

clean:
	find . -type f | xargs touch
	@rm -rf *.o *.h.gch *.log *.log.0 *.txt Tag
