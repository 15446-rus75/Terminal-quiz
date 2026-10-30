MODULE := services

CXX            ?= g++
TEST_CXXFLAGS  ?= -std=c++20 -Wall -Wextra -Wpedantic -O0 -g
BOOST_FLAGS    ?= -DBOOST_TEST_DYN_LINK
BOOST_LIBS     ?= -lboost_unit_test_framework
THREAD_FLAGS   ?= -pthread
TEST_BUILD_DIR ?= $(abspath ../../../build/test)
TEST_ARGS      ?= --report_level=short --color_output=no

TARGET       := $(TEST_BUILD_DIR)/services_tests
SRC_ROOT     := ../..
SERVICE_DIR  := $(SRC_ROOT)/services
CORE_DIR     := $(SRC_ROOT)/core
UTIL_DIR     := $(SRC_ROOT)/util

SOURCES      := $(wildcard *.cpp)
SERVICE_SOURCES := $(wildcard $(SERVICE_DIR)/*.cpp)
CORE_SOURCES    := $(wildcard $(CORE_DIR)/*.cpp)
UTIL_SOURCES    := $(wildcard $(UTIL_DIR)/*.cpp)

.PHONY: all test clean

all: $(TARGET)

$(TARGET): $(SOURCES) $(SERVICE_SOURCES) $(CORE_SOURCES) $(UTIL_SOURCES)
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(TEST_CXXFLAGS) $(BOOST_FLAGS) $(THREAD_FLAGS) -I$(SRC_ROOT) $^ -o $@ $(BOOST_LIBS)

test: $(TARGET)
	$(TARGET) $(TEST_ARGS)

clean:
	rm -f $(TARGET)

