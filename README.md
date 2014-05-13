amanda
======

Functional programming language implemented in C, initially written by
Dick Bruin.

Platform compiling
======
<b>Linux</b>

Compiling under Linux is a no-brainer.

    sudo apt-get install build-essential libreadline-dev

Then `make` inside the `amanda` folder.

After compiling, the `amanda.sh` file will dynamically import `libamanda.so` and
run `amanda`. This is useful for testing. Installing `amanda` into
`/usr/local/bin` and installing `libamanda.so` into `/usr/local/lib` should
resolve the need for the hacky `amanda.sh`.

<b>OS X</b>

Ensure you have XCode installed, and its command-line tools. Also make
sure that `gcc-4.8` is installed (`brew install gcc48`). The standard GCC
compiler will be used, without Apple's standard LLVM back-end. Once this is set
up, a simple `make` will do.

After compiling, the `amanda.sh` file will dynamically import `libamanda.so` and
run `amanda`. This is useful for testing. Installing `amanda` into
`/usr/local/bin` and installing `libamanda.so` into `/usr/local/lib` should
resolve the need for the hacky `amanda.sh`.

<b>Windows</b>

This is a bit more complex. Install [MinGW](http://www.mingw.org/).
This will provide everything you need, but to access your tools, you
need to alter your PATH variable. Instructions:

* Right-click "My PC", and go to properties
* Go to "Advanced system settings"
* Click on "Environment variables..."
* If PATH exists as <b>user variable</b>, edit it and add `; C:\MinGW\bin`
* If it doesn't already exist, make a new one and add `C:\MinGW\bin`

`mingw32-make` in the `amanda` folder will now do your servings.

`amanda.exe` will locally import `libamanda.dll` and run just fine on its own.

<b>Cross-compile from Linux to Windows</b>

You need the MinGW compiler under Linux.

    sudo apt-get install mingw-w64 mingw32 mingw32-binutils

Then compile using `CROSSFLAG=-cross make`

original fork message (acda6a917f)
======

Completely barebones version of Amanda as provided by Dick Bruin.

This version provided by Dick Bruin is free of any rights. Exact quote:

"onder geen enkele licentie.
Het staat ze vrij te veranderen wat ze willen. Als ze het publiceren,
dan graag vermelden dat er geen rechten aan kunnen worden verbonden.
Mijn naam hoeft niet genoemd te worden."

This fork will only accept contributions licensed (or not) under an
(un)license that has yet to be decided. If a completely public domain,
unedited version of Amanda is desired, this commit is the desired commit
to fork.

Java GUI removed. Any UI implementations of Amanda should not have to
re-implement Amanda proper. Any UI implementations henceforth should be
their own project, and should have Amanda as a dependency rather than as
an extra component.

Several improvements have been applied to Amanda by students of NHL
Hogeschool. This version - insofar I am concerned - does not contain any
of those patches. These patches may be reapplied upon further
inspection, under the assumption that their patches were public domain
contributions.

This repository will (most likely) be abandoned by the end of the school
year (Summer 2014). Two teams consisting of approximately four members
each will be working on this repository for three months. One team will
work on the core. The other team will implement additional
functionality.

Future implementations of Amanda should preferably fork from the last
commit to this repository. Future IDE implementations of Amanda should
treat Amanda as a separate dependency for their program, and start a new
blank repository. In case core Amanda tweaks are necessary to implement
IDE functionality, such a team should both fork from the last commit to
this repository (where the core patches will be applied) and create a
new blank repository (where the IDE will be developed from scratch). The
same applies to any other non-core projects involving Amanda, unless
such projects aim to create new standard libraries. Sensibility is
required in that decision-making.
