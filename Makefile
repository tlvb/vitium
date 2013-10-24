CC=gcc
CFLAGS=--std=gnu99 -Wextra -pedantic-errors -g

vpath %.c src/
vpath %.h src/

vitium: main.c logger.o ycbcr.o ppmio.o ibuffer.o counter.o work.o
	$(CC) $(CFLAGS) -o $@ $(filter-out %.h, $^) -lpthread -lm

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $<

.PHONY: purge
purge: clean
	-rm vitium
.PHONY: clean
clean:
	-rm *.o
