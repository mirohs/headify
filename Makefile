CFLAGS = -std=c99 -Wall -Wno-unused-function -Wno-unused-variable -Werror -Wpointer-arith -Wfatal-errors
DEBUG = -g

# disable default suffixes
.SUFFIXES:

SOURCES = headify.c util.c
DEPENDENCIES = $(SOURCES:.c=.d)
OBJECTS = $(SOURCES:.c=.o)

# pattern rule for compiling .c-file to executable
%: %.o util.o
	gcc $(CFLAGS) $(DEBUG) $< util.o -lm -o $@
	
headify: $(OBJECTS)
	gcc $(CFLAGS) $(DEBUG) $(OBJECTS) -lm -o $@

%.c %.h: %.d.c
	./headify $< > $@

%.o: %.c
	gcc -c $(CFLAGS) $(DEBUG) $<

%.d: %.c
	@echo "$@ \\" >$@; \
	gcc -MM $(CFLAGS) $(DEBUG) $< >>$@

include $(DEPENDENCIES)

# do not treat "clean" as a file name
.PHONY: clean 

# remove produced files, invoke as "make clean"
clean: 
	rm -f *.o
	rm -f *.d
	rm -rf .DS_Store
	rm -rf *.dSYM
