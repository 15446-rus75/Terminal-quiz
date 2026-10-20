CXX           := g++
CXXFLAGS_BASE := -std=c++20 -Wall -Wextra -Wpedantic
CXXFLAGS      := $(CXXFLAGS_BASE) -O2 -g -MMD -MP
INCLUDES      := -Isrc
LIBS          := -lpqxx -lpq -lcurl

SRC_DIR   := src
TEST_DIR  := $(SRC_DIR)/test
BUILD_DIR := build
BIN       := $(BUILD_DIR)/terminal_quiz

ALL_FILES    := $(shell find $(SRC_DIR) -type f 2>/dev/null)
FORMAT_FILES := $(filter %.cpp %.hpp %.h,$(ALL_FILES))
APP_FILES    := $(filter-out $(TEST_DIR)/%,$(ALL_FILES))
SOURCES      := $(filter %.cpp,$(APP_FILES))
HEADERS      := $(filter %.hpp,$(APP_FILES))
OBJECTS      := $(SOURCES:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)

BOOST_FLAGS    := -DBOOST_TEST_DYN_LINK
BOOST_LIBS     := -lboost_unit_test_framework
TEST_CXXFLAGS  := $(CXXFLAGS_BASE) -O0 -g
TEST_BUILD_DIR := $(CURDIR)/$(BUILD_DIR)/test
TEST_MODULES   := $(patsubst $(TEST_DIR)/%/Makefile,%,$(wildcard $(TEST_DIR)/*/Makefile))

export CXX TEST_CXXFLAGS LIBS BOOST_FLAGS BOOST_LIBS TEST_BUILD_DIR

.DEFAULT_GOAL := build
.PHONY: build run clean test check-format check-headers check-tidy check-cppcheck check-all fix-format

build: $(BIN)

$(BIN): $(OBJECTS)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LIBS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

-include $(OBJECTS:.o=.d)

run: build
	./$(BIN)

clean:
	rm -rf $(BUILD_DIR)

test:
ifeq ($(TEST_MODULES),)
	@echo "Тестовых модулей не найдено (ожидаются $(TEST_DIR)/<модуль>/Makefile)"
else
	@failed=""; \
	for module in $(TEST_MODULES); do \
	  echo "==> $(TEST_DIR)/$$module"; \
	  $(MAKE) --no-print-directory -C $(TEST_DIR)/$$module test || failed="$$failed $$module"; \
	done; \
	if [ -n "$$failed" ]; then echo "Тесты упали в модулях:$$failed"; exit 1; fi; \
	echo "Все тесты пройдены"
endif

test-%:
	@$(MAKE) --no-print-directory -C $(TEST_DIR)/$* test

check-format:
	@$(if $(FORMAT_FILES),clang-format --dry-run --Werror $(FORMAT_FILES))

fix-format:
	@$(if $(FORMAT_FILES),clang-format -i $(FORMAT_FILES))
	@echo "Форматирование применено"

check-headers:
	@$(if $(HEADERS),$(CXX) $(CXXFLAGS_BASE) $(INCLUDES) -fsyntax-only -x c++ $(HEADERS))

check-tidy:
	@$(if $(SOURCES),clang-tidy --quiet $(SOURCES) -- -std=c++20 $(INCLUDES))

check-cppcheck:
	@cppcheck --enable=warning,style,performance,portability --inconclusive --force \
	  --template='{file}:{line}: ({severity}) {message} [{id}]' \
	  --std=c++20 --language=c++ \
	  --error-exitcode=1 \
	  -i $(TEST_DIR) \
	  $(SRC_DIR)/

check-all: check-format check-headers check-tidy check-cppcheck
	@echo "Все проверки пройдены"
