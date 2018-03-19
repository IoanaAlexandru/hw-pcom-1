all: build 

build: ksender kreceiver

ksender: ksender.o kutil.o link_emulator/lib.o
	gcc -g ksender.o link_emulator/lib.o kutil.o -o ksender

kreceiver: kreceiver.o kutil.o link_emulator/lib.o
	gcc -g kreceiver.o link_emulator/lib.o kutil.o -o kreceiver

.c.o: 
	gcc -Wall -g -c $? 

clean:
	-rm -f *.o ksender kreceiver recv_*
