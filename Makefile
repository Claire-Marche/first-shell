CC=gcc
TARGETS=fish cmd_line_test
CFLAGS=-Wall -std=c99 -g -O2
LDFLAGS=-g

all: $(TARGETS)

fish: fish.o cmdline.o list.o
	$(CC) $(LDFLAGS) $^ -o $@

cmd_line_test: cmdline.o cmdline_test.o
	$(CC) $(LDFLAGS) $^ -o $@
	
cmd_line_test.o: cmdline_test.c cmdline.h
	$(CC) $(CFLAGS) -c -o $@ $<

fish.o: fish.c cmdline.h
	$(CC) $(CFLAGS) -c -o $@ $<
	
cmdline.o: cmdline.c cmdline.h
	$(CC) $(CFLAGS) -c -o $@ $<

list.o: list.c list.h
	$(CC) $(CFLAGS) -c -o $@ $<
	
clean:
	rm -f *.o
	
mrproper: clean
	rm -f $(TARGETS)
