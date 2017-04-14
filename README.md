# Rubiee

Rubiee is a programming language based on LLVM. Its syntax is very similar to Ruby, but surely Rubiee is not Ruby.

Rubiee has no `Class`, it's just a simple language with only the support of `function`, `variable` and `control flow construct`. 

## Current Progress

1. Simple arithmetic operators
2. Comparison operators 
3. Variable
4. Function call 
5. if construct
6. for loop

## How to build ?

This project is based on LLVM, Flex and Bison. 

Requirements:

1. LLVM version : 5.0.0svn
2. Flex version : 2.5.35
3. Bison version : 3.0.4

If your version is different, you could have problem when doing compilation.

To build :

`make`