LIB_TARGET := ../libshCompiler.a
INCLUDE_DIR := ../include
OPTIONS := -Wall -I$(INCLUDE_DIR) -O2
RM := rm -f

.PHONY: all
all: $(LIB_TARGET)
$(LIB_TARGET): dfa.o nfa.o nfa2dfa.o lexer.o parser.o llparser.o lrparser.o tokens.o grammar.o
	-$(RM) -f $@
	ar cq $@ $^
%.o: %.cpp
	g++ -o $@ -c $< $(OPTIONS)
dfa.o: dfa.cpp $(INCLUDE_DIR)/dfa.h $(INCLUDE_DIR)/nfa.h
nfa.o: nfa.cpp $(INCLUDE_DIR)/nfa.h
nfa2dfa.o: nfa2dfa.cpp $(INCLUDE_DIR)/nfa2dfa.h $(INCLUDE_DIR)/dfa.h $(INCLUDE_DIR)/nfa.h
lexer.o: lexer.cpp $(INCLUDE_DIR)/lexer.h $(INCLUDE_DIR)/parser.h $(INCLUDE_DIR)/dfa.h $(INCLUDE_DIR)/nfa.h $(INCLUDE_DIR)/nfa2dfa.h tokens.h nonstd.h
parser.o: parser.cpp $(INCLUDE_DIR)/parser.h $(INCLUDE_DIR)/lexer.h $(INCLUDE_DIR)/dfa.h $(INCLUDE_DIR)/nfa.h $(INCLUDE_DIR)/klist.h
llparser.o: llparser.cpp $(INCLUDE_DIR)/parser.h $(INCLUDE_DIR)/lexer.h $(INCLUDE_DIR)/dfa.h $(INCLUDE_DIR)/nfa.h $(INCLUDE_DIR)/klist.h grammar.h nonstd.h
lrparser.o: lrparser.cpp $(INCLUDE_DIR)/parser.h $(INCLUDE_DIR)/lexer.h $(INCLUDE_DIR)/dfa.h $(INCLUDE_DIR)/nfa.h $(INCLUDE_DIR)/klist.h grammar.h nonstd.h
tokens.o: tokens.cpp $(INCLUDE_DIR)/parser.h $(INCLUDE_DIR)/lexer.h $(INCLUDE_DIR)/dfa.h $(INCLUDE_DIR)/nfa.h $(INCLUDE_DIR)/klist.h tokens.h nonstd.h
grammar.o: grammar.cpp $(INCLUDE_DIR)/parser.h $(INCLUDE_DIR)/lexer.h $(INCLUDE_DIR)/dfa.h $(INCLUDE_DIR)/nfa.h $(INCLUDE_DIR)/klist.h grammar.h nonstd.h
.PHONY: clean
clean:
	-$(RM) *.o
