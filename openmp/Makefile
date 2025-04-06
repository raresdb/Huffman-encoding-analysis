CXX=g++
CXXFLAGS=-g -fopenmp

SOURCE_HUFFMANN=$(wildcard huffman/*.cpp)
OBJ_HUFFMANN=$(SOURCE_HUFFMANN:.cpp=.o)

SOURCE_HUFF=huff.cpp
OBJ_HUFF=$(SOURCE_HUFF:.cpp=.o)

SOURCE_DEHUFF=dehuff.cpp
OBJ_DEHUFF=$(SOURCE_DEHUFF:.cpp=.o)

all: huff dehuff

huff: $(OBJ_HUFFMANN) $(OBJ_HUFF)
	$(CXX) -o $@ $^ $(CXXFLAGS)

dehuff: $(OBJ_HUFFMANN) $(OBJ_DEHUFF)
	$(CXX) -o $@ $^ $(CXXFLAGS)

clean:
	$(RM) $(OBJ_HUFFMANN) $(OBJ_HUFF) $(OBJ_DEHUFF) huff dehuff output*.txt