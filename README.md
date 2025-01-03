# Compiler for (a subset) of C
Using the LLVM compiler framework to lex, parse and generate intermediate representation (IR) code to compile C programs.

The language is based on C99 but it does not include arrays, structs, unions, files, pointers, sets, switch statements, do statements, for loops, or many of the low level operators. This still means that functions, types, variables and most of the operators for booleans, floats and integers are accepted.

The compiler itself uses a top-down recursive parser, operating on a transformed LL(2) grammar. 
