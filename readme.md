# WFC

A simple [wave function collapse/model
synthesis](https://en.wikipedia.org/wiki/Model_synthesis) implementation in c.

## How to run

Requirements:

* a c compiler (e.g. gcc, clang)
* cmake

How to compile (on linux/macos):

* `cmake -B build`
* `cmake --build build`

How to run:

`[executable] [input file] [output file]`

The executable is the one you compiled in the previous step (usually
`build/wfc`), the input and output files are ppm files. For the input file you
can try `input.ppm` as an example, the variables still have to be configured in
the code.

## Todo

* collapse cells
* output image

## Goals

* create a simple and portable wave function collapse implementation
* improve my general c skills
* learn to write clean c code
