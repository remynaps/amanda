#!/bin/sh

UNAME_STR=`uname`
PROGRAM_DIRECTORY="`dirname "$0"`"
if [[ $UNAME_STR == 'Linux' ]]; then
    export LD_LIBRARY_PATH="$PROGRAM_DIRECTORY"
else
    export DYLD_LIBRARY_PATH="$PROGRAM_DIRECTORY"
fi

"$PROGRAM_DIRECTORY/amanda" "$@"
