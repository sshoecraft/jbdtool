
PROG=jbdtool
SRCS=main.c module.c jbd.c ip.c bt.c parson.c list.c utils.c
OBJS=$(SRCS:.c=.o)
CFLAGS+=-DJBDTOOL
#CFLAGS+=-g -Wall -I../mybmm -I../util
CFLAGS+=-g -Wall
LIBS+=-ldl -lgattlib -lglib-2.0 -lpthread
LDFLAGS+=-rdynamic

all: $(PROG)

$(PROG): $(OBJS) $(DEPS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(PROG) $(OBJS) $(LIBS)

#$(OBJS): Makefile *.h

debug: $(PROG)
	gdb ./$(PROG)

install: $(PROG)
	sudo install -m 755 -o bin -g bin $(NAME) /usr/sbin

clean:
	rm -rf $(PROG) $(OBJS) $(CLEANFILES)

push: clean
	git add -A .
	git commit -m refresh
	git push

pull: clean
	git reset --hard
	git pull
