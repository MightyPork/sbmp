CFLAGS += -g -Wall -Wextra -I.

OBJECTS = \
	main.o \
	sbmp/crc32.o \
	sbmp/sbmp_checksum.o \
	sbmp/sbmp_frame.o \
	sbmp/sbmp_datagram.o \
	sbmp/sbmp_session.o

main: $(OBJECTS)

run: main
	@./main

clean:
	rm -f *.o *.lst main
	rm -f sbmp/*.o
