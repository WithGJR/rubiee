SHELL = /bin/bash
OBJS = main.o parser.bison.o lexer.o flex_lexer.o ast.o driver.o codegen_visitor.o stdlib.o
CC = g++
LLVM_CONFIG = `llvm-config --cxxflags`

main: ${OBJS}
	${CC} `llvm-config --cxxflags --ldflags --system-libs --libs core native support orcjit executionengine` -o main ${OBJS}

parser.bison.o: parser.bison.cc
	${CC} -Wno-deprecated-register -std=c++11 -c parser.bison.cc -o parser.bison.o

parser.bison.cc: parser.yy
	bison -d -o parser.bison.cc parser.yy

flex_lexer.o: flex_lexer.cc
	${CC} -Wno-deprecated-register -std=c++11 -c flex_lexer.cc -o flex_lexer.o

flex_lexer.cc: lexer.l
	flex lexer.l

%.o: %.cpp
	${CC} ${LLVM_CONFIG} -std=c++11 -c $<

.PHONY: clean
clean:
	rm -f *.o
	rm -f *.cc
	rm -f *.hh
