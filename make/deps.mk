# this is faster than github's release page
URL := https://github.com/crazyboycjr/deps/raw/master/build
WGET ?= wget

BOOST_COROUTINE := $(DEPS_PATH)/include/boost/coroutine/
$(BOOST_COROUTINE):
	$(eval FILE=boost-coroutine-1.70.0.tar.gz)
	$(eval DIR=boost-coroutine-1.70.0)
	rm -rf $(FILE) $(DIR)
	$(WGET) $(URL)/$(FILE) && tar --no-same-owner -zxf $(FILE) && cd $(DIR) && ./bootstrap.sh --with-libraries=coroutine && ./b2 --prefix=$(DEPS_PATH) --build-type=minimal variant=release link=static debug-symbols=on coroutine install -j 4
	rm -rf $(FILE) $(DIR)

GOOGLE_BENCHMARK := $(DEPS_PATH)/include/benchmark/benchmark.h
$(GOOGLE_BENCHMARK):
	$(eval FILE=google-benchmark-1.5.0.tar.gz)
	$(eval DIR=benchmark-1.5.0)
	rm -rf $(FILE) $(DIR)
	$(WGET) $(URL)/$(FILE) && tar --no-same-owner -zxf $(FILE)
	cd $(DIR) && mkdir -p build && cd build && cmake -DBENCHMARK_ENABLE_TESTING=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$(DEPS_PATH) ../ && $(MAKE) && $(MAKE) install
	rm -rf $(FILE) $(DIR)

GRPC := $(DEPS_PATH)/include/grpcpp/grpcpp.h
$(GRPC):
	$(eval FILE=grpc-v1.29.1.tar.gz)
	$(eval DIR=grpc-v1.29.1)
	rm -rf $(FILE) $(DIR)
	$(WGET) $(URL)/$(FILE) && tar --no-same-owner -zxf $(FILE)
	cd $(DIR) && mkdir -p cmake/build && cd cmake/build && cmake -Dprotobuf_WITH_ZLIB=ON -DgRPC_INSTALL=ON -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=ON -DgRPC_BUILD_GRPC_CSHARP_PLUGIN=OFF -DgRPC_BUILD_GRPC_NODE_PLUGIN=OFF -DgRPC_BUILD_GRPC_OBJECTIVE_C_PLUGIN=OFF -DgRPC_BUILD_GRPC_PHP_PLUGIN=OFF -DgRPC_BUILD_GRPC_RUBY_PLUGIN=OFF -DCMAKE_INSTALL_PREFIX=$(DEPS_PATH) ../.. && $(MAKE) && $(MAKE) install
	rm -rf $(FILE) $(DIR)

GFLAGS := $(DEPS_PATH)/include/gflags/gflags.h
$(GFLAGS):
	$(eval FILE=gflags-2.2.2.tar.gz)
	$(eval DIR=gflags-2.2.2)
	rm -rf $(FILE) $(DIR)
	$(WGET) $(URL)/$(FILE) && tar --no-same-owner -zxf $(FILE)
	cd $(DIR) && mkdir -p build && cd build && cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=ON -DCMAKE_INSTALL_PREFIX=$(DEPS_PATH) .. && $(MAKE) && $(MAKE) install
	rm -rf $(FILE) $(DIR)

LEVELDB := $(DEPS_PATH)/include/leveldb/db.h
$(LEVELDB):
	$(eval FILE=leveldb-1.22.tar.gz)
	$(eval DIR=leveldb-1.22)
	rm -rf $(FILE) $(DIR)
	$(WGET) $(URL)/$(FILE) && tar --no-same-owner -zxf $(FILE)
	cd $(DIR) && mkdir -p build-static && cd build-static && cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=ON LEVELDB_BUILD_TESTS=OFF LEVELDB_BUILD_BENCHMARKS=OFF -DCMAKE_INSTALL_PREFIX=$(DEPS_PATH) .. && $(MAKE) && $(MAKE) install
	cd $(DIR) && mkdir -p build-shared && cd build-shared && cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF LEVELDB_BUILD_TESTS=OFF LEVELDB_BUILD_BENCHMARKS=OFF -DCMAKE_INSTALL_PREFIX=$(DEPS_PATH) .. && $(MAKE) && $(MAKE) install
	rm -rf $(FILE) $(DIR)

BRPC := $(DEPS_PATH)/include/brpc/channel.h
$(BRPC): $(GFLAGS) $(LEVELDB) $(GRPC)
	$(eval FILE=incubator-brpc-0.9.7.tar.gz)
	$(eval DIR=incubator-brpc-0.9.7)
	rm -rf $(FILE) $(DIR)
	$(WGET) $(URL)/$(FILE) && tar --no-same-owner -zxf $(FILE)
	cd $(DIR) && sed -i "173s#\(.*\)-lssl -lcrypto\(.*\)#\1`pkg-config --variable=libdir libssl`/libssl.so `pkg-config --variable=libdir libcrypto`/libcrypto.so\2#g" config_brpc.sh && PATH=$(DEPS_PATH)/bin:$(PATH) sh config_brpc.sh --headers="$(DEPS_PATH)/include /usr/include" --libs="$(DEPS_PATH)/lib /usr/lib64" && $(MAKE) && cp -r output/* $(DEPS_PATH)
	rm -rf $(FILE) $(DIR)

ZEROMQ := $(DEPS_PATH)/include/zmq.h
$(ZEROMQ):
	$(eval FILE=zeromq-4.3.2.tar.gz)
	$(eval DIR=zeromq-4.3.2)
	rm -rf $(DIR)
	$(WGET) https://github.com/zeromq/libzmq/releases/download/v4.3.2/zeromq-4.3.2.tar.gz && tar --no-same-owner -zxf $(FILE)
	cd $(DIR) && mkdir -p build && cd build && cmake -DCMAKE_INSTALL_PREFIX=$(DEPS_PATH) .. && $(MAKE) && $(MAKE) install
	rm -rf $(FILE) $(DIR)
