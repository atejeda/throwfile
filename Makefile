SOURCES    =  main.cpp
LIBRARIES  = $(shell curl-config --libs)
CXXFLAGS   = $(shell curl-config --cflags) -Wno-comment
CXXFLAGS  += -Wno-deprecated-declarations -std=c++11
CXX        = g++

all:
	${CXX} ${SOURCES} -o throwfile ${CXXFLAGS} ${LIBRARIES}

clean:
	rm -rf throwfile
