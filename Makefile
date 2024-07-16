# compiler setup
CC=gcc
CFLAGS=-Wall -Wextra -std=c99

# define targets
TARGETS = sfl

build: $(TARGETS)

sfl: sfl.c
	$(CC) $(CFLAGS) -o $@ $<


# pack:
# 	zip -FSr 313CC_RusanescuAndreiMarian_tema1.zip Makefile *.c *.in
run_sfl:
	./sfl

clean:
	rm -f $(TARGETS)
.PHONY: pack clean
