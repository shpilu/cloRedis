#
# cloredis Makefile 
# Copyright (c) 2018 James Wei (weijianlhp@163.com). All rights reserved.
#
# NOTE: 
# * hiredis has been packed into cloredis, so you don't need to install hiredis independently
# * Cloredis will install in /usr/local/ directory by default
# * You can change installation directory by run 'make install PREFIX=$(YOUR_DIR)'
# * For cloredis internal test, use 'make DEFINE=-DUSE_DEBUG'
#
PREFIX?=/usr/local
INCLUDE_PATH?=include/cloredis
LIB_PATH?=lib

GCC=cc
CXX=g++

TEST_BIN=cloredis_test
CLOREDIS_SONAME=$(shell grep CLOREDIS_SONAME cloredis.h | awk '{print $$3}')
MAJOR_VERSION=$(shell grep CLOREDIS_MAJOR cloredis.h | awk '{print $$3}')
MINOR_VERSION=$(shell grep CLOREDIS_MINOR cloredis.h | awk '{print $$3}')

DYLIBSUFFIX=so
STLIBSUFFIX=a

# Currently macro 'USE_DEBUG' used for internal test only

# gcc parameters 
OPTIMIZATION?=-O2
DEFINE?= #-DUSE_DEBUG 
DEBUG_FLAGS?= # -g -ggdb 
WARNINGS=-Wall -W -Wwrite-strings
CXXFLAGS=-std=c++11 -fPIC -fno-omit-frame-pointer
LDFLAGS+=-L../lib # xxx 
LDYNAMICS+=-lpthread -lcconf # -lhiredis
TEST_SPEC_LD=-lgtest -lpthread
INCLUDES=-I. -I./include # -I/usr/local/include/hiredis

CFLAGS=-std=c99 -pedantic -c -O3 -fPIC  -Wall -W -Wstrict-prototypes -Wwrite-strings # -g -ggdb
CXXFLAGS_ALL=$(CXXFLAGS) $(DEFINE) $(OPTIMIZATION) $(DEBUG_FLAGS) $(WARNINGS)

INSTALL_INCLUDE_PATH=$(PREFIX)/$(INCLUDE_PATH)
INSTALL_LIB_PATH=$(PREFIX)/$(LIB_PATH)

INSTALL_CMD?= cp -pPR
DYLIB_MAJOR_NAME=$(CLOREDIS_SONAME).$(DYLIBSUFFIX).$(MAJOR_VERSION) 				 
DYLIB_MINOR_NAME=$(CLOREDIS_SONAME).$(DYLIBSUFFIX).$(MAJOR_VERSION).$(MINOR_VERSION) 
DYLIB_NAME=$(CLOREDIS_SONAME).$(DYLIBSUFFIX)  										 
STLIB_NAME=$(CLOREDIS_SONAME).$(STLIBSUFFIX)										

TEST_SRC=$(wildcard test/*.cc)
TEST_OBJECTS=$(TEST_SRC:%.cc=%.o)
SOURCES=$(wildcard *.cc public/*.cc) 
OBJECTS=$(SOURCES:%.cc=%.o)
HIREDIS_SRC=$(wildcard hiredis/*.c)
HIREDIS_OBJS=$(HIREDIS_SRC:%.c=%.o)

all: $(DYLIB_MINOR_NAME) $(STLIB_NAME) $(TEST_BIN)
	mv $(TEST_BIN) ../bin/  
	@echo "All done ===="

$(TEST_BIN):$(OBJECTS) $(TEST_OBJECTS) $(HIREDIS_OBJS)
	$(CXX) $^ -o $@ $(LDFLAGS) $(LDYNAMICS) ${TEST_SPEC_LD}

$(DYLIB_MINOR_NAME):$(OBJECTS) $(HIREDIS_OBJS)
	$(CXX) -o $@ $^ -shared -Wl,-soname,$(DYLIB_MAJOR_NAME) $(LDFLAGS) $(LDYNAMICS) 

$(STLIB_NAME):$(OBJECTS) $(HIREDIS_OBJS)
	ar rcs $@ $^

$(HIREDIS_OBJS):%.o:%.c
	$(GCC) $(CFLAGS) -c $< -o $@
	
$(TEST_OBJECTS):%.o:%.cc
	$(CXX) $(INCLUDES) $(CXXFLAGS_ALL) -c $< -o $@

$(OBJECTS):%.o:%.cc
	$(CXX) $(INCLUDES) $(CXXFLAGS_ALL) -c $< -o $@

install:$(DYLIB_MINOR_NAME) $(STLIB_NAME)
	mkdir -p $(INSTALL_INCLUDE_PATH) $(INSTALL_LIB_PATH)
	$(INSTALL_CMD) cloredis.h $(INSTALL_INCLUDE_PATH) 
	$(INSTALL_CMD) $(STLIB_NAME) $(INSTALL_LIB_PATH)
	$(INSTALL_CMD) $(DYLIB_MINOR_NAME) $(INSTALL_LIB_PATH)
	cd $(INSTALL_LIB_PATH) && ln -sf $(DYLIB_MINOR_NAME) $(DYLIB_MAJOR_NAME) 
	cd $(INSTALL_LIB_PATH) && ln -sf $(DYLIB_MINOR_NAME) $(DYLIB_NAME)
	@echo "All done ===="

clean:
	-rm -f $(TEST_OBJECTS) $(OBJECTS) $(HIREDIS_OBJS) $(STLIB_NAME) $(DYLIB_MINOR_NAME)  

.PHONY: all install clean
