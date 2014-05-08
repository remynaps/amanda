CFLAGS=-g -O6 -Wall -Wextra -Isrc -fPIC -DNDEBUG $(OPTFLAGS)
LIBS=$(OPTLIBS)

ifeq ($(OS),Windows_NT)
	CC=gcc
	EXE=bin\amanda.exe
	AMALIB=bin\amanda.dll
	RM=del /f /q
	MKDIR=mkdir
	SOURCES=$(subst /,\,$(wildcard src/*.c)))
else
	UNAME:=$(shell uname)
	RM=rm -rf
	MKDIR=mkdir -p
	SOURCES=$(wildcard src/*.c)
	ifeq ($(CROSSFLAG),-cross)
		CC=x86_64-w64-mingw32-gcc
		EXE=bin/amanda.exe
		AMALIB=bin/amanda.dll
	else
		EXE=bin/amanda
		AMALIB=bin/amanda.so
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

OBJECTS=$(addsuffix .o,$(basename $(SOURCES)))

# The Target Build
all: copyfiles $(EXE)

LIB: $(OBJECTS)
	$(CC) -shared $(CFLAGS) $(OBJECTS) -o $(AMALIB)

$(EXE): LIB
	$(CC) $(CFLAGS) src/amcon.o $(AMALIB) $(LIBS) -o $(EXE)

build:
	$(MKDIR) build
	$(MKDIR) bin

# The Cleaner
ifeq ($(OS),Windows_NT)
copyfiles: build
	copy misc\amanda.ini bin
	copy misc\test.ama bin
else
copyfiles: build
	cp misc/amanda.ini bin/amanda.ini
	cp misc/test.ama bin/test.ama
endif

clean:
	$(RM) build $(OBJECTS) $(TESTS)
