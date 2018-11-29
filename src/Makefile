#
# cloredis Makefile 
# Copyright (c) 2018 James Wei (weijianlhp@163.com). All rights reserved.
#
# NOTE: 
# * hiredis has been packed into cloredis, so you don't need to install hiredis independently
# * You can change installation directory by run 'make install PREFIX=$(YOUR_DIR)'
#
# Compile commands:
#
# make 				        -- normal compilation, no tutorial nor unit test
# make DEFINE=-DUSE_DEBUG   -- enable cloredis internal debug
# make enable_utest=true    -- enable cloredis unit test
# make enable_tutorial=true -- enable cloredis example 
# make install PREFIX=xxx   -- install cloredis in directory xxx 
#
# by default, cloredis will be installed in '/usr/local/cloredis/' directory
#
PREFIX?=/usr/local/cloredis
INCLUDE_PATH?=include/cloredis
LIB_PATH?=lib

GCC=cc
CXX=g++

TEST_BIN=cloredis_test
TUTORIAL_BIN=tutorial
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
LDFLAGS+=-L../lib
LDYNAMICS+=-lpthread
TEST_SPEC_LD=-lgtest -lpthread
INCLUDES=-I.

CFLAGS=-std=c99 -pedantic -c -O3 -fPIC  -Wall -W -Wstrict-prototypes -Wwrite-strings # -g -ggdb

ifeq ($(enable_utest), true)
	LDFLAGS+=-L/usr/local/cloriconf/lib
	LDYNAMICS+=-lcloriconf -Wl,-rpath=/usr/local/cloriconf/lib 
	INCLUDES+=-I/usr/local/cloriconf/include 
endif

CXXFLAGS_ALL=$(CXXFLAGS) $(DEFINE) $(OPTIMIZATION) $(DEBUG_FLAGS) $(WARNINGS)

INSTALL_INCLUDE_PATH=$(PREFIX)/$(INCLUDE_PATH)
INSTALL_LIB_PATH=$(PREFIX)/$(LIB_PATH)

INSTALL_CMD?= cp -pPR
DYLIB_MAJOR_NAME=$(CLOREDIS_SONAME).$(DYLIBSUFFIX).$(MAJOR_VERSION) 				 
DYLIB_MINOR_NAME=$(CLOREDIS_SONAME).$(DYLIBSUFFIX).$(MAJOR_VERSION).$(MINOR_VERSION) 
DYLIB_NAME=$(CLOREDIS_SONAME).$(DYLIBSUFFIX)  										 
STLIB_NAME=$(CLOREDIS_SONAME).$(STLIBSUFFIX)										

TEST_SRC=$(wildcard googletest/*.cc)
TUTORIAL_SRC=$(wildcard example/*.cc)

TEST_OBJECTS=$(TEST_SRC:%.cc=%.o)
TUTORIAL_OBJECTS=$(TUTORIAL_SRC:%.cc=%.o)

SOURCES=$(wildcard *.cc internal/*.cc) 
OBJECTS=$(SOURCES:%.cc=%.o)
HIREDIS_SRC=$(wildcard hiredis/*.c)
HIREDIS_OBJS=$(HIREDIS_SRC:%.c=%.o)

ALL_TARGET=$(DYLIB_MINOR_NAME) $(STLIB_NAME)
ifeq ($(enable_utest), true)
	ALL_TARGET+=$(TEST_BIN)
endif

ifeq ($(enable_tutorial), true)
	ALL_TARGET+=$(TUTORIAL_BIN)
endif

all: $(ALL_TARGET)
	@echo "mv $(TEST_BIN) and $(TUTORIAL_BIN) to bin directory..."
	@if [ -f $(TEST_BIN) ]; then mv $(TEST_BIN) ../bin/ ; fi
	@if [ -f $(TUTORIAL_BIN) ]; then mv $(TUTORIAL_BIN) ../bin/ ; fi
	@echo "All done ===="

$(TEST_BIN):$(OBJECTS) $(TEST_OBJECTS) $(HIREDIS_OBJS)
	$(CXX) $^ -o $@ $(LDFLAGS) $(LDYNAMICS) ${TEST_SPEC_LD}

$(TUTORIAL_BIN):$(OBJECTS) $(TUTORIAL_OBJECTS) $(HIREDIS_OBJS)
	$(CXX) $^ -o $@ $(LDFLAGS) $(LDYNAMICS) ${TEST_SPEC_LD}

$(DYLIB_MINOR_NAME):$(OBJECTS) $(HIREDIS_OBJS)
	$(CXX) -o $@ $^ -shared -Wl,-soname,$(DYLIB_MAJOR_NAME) $(LDFLAGS) $(LDYNAMICS) 

$(STLIB_NAME):$(OBJECTS) $(HIREDIS_OBJS)
	ar rcs $@ $^

$(HIREDIS_OBJS):%.o:%.c
	$(GCC) $(CFLAGS) -c $< -o $@
	
$(TEST_OBJECTS):%.o:%.cc
	$(CXX) $(INCLUDES) $(CXXFLAGS_ALL) -c $< -o $@

$(TUTORIAL_OBJECTS):%.o:%.cc
	$(CXX) $(INCLUDES) $(CXXFLAGS_ALL) -c $< -o $@

$(OBJECTS):%.o:%.cc
	$(CXX) $(INCLUDES) $(CXXFLAGS_ALL) -c $< -o $@

install:$(DYLIB_MINOR_NAME) $(STLIB_NAME)
	mkdir -p $(INSTALL_INCLUDE_PATH) $(INSTALL_INCLUDE_PATH)/internal $(INSTALL_LIB_PATH)
	$(INSTALL_CMD) cloredis.h $(INSTALL_INCLUDE_PATH) 
	$(INSTALL_CMD) connection.h $(INSTALL_INCLUDE_PATH) 
	$(INSTALL_CMD) reply.h $(INSTALL_INCLUDE_PATH) 
	$(INSTALL_CMD) internal/connection_pool.h $(INSTALL_INCLUDE_PATH)/internal
	$(INSTALL_CMD) internal/singleton.h $(INSTALL_INCLUDE_PATH)/internal
	$(INSTALL_CMD) $(STLIB_NAME) $(INSTALL_LIB_PATH)
	$(INSTALL_CMD) $(DYLIB_MINOR_NAME) $(INSTALL_LIB_PATH)
	cd $(INSTALL_LIB_PATH) && ln -sf $(DYLIB_MINOR_NAME) $(DYLIB_MAJOR_NAME) 
	cd $(INSTALL_LIB_PATH) && ln -sf $(DYLIB_MINOR_NAME) $(DYLIB_NAME)
	@echo "All done ===="

clean:
	-rm -f $(TEST_OBJECTS) ${TUTORIAL_OBJECTS} $(OBJECTS) $(HIREDIS_OBJS) $(STLIB_NAME) $(DYLIB_MINOR_NAME)  

.PHONY: all install clean
