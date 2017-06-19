SOURCES    =  main.cpp
LIBRARIES  = $(shell curl-config --libs)
CXXFLAGS   = $(shell curl-config --cflags) -Wno-comment
CXXFLAGS  += -Wno-deprecated-declarations -std=c++11
CXX        = ccache g++


throwfile:
	@echo "...throwfile"
	@mkdir -p ../bin
	${CXX} ${SOURCES} -o ../bin/throwfile ${CXXFLAGS} ${LIBRARIES}

clean:
	@echo "...clean"
	@rm -rf ../{objects,bin} hexdump

all: throwfile
	@echo "...all"
