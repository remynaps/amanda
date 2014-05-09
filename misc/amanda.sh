#!/bin/bash

PROGRAM_DIRECTORY="`dirname "$0"`"

if [[ "$OSTYPE" == 'linux-gnu' ]]; then
    export LD_LIBRARY_PATH="$PROGRAM_DIRECTORY"
else
    export DYLD_LIBRARY_PATH="$PROGRAM_DIRECTORY"
fi

"$PROGRAM_DIRECTORY/amanda" "$@"
