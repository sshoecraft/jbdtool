CFLAGS=-Wall -g -DDEBUG=1 -I..
SRCS=jbd_info.c jbd.c parson.c list.c utils.c cfg.c daemon.c module.c ip.c serial.c bt.c can.c
DEPS=$(addprefix ../,$(SRCS))

TESTS=test_parse_bits

all: $(TESTS)

test: $(TESTS)
	@for t in $(TESTS); do ./$$t || exit 1; done

test_parse_bits: test_parse_bits.c $(DEPS) ../main.c
	$(CC) $(CFLAGS) -o $@ test_parse_bits.c $(DEPS) -lpthread

clean:
	rm -rf $(TESTS) *.dSYM
