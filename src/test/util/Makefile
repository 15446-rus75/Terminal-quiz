CXX            ?= g++
TEST_CXXFLAGS  ?= -std=c++20 -Wall -Wextra -Wpedantic -O0 -g
BOOST_FLAGS    ?= -DBOOST_TEST_DYN_LINK
BOOST_LIBS     ?= -lboost_unit_test_framework
THREAD_FLAGS   ?= -pthread
TEST_BUILD_DIR ?= $(abspath ../../../build/test)
TEST_ARGS      ?= --report_level=short --color_output=no

TARGET       := $(TEST_BUILD_DIR)/util_tests
UTIL_DIR     := ../../util
SOURCES      := $(wildcard test_*.cpp)
UTIL_SOURCES := $(UTIL_DIR)/html_decoder.cpp $(UTIL_DIR)/logger.cpp
HEADERS      := $(wildcard $(UTIL_DIR)/*.hpp)

.PHONY: all test clean

all: $(TARGET)

$(TARGET): $(SOURCES) $(UTIL_SOURCES) $(HEADERS)
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(TEST_CXXFLAGS) $(BOOST_FLAGS) $(THREAD_FLAGS) -I../.. $(SOURCES) $(UTIL_SOURCES) $(LDFLAGS) -o $@ $(BOOST_LIBS)

test: $(TARGET)
	$(TARGET) $(TEST_ARGS)

clean:
	rm -f $(TARGET)
