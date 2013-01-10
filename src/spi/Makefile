include ../Makefile.global

all: partition_insert_trigger_spi.so

partition_insert_trigger_spi.so: partition_insert_trigger_spi.o
	${LD} ${LD_FLAGS} -o partition_insert_trigger_spi.so partition_insert_trigger_spi.o

partition_insert_trigger_spi.o: partition_insert_trigger_spi.c
	${CC} ${CC_FLAGS} -c partition_insert_trigger_spi.c

install: partition_insert_trigger_spi.so
	${INSTALL} partition_insert_trigger_spi.so ${INSTALL_DIR}

clean: build-clean
	rm -vf *.so

build-clean:
	rm -vf *.o

