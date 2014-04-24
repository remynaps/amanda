CFLAGS=-g -O6 -Wall -Wextra -Isrc -DNDEBUG $(OPTFLAGS)
LIBS=$(OPTLIBS)

ifeq ($(OS),Windows_NT)
	CC=gcc
	TARGET=bin/amanda.exe
	RM=del /f /q
	MKDIR=mkdir
else
	UNAME := $(shell uname)
	RM=rm -rf
	MKDIR=mkdir -p
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
	$(MKDIR) build
	$(MKDIR) bin
	#COPY STUFF
#	ifeq ($(OS),Windows_NT)
#		mkdir build
#		mkdir bin
#		copy misc\amanda.ini bin
#		copy misc\test.ama bin
#	else
#		@mkdir build
#		@mkdir bin
#		@cp misc/amanda.ini bin/amanda.ini
#		@cp misc/test.ama  bin/test.ama
#	endif

# The Cleaner
clean:
	$(RM) build $(OBJECTS) $(TESTS)
#	ifeq ($(OS),Windows_NT)
#		del /f /q build $(OBJECTS) $(TESTS)
#	else
#		rm -rf build $(OBJECTS) $(TESTS)
#		#rm -f tests/tests.log
#		find . -name "*.gc*" -exec rm {} \;
#		rm -rf `find . -name "*.dSYM" -print`
#	endif
