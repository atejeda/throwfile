# Dependencies,
# Ubuntu: apt-get install -y libssl-dev

SOURCES    =  mongoose.c restclient.cpp connector.cpp tokenmgr.cpp throwfile.cpp
LIBRARIES  = -lssl -lcrypto
INCLUDE    = -I../include
CXXFLAGS   = -pthread -DMG_ENABLE_SSL -Wno-comment -Wno-deprecated-declarations -std=c++0x
CXX        = g++
CC         = gcc

# mongoose:
# 	@echo "...mongoose"
# 	@mkdir -p ../objects
# 	@${CC} -Wall -W -Os -g -c ${INCLUDE} -pthread -DMG_ENABLE_SSL mongoose.c -o ../objects/mongoose

throwfile:
	@echo "...throwfile"
	@mkdir -p ../bin
	${CXX} ${SOURCES} -o ../bin/throwfile ${INCLUDE} ${CXXFLAGS} ${LIBRARIES}

clean:
	@echo "...clean"
	@rm -rf ../{objects,bin} hexdump

all: throwfile
	@echo "...all"
