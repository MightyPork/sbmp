CFLAGS += -g

OBJECTS = \
    main.o \
    sbmp/crc32.o \
    sbmp/sbmp_frame.o \
    sbmp/sbmp_datagram.o

main: $(OBJECTS)

run: main
	@./main

clean:
	rm -f *.o *.lst main
	cd sbmp
	rm -f *.o *.lst main
