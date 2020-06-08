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
	$(WGET) -nc $(URL)/$(FILE) && tar --no-same-owner -zxf $(FILE)
	cd $(DIR) && mkdir -p cmake/build && cd cmake/build && cmake -DgRPC_INSTALL=ON -DCMAKE_BUILD_TYPE=Release -DgRPC_BUILD_GRPC_CSHARP_PLUGIN=OFF -DgRPC_BUILD_GRPC_NODE_PLUGIN=OFF -DgRPC_BUILD_GRPC_OBJECTIVE_C_PLUGIN=OFF -DgRPC_BUILD_GRPC_RUBY_PLUGIN=OFF -DCMAKE_INSTALL_PREFIX=$(DEPS_PATH) ../.. && $(MAKE) && $(MAKE) install
	rm -rf $(FILE) $(DIR)
