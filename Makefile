CC=gcc
CFLAGS=-std=gnu17 -g3 -O0

all: ckpt readckpt restart counting-test

#========================

# https://stackoverflow.com/questions/36692315/what-exactly-does-rdynamic-do-and-when-exactly-is-it-needed
counting-test: counting-test.c 
	${CC} ${CFLAGS} -rdynamic -o $@ $<

ckpt: ckpt0 libckpt.so
	cp $< $@
	rm ckpt0

ckpt0: ckpt.c
	${CC} ${CFLAGS} -o $@ $<

libckpt.so: libckpt.o
	${CC} ${CFLAGS} -shared -fPIC -o $@ $<  # -fPIC required for .so files

libckpt.o: libckpt.c
	${CC} ${CFLAGS} -fPIC -c $<  # -fPIC required for .so files

readckpt: readckpt.c
	${CC} ${CFLAGS} -o $@ $<

restart: restart.c
	gcc -static \
	       -Wl,-Ttext-segment=5000000 -Wl,-Tdata=5100000 -Wl,-Tbss=5200000 \
	       -g3 -o $@ $<

check: build
	rm -f myckpt.dat
	./ckpt ./counting-test 17 &
	sleep 4
	kill -12 `pgrep --newest counting-test`
	pkill -9 counting-test
	./restart

build:
	make all

clean:
	rm -f a.out counting-test
	rm -f libckpt.so libckpt.o ckpt restart readckpt myckpt.dat
