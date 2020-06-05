.PHONY: clean lint compiledb

CXX ?= g++
CC ?= gcc

DEPS_PATH ?= $(shell pwd)/deps

ifndef PROTOC
PROTOC = ${DEPS_PATH}/bin/protoc
endif

INCPATH = -I./src -I$(DEPS_PATH)/include
CFLAGS = -std=c++17 -msse2 -ggdb -Wall -finline-functions $(INCPATH) $(ADD_CFLAGS)
LIBS = -lpthread

DEBUG := 1
ifeq ($(DEBUG), 1)
CFLAGS += -ftrapv -fstack-protector-strong -DPRISM_LOG_STACK_TRACE -DPRISM_LOG_STACK_TRACE_SIZE=128
else
CFLAGS += -O3 -DNDEBUG
endif

ifdef ASAN
CFLAGS += -fsanitize=address -fno-omit-frame-pointer -fno-optimize-sibling-calls
endif

SRCS = $(shell find src -type f -name "*.cc")
OBJS = $(addprefix build/, rpc_bench.o app.o)

all: rpc-bench

include make/deps.mk

clean:
	rm -rfv $(OBJS)
	rm -rfv $(OBJS:.o=.d)

lint:
	@echo 'todo lint'

rpc-bench: build/rpc-bench

build/rpc-bench: $(OBJS)
	$(CXX) $(CFLAGS) $(LIBS) $^ -o $@

build/%.o: src/%.cc
	@mkdir -p $(@D)
	$(CXX) $(INCPATH) $(CFLAGS) -MM -MT build/$*.o $< >build/$*.d
	$(CXX) $(INCPATH) $(CFLAGS) -c $< -o $@

compiledb:
	mv compile_commands.json /tmp
	bear make -C .

-include build/*.d
-include build/*/*.d
