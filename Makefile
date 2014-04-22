UNAME := $(shell uname)
ifeq ($(UNAME), Linux)
	CC=gcc
endif

ifeq ($(UNAME), Darwin)
	CC=gcc-4.9
endif

CFLAGS=-g -O6 -Wall -Wextra -Isrc -DNDEBUG -DAMA_READLINE $(OPTFLAGS)
LIBS=-ldl -lm -lreadline $(OPTLIBS)
PREFIX?=/usr/local

SOURCES=$(wildcard src/**/*.c src/*.c)
OBJECTS=$(patsubst %.c,%.o,$(SOURCES))

TARGET=bin/amanda
SO_TARGET=$(patsubst %.a,%.so,$(TARGET))

# The Target Build
all: $(TARGET)

dev: CFLAGS=-g -Wall -Isrc -Wall -Wextra $(OPTFLAGS)
dev: all

$(TARGET): CFLAGS += -fPIC
$(TARGET): build $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) $(LIBS) -o $(TARGET)

build:
	@mkdir -p build
	@mkdir -p bin
	@cp misc/amanda.ini bin/amanda.ini
	@cp misc/test.ama  bin/test.ama

# The Cleaner
clean:
	rm -rf build $(OBJECTS) $(TESTS)
	#rm -f tests/tests.log
	find . -name "*.gc*" -exec rm {} \;
	rm -rf `find . -name "*.dSYM" -print`
