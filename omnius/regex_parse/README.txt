REGEX PARSER
============
This program has been modified export a function which takes, as input, a null-terminated regular expression. and
It returns a reduced mapping between external char input symbols and an internal enumeration, as well as a jump table
that describes a FSM through transitions on input (internal enumeration).


ORIGINAL README.txt CONTENTS
============================
A C++ implementation of a regular expression recognizer. The recognizer constructs an NFA from a regular expression
(with a simplified syntax), converts it into a DFA, and simulates the DFA on the input string.

The main function is in regex_parse.cpp

To compile/build, run:
g++ -ansi -Wall -pedantic -o regex *.cpp

The code is ANSI C++ and should compile cleanly on any standard C++ compiler.


This code is in the public domain
Eli Bendersky (eliben@gmail.com)
