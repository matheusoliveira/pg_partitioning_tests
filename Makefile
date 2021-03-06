all: src/partition_insert_trigger.so src/spi/partition_insert_trigger_spi.so

src/partition_insert_trigger.so: src/partition_insert_trigger.c
	$(MAKE) -C src/

src/spi/partition_insert_trigger_spi.so: src/spi/partition_insert_trigger_spi.c
	$(MAKE) -C src/spi/

install:
	$(MAKE) -C src/ install
	$(MAKE) -C src/spi/ install

clean:
	$(MAKE) -C src/ clean
	$(MAKE) -C src/spi/ clean

build-clean:
	$(MAKE) -C src/ buld-clean
	$(MAKE) -C src/spi/ buld-clean

check:
	$(MAKE) -C test/regress/

