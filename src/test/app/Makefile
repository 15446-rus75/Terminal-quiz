MODULE := app

CXX            ?= g++
TEST_CXXFLAGS  ?= -std=c++23 -Wall -Wextra -Wpedantic -O0 -g
BOOST_FLAGS    ?= -DBOOST_TEST_DYN_LINK
BOOST_LIBS     ?= -lboost_unit_test_framework
LIBS           ?= -lpqxx -lpq -lcurl
TEST_BUILD_DIR ?= $(abspath ../../../build/test)

ifdef BOOST_ROOT
BOOST_INCLUDES := -I$(BOOST_ROOT)/include
BOOST_LDFLAGS  := -L$(BOOST_ROOT)/lib -Wl,-rpath,$(BOOST_ROOT)/lib
endif

SRC_ROOT  := ../..
LAYER_DIR := $(SRC_ROOT)/app
BUILD_DIR := $(TEST_BUILD_DIR)/$(MODULE)
BIN       := $(BUILD_DIR)/run_tests

DEPENDENCY_SOURCES := \
  $(patsubst $(SRC_ROOT)/%,%,$(filter-out $(SRC_ROOT)/main.cpp,$(wildcard $(SRC_ROOT)/tui/*.cpp))) \
  $(patsubst $(SRC_ROOT)/%,%,$(wildcard $(SRC_ROOT)/services/*.cpp)) \
  $(patsubst $(SRC_ROOT)/%,%,$(wildcard $(SRC_ROOT)/db/*.cpp)) \
  $(patsubst $(SRC_ROOT)/%,%,$(wildcard $(SRC_ROOT)/api/*.cpp)) \
  $(patsubst $(SRC_ROOT)/%,%,$(wildcard $(SRC_ROOT)/core/*.cpp)) \
  $(patsubst $(SRC_ROOT)/%,%,$(wildcard $(SRC_ROOT)/util/*.cpp))

TEST_SOURCES       := $(wildcard *.cpp)
LAYER_SOURCES      := $(wildcard $(LAYER_DIR)/*.cpp)
TEST_OBJECTS       := $(TEST_SOURCES:%.cpp=$(BUILD_DIR)/tests/%.o)
LAYER_OBJECTS      := $(patsubst $(LAYER_DIR)/%.cpp,$(BUILD_DIR)/layer/%.o,$(LAYER_SOURCES))
DEPENDENCY_OBJECTS := $(DEPENDENCY_SOURCES:%.cpp=$(BUILD_DIR)/dependencies/%.o)

.DEFAULT_GOAL := test
.PHONY: test build clean

build: $(BIN)

test: $(BIN)
	$(BIN)

$(BIN): $(TEST_OBJECTS) $(LAYER_OBJECTS) $(DEPENDENCY_OBJECTS)
	@mkdir -p $(@D)
	$(CXX) $^ -o $@ $(BOOST_LDFLAGS) $(LIBS) $(BOOST_LIBS)

$(BUILD_DIR)/tests/%.o: %.cpp
	@mkdir -p $(@D)
	$(CXX) $(TEST_CXXFLAGS) -MMD -MP $(BOOST_FLAGS) $(BOOST_INCLUDES) -I$(SRC_ROOT) -c $< -o $@

$(BUILD_DIR)/layer/%.o: $(LAYER_DIR)/%.cpp
	@mkdir -p $(@D)
	$(CXX) $(TEST_CXXFLAGS) -MMD -MP -I$(SRC_ROOT) -c $< -o $@

$(BUILD_DIR)/dependencies/%.o: $(SRC_ROOT)/%.cpp
	@mkdir -p $(@D)
	$(CXX) $(TEST_CXXFLAGS) -MMD -MP -I$(SRC_ROOT) -c $< -o $@

-include $(TEST_OBJECTS:.o=.d) $(LAYER_OBJECTS:.o=.d) $(DEPENDENCY_OBJECTS:.o=.d)

clean:
	rm -rf $(BUILD_DIR)
