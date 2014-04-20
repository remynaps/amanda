CC = gcc

EXE_NAME = amanda

SOURCES=$(wildcard src/**/*.c src/*.c)
OBJECTS=$(patsubst %.c,%.o,$(SOURCES))

.c.o:
	$(CC) -c -DAMA_READLINE -O6 $*.c

amanda: $(OBJECTS)
	@mkdir -p bin
	$(CC) -O6 $(OBJECTS) -lm -lreadline -o bin/$(EXE_NAME)

clean:
	rm *.o
