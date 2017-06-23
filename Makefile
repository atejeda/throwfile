# valgrind --tool=memcheck --leak-check=yes ./throwfile -r /cloud ./tmp
# -g as a cxxflag to have the debugging symbols

SOURCES    = main.cpp
LIBRARIES  = $(shell curl-config --libs)
CXXFLAGS   = $(shell curl-config --cflags) -Wno-comment
CXXFLAGS  += -Wno-deprecated-declarations -std=c++11 -g
CXX        = g++

all:
	${CXX} ${SOURCES} -o throwfile ${CXXFLAGS} ${LIBRARIES}

clean:
	rm -rf throwfile