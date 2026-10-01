CXX      := g++
CXXFLAGS := -std=c++17 -O2 -g -Wall
INCLUDES := -IDependencies/include
LIBS     := -lglfw -lGL -lX11 -lpthread -ldl -lm

GLAD_OBJ := src/glad.o
BINS     := boundary_fill scanline_fill jordan_fill

all: $(BINS)

boundary_fill: boundary_fill.o $(GLAD_OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LIBS)

scanline_fill: scanline_fill.o $(GLAD_OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LIBS)

jordan_fill: jordan_fill.o $(GLAD_OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LIBS)

boundary_fill.o: boundary_fill.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

scanline_fill.o: scanline_fill.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

jordan_fill.o: jordan_fill.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

%.o: %.c
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f *.o $(GLAD_OBJ) $(BINS)

.PHONY: all clean
