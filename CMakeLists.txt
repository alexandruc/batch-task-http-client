cmake_minimum_required(VERSION 2.8)

set(SRC_FILES
	./httpclient.cpp
	./requestbuilder.cpp
)
set(HEADER_FILES
	./httpclient.h
	./requestbuilder.h
)
add_library(http STATIC ${SRC_FILES} ${HEADER_FILES})
