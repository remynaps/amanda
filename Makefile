CFLAGS=-g -O6 -Wall -Wextra -Isrc -DNDEBUG $(OPTFLAGS)
LIBS=$(OPTLIBS)

ifeq ($(OS),Windows_NT)
	CC=gcc
	TARGET=bin/amanda.exe
else
	UNAME := $(shell uname)
	ifeq ($(CROSSFLAG),-cross)
		CC=x86_64-w64-mingw32-gcc
		TARGET=bin/amanda.exe
	else
		TARGET=bin/amanda
		CFLAGS+=-DAMA_READLINE
		LIBS+=-ldl -lm -lreadline

		ifeq ($(UNAME),Linux)
			CC=gcc
		endif

		ifeq ($(UNAME),Darwin)
			CC=gcc-4.9
		endif
	endif
endif

PREFIX?=/usr/local

SOURCES=$(wildcard src/**/*.c src/*.c)
OBJECTS=$(patsubst %.c,%.o,$(SOURCES))

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
