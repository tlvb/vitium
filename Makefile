CC=gcc
CFLAGS=--std=gnu99 -Wextra -pedantic-errors -g

test: main.c logger.o ycbcr.o ppmio.o ibuffer.o counter.o work.o ppmsupport.o
	$(CC) $(CFLAGS) -o test main.c logger.o ycbcr.o ppmio.o ibuffer.o counter.o work.o ppmsupport.o -lpthread -lm

counter.o: counter.c counter.h
	$(CC) $(CFLAGS) -c counter.c

logger.o: logger.c logger.h
	$(CC) $(CFLAGS) -c logger.c

ycbcr.o: ycbcr.c ycbcr.h common.h
	$(CC) $(CFLAGS) -c ycbcr.c

ppmio.o: ppmio.c ppmio.h common.h
	$(CC) $(CFLAGS) -c ppmio.c

ibuffer.o: ibuffer.c ibuffer.h
	$(CC) $(CFLAGS) -c ibuffer.c

work.o: work.c work.h
	$(CC) $(CFLAGS) -c work.c

ppmsupport.o: ppmsupport.c ppmsupport.h
	$(CC) $(CFLAGS) -c ppmsupport.c


.PHONY: clean
clean:
	-rm *.o
	-rm test
