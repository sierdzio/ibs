# Intro

IBS (in-source build system) is a build tool that makes it easier to build C++
projects by using the information from source code to compile it.

In short, to compile a project it is enough to run:

    ibs main.cpp

Instead of "the old way":

    qmake project.pro
    make

# Usage

A typical usage of ibs would be to call it on your main source file (of your
app or libary):

    ibs main.cpp

To clean the project, run:

    ibs --clean main.cpp

Specifying the source file is not necessary - it will be extracted from ibs
cache file generated during compilation.

## Command line flags

To see all available commands, type:

    ibs --help

Most notable options are:
* `-c "target name ibs; include someDir"` allows you to pass ibs commands via
the command line. All available ibs commands are listed below
* `-j 5` allows you to lower the maximum number of threads used by ibs. By
default, ibs uses all CPU cores. Note: you can also specify a fraction as the
argument to `j`. For example, `-j 0.5` will use half of all available CPU cores
* `--clean` removes all build artefacts. Executable and ibs cache are left alone
* `--auto-include` Automatically scans source directory for include paths. This
can be used instead of ibs command 'include some/path' if the path is below
input file
* `--auto-qt-modules` Automatically guesses Qt modules used by the project. This
is done using internal dictionary mapping Qt classes to modules

# Commands

## Syntax

To get additional functionality and specify external libraries, sources etc.
you can use ibs commands inside comments of your C++ code.

You can use one-line command syntax:

    //i some command

Or comment scope:

    /*i
    some command
    other command
    */

## Available commands

### Specific source files

You can specify an extra source file to compile like this:

    //i source my_source_file.cpp

This is especially useful for cases where ibs cannot guess the source file for
given header.

### Target

Target command can be used to define 3 things:

* executable / library name

    //i target name MyAwesomeApp

* target type: either a library or an application

    //i target type app
    //i target type lib

* libraries can additionally be marked as static or dynamic. Dynamic libraries
are the default value, it can be skipped

    //i target type lib static
    //i target type lib dynamic

### Defines

To pass custom defines to the compiler, use the following command:

    //i define MY_DEFINE

### Include paths

Include paths are specified using `include` command:

    //i include some/path

### Libraries

To include and link to an external library, you need to specify both the include
and libs commands.

    //i include some/path
    //i lib -Lsome/path -llibrary1 -llibrary2

### Qt support

Ibs is written in Qt, and it makes it easier to build other Qt projects, too.
In order to compile a Qt application or library, you need to specify Qt
directory by runnig ibs with `--qt-dir` flag:

    ibs --qt-dir /home/qt/5.9.4/gcc_64 main.cpp

This will tell ibs which Qt version should it use. Then, in the source code of
your application or library, you need to specify Qt modules which should be
loaded.

    //i qt core network

To run Qt tools, use the `tools` command:

    //i tool rcc myResource.qrc myOtherResource.qrc

You do not need to run MOC manually, ibs will run it automatically when needed.

### Tools

IBS can run external tools, applications and processes. To do this, use the `tool`
command, followed by executable path and any necessary arguments.

There are some tools which are pre-configured (like Qt's tools: `rcc` and `uic`)
and you don't need to specify their paths.

    //i tool myexecutable.exe --some -a -r -g -s

# Recommendations

It's best to put ibs commands early in .cpp or .h file, so that they can be
parsed before the rest of includes. Otherwise it may happen that you define
include folder after it was needed.
