CFLAGS = -std=c99 -Wall -Wno-unused-function -Wno-unused-variable -Werror -Wpointer-arith -Wfatal-errors
DEBUG = -g

# https://www.gnu.org/software/make/manual/html_node/File-Name-Functions.html
DCFILES = $(wildcard *.d.c)
# https://www.gnu.org/software/make/manual/html_node/Text-Functions.html#Text-Functions
EXEFILES = $(DCFILES:.d.c=)

CFILES = $(wildcard *.c)
EXEFILES2 = $(CFILES:.c=)

# disable default suffixes
.SUFFIXES:

module_b: module_b.o module_a.o
	gcc $(CFLAGS) $(DEBUG) $^ -lm -o $@

# pattern rule for compiling .c-file to executable
%: %.o ../util.o
	gcc $(CFLAGS) $(DEBUG) $^ -lm -o $@
	
%.h.c: %.d.c
	../../embrace/embrace $< > $@

%.c %.h: %.h.c
	../headify $<

%.o: %.c
	gcc -c $(CFLAGS) $(DEBUG) -iquote.. $<

# do not treat "clean" as a file name
.PHONY: clean 

# remove produced files, invoke as "make clean"
clean: 
	rm -f *.o
	rm -f *.d
	rm -rf .DS_Store
	rm -rf *.dSYM
	rm -f $(EXEFILES)
	rm -f $(EXEFILES2)
	rm -f *_headify.[ch]
