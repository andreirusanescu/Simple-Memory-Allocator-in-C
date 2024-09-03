# compiler setup
CC=gcc
CFLAGS=-Wall -Wextra -std=c99

# define targets
TARGETS = sfl

build: $(TARGETS)

sfl: sfl.o func.o
	$(CC) $^ -o $@

sfl.o: sfl.c
	$(CC) $(CFLAGS) $^ -c

$(func).o: $(func).c $(func).h
	$(CC) $(CFLAGS) $^ -c

# pack:
# 	zip -FSr 313CC_RusanescuAndreiMarian_tema1.zip Makefile *.c *.in
run_sfl:
	./sfl

clean:
	rm -f $(TARGETS) *.o
.PHONY: pack clean
