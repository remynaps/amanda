CFLAGS=-g -O6 -Wall -Wextra -Isrc -DNDEBUG $(OPTFLAGS)
LIBS=$(OPTLIBS)

ifeq ($(OS),Windows_NT)
	CC=gcc
	TARGET=bin\amanda.exe
	RM=del /f /q
	MKDIR=mkdir
	SOURCES=$(wildcard src\*.c)
	OBJECTS=src\amcheck.o src\amcon.o src\amerror.o src\ameval.o src\amio.o src\amlex.o src\amlib.o src\ammem.o src\ammodify.o src\amparse.o src\ampatter.o src\amprint.o src\amstack.o src\amsyslib.o src\amtable.o
else
	UNAME := $(shell uname)
	RM=rm -rf
	MKDIR=mkdir -p
	SOURCES=$(wildcard src/*.c)
	OBJECTS=$(patsubst %.c,%.o,$(SOURCES))
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

SO_TARGET=$(patsubst %.a,%.so,$(TARGET))

# The Target Build
all: $(TARGET) copyfiles

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
#	ifeq ($(OS),Windows_NT)
#		del /f /q build $(OBJECTS) $(TESTS)
#	else
#		rm -rf build $(OBJECTS) $(TESTS)
#		#rm -f tests/tests.log
#		find . -name "*.gc*" -exec rm {} \;
#		rm -rf `find . -name "*.dSYM" -print`
#	endif
