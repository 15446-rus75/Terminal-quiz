MODULE := db
CXX            ?= g++
TEST_CXXFLAGS  ?= -std=c++20 -Wall -Wextra -Wpedantic -O0 -g
BOOST_FLAGS    ?= -DBOOST_TEST_DYN_LINK
BOOST_LIBS     ?= -lboost_unit_test_framework
TEST_BUILD_DIR ?= $(abspath ../../../build/test)
PG_CONFIG      ?= pg_config
TEST_ARGS      ?= --report_level=short --color_output=no

ifdef BOOST_ROOT
BOOST_INCLUDES := -I$(BOOST_ROOT)/include
BOOST_LDFLAGS  := -L$(BOOST_ROOT)/lib -Wl,-rpath,$(BOOST_ROOT)/lib
endif
PQ_CFLAGS := $(shell $(PG_CONFIG) --includedir 2>/dev/null | sed 's|^|-I|')
PQ_LDFLAGS := $(shell $(PG_CONFIG) --libdir 2>/dev/null | sed 's|^|-L|')

SRC_ROOT := ../..
BUILD_DIR := $(TEST_BUILD_DIR)/$(MODULE)
BIN := $(BUILD_DIR)/run_tests
TEST_SOURCES := $(wildcard *.cpp)
DB_SOURCES := $(wildcard $(SRC_ROOT)/db/*.cpp)
TEST_OBJECTS := $(patsubst %.cpp,$(BUILD_DIR)/tests/%.o,$(TEST_SOURCES))
DB_OBJECTS := $(patsubst $(SRC_ROOT)/db/%.cpp,$(BUILD_DIR)/layer/%.o,$(DB_SOURCES))
CORE_OBJECT := $(BUILD_DIR)/dependencies/enums.o
OBJECTS := $(TEST_OBJECTS) $(DB_OBJECTS) $(CORE_OBJECT)

.DEFAULT_GOAL := test
.PHONY: test unit build clean check-sources

check-sources:
	@test -f $(SRC_ROOT)/db/schema_initializer.cpp || { echo 'db implementation is missing: synchronize the test branch with master first'; exit 1; }

build: check-sources $(BIN)

test: build
	@PG_CONFIG="$(PG_CONFIG)" sh ./run_tests.sh "$(BIN)" $(TEST_ARGS)

unit: build
	"$(BIN)" --run_test=support $(TEST_ARGS)

$(BIN): $(OBJECTS)
	@mkdir -p $(@D)
	$(CXX) $(TEST_CXXFLAGS) -pthread $(LDFLAGS) $^ -o $@ $(BOOST_LDFLAGS) $(PQ_LDFLAGS) $(BOOST_LIBS) -lpq

$(BUILD_DIR)/tests/%.o: %.cpp
	@mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(TEST_CXXFLAGS) -pthread -MMD -MP $(BOOST_FLAGS) $(BOOST_INCLUDES) -I$(SRC_ROOT) -c $< -o $@

$(BUILD_DIR)/layer/%.o: $(SRC_ROOT)/db/%.cpp
	@mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(TEST_CXXFLAGS) -pthread -MMD -MP $(PQ_CFLAGS) -I$(SRC_ROOT) -c $< -o $@

$(CORE_OBJECT): $(SRC_ROOT)/core/enums.cpp
	@mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(TEST_CXXFLAGS) -pthread -MMD -MP -I$(SRC_ROOT) -c $< -o $@

-include $(OBJECTS:.o=.d)

clean:
	rm -rf "$(BUILD_DIR)"
