CXX=clang++ -std=c++17
CFLAGS= -g -O3 `llvm-config --cppflags --ldflags --system-libs --libs all` \
-Wno-unused-function -Wno-unknown-warning-option -fno-rtti



# mccomp: mccomp.cpp
# 	$(CXX) mccomp.cpp $(CFLAGS) -o mccomp

# SOURCES = mccomp.cpp lexer.cpp

# FROM github
OBJECTS = $(SOURCES:.cpp=.o)
EXES = $(OBJECTS:.o=)

# all : $(OBJECTS) $(EXES)
# 	$(CXX) $(CFLAGS) -o mccomp $(OBJECTS)
# .cpp.o: 
# 	$(CXX) -o $(CFLAGS) $@ $<

# %: %.o
# 	$(CXX) -o $(CFLAGS) $<

# clean:
# 	rm -f $(OBJECTS) $(EXES) *~


# mccomp: mccomp.cpp
# 	$(CXX) mccomp.cpp $(CFLAGS) -o mccomp

# clean:
# 	rm -rf mccomp 

SOURCES = mccomp.cpp # lexer.cpp parser.cpp ast_string.cpp ast_codegen.cpp common.cpp

mccomp: $(SOURCES)
	$(CXX) $(SOURCES) $(CFLAGS) -o mccomp

clean:
	rm -rf mccomp 
