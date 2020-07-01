.PHONY: clean lint compiledb

CXX ?= g++
CC ?= gcc

DEPS_PATH ?= $(shell pwd)/deps

PROTOC ?= ${DEPS_PATH}/bin/protoc
GRPC_CPP_PLUGIN ?= ${DEPS_PATH}/bin/grpc_cpp_plugin
GRPC_CPP_PLUGIN_PATH := `which $(GRPC_CPP_PLUGIN)`

INCPATH += -I./src -I$(DEPS_PATH)/include
CFLAGS += -std=c++17 -msse2 -ggdb -Wall -finline-functions $(INCPATH) $(ADD_CFLAGS)

PROTOBUF_LIBS := `pkg-config --with-path=${DEPS_PATH}/lib/pkgconfig/ --libs protobuf grpc++`
GRPC_LIBS := `pkg-config --with-path=${DEPS_PATH}/lib/pkgconfig/ --libs protobuf grpc++`
BRPC_LIBS := -lbrpc -lgflags -lleveldb ${PROTOBUF_LIBS}
ZEROMQ_LIBS := -lzmq

LIBS += -L$(DEPS_PATH)/lib -Wl,-rpath=$(DEPS_PATH)/lib -Wl,--enable-new-dtags \
		$(GRPC_LIBS) \
		$(BRPC_LIBS) \
		$(ZEROMQ_LIBS) \
		-lpthread \
		-Wl,--no-as-needed -lgrpc++_reflection -Wl,--as-needed \
		-ldl

DEBUG := 1
ifeq ($(DEBUG), 1)
CFLAGS += -ftrapv -fstack-protector-strong -DPRISM_LOG_STACK_TRACE -DPRISM_LOG_STACK_TRACE_SIZE=128
else
CFLAGS += -O3 -DNDEBUG
endif

ifdef ASAN
CFLAGS += -fsanitize=address -fno-omit-frame-pointer -fno-optimize-sibling-calls
endif

PROTO_PATH = src/protos
#PROTOS = $(shell find $(PROTO_PATH) -type f -name "*.proto")
PROTOS = $(addprefix $(PROTO_PATH)/,bw_app.proto)
GEN_SRCS = $(PROTOS:.proto=.pb.cc) $(PROTOS:.proto=.grpc.pb.cc) $(addprefix $(PROTO_PATH)/,bw_app_brpc.pb.cc)
GEN_HEADERS = $(PROTOS:.proto=.pb.h) $(PROTOS:.proto=.grpc.pb.h) $(addprefix $(PROTO_PATH)/,bw_app_brpc.pb.h)

SRCS = $(shell find src -type f -name "*.cc") $(GEN_SRCS)
OBJS = $(patsubst src/%,build/%,$(SRCS:.cc=.o))

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

$(PROTO_PATH)/%.pb.cc $(PROTO_PATH)/%.pb.h: $(PROTO_PATH)/%.proto $(GRPC)
	LD_LIBRARY_PATH=$(DEPS_PATH)/lib $(PROTOC) -I $(PROTO_PATH) --cpp_out=$(PROTO_PATH) $<

$(PROTO_PATH)/%.grpc.pb.cc $(PROTO_PATH)/%.grpc.pb.h: $(PROTO_PATH)/%.proto $(GRPC)
	LD_LIBRARY_PATH=$(DEPS_PATH)/lib $(PROTOC) -I $(PROTO_PATH) --grpc_out=$(PROTO_PATH) --plugin=protoc-gen-grpc=$(GRPC_CPP_PLUGIN_PATH) $<
	#sed -e 's/option cc_generic_services = true;/option cc_generic_services = false;/' $< > $<.tmp
	#LD_LIBRARY_PATH=$(DEPS_PATH)/lib $(PROTOC) -I $(PROTO_PATH) --grpc_out=$(PROTO_PATH) --plugin=protoc-gen-grpc=$(GRPC_CPP_PLUGIN_PATH) $<.tmp
	#mv $<.tmp.grpc.pb.cc %.grpc.pb.cc
	#mv $<.tmp.grpc.pb.h %.grpc.pb.h
	#rm -fv $<.tmp

build/%.o: src/%.cc $(GEN_HEADERS) $(GRPC) $(BRPC)
	@mkdir -p $(@D)
	$(CXX) $(INCPATH) $(CFLAGS) -MM -MT build/$*.o $< >build/$*.d
	$(CXX) $(INCPATH) $(CFLAGS) -c $< -o $@

compiledb:
	@[[ -f compile_commands.json ]] && mv -f compile_commands.json /tmp || true
	bear make -C .

-include build/*.d
-include build/*/*.d
