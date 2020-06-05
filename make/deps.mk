# this is faster than github's release page
URL := https://raw.githubusercontent.com/crazyboycjr/deps/master/build
WGET ?= wget

PROTOBUF := $(DEPS_PATH)/include/google/protobuf/message.h
$(PROTOBUF):
	$(eval FILE=protobuf-cpp-3.6.1.tar.gz)
	$(eval DIR=protobuf-3.6.1)
	rm -rf $(FILE) $(DIR)
	$(WGET) $(URL)/$(FILE) && tar --no-same-owner -zxf $(FILE)
	cd $(DIR) && ./configure --prefix=$(DEPS_PATH) && $(MAKE) && $(MAKE) install
	rm -rf $(FILE) $(DIR)

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

