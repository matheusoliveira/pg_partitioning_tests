include Makefile.global

all: partition_insert_trigger.so

partition_insert_trigger.so: partition_insert_trigger.o
	${LD} ${LD_FLAGS} -o partition_insert_trigger.so partition_insert_trigger.o

partition_insert_trigger.o: partition_insert_trigger.c
	${CC} ${CC_FLAGS} -c partition_insert_trigger.c

install: partition_insert_trigger.so
	${INSTALL} partition_insert_trigger.so ${INSTALL_DIR}

clean: build-clean
	rm -vf *.so

build-clean:
	rm -vf *.o

