CXX           := g++
CXXFLAGS_BASE := -std=c++20 -Wall -Wextra -Wpedantic
CXXFLAGS      := $(CXXFLAGS_BASE) -O2 -g -MMD -MP
INCLUDES      := -Isrc
LIBS          := -lpqxx -lpq -lcurl

SRC_DIR   := src
TEST_DIR  := $(SRC_DIR)/test
BUILD_DIR := build
BIN       := $(BUILD_DIR)/terminal_quiz

SOURCES := $(shell find $(SRC_DIR) -path $(TEST_DIR) -prune -o -name '*.cpp' -print 2>/dev/null)
OBJECTS := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SOURCES))
HEADERS := $(shell find $(SRC_DIR) -path $(TEST_DIR) -prune -o -name '*.hpp' -print 2>/dev/null)

BOOST_FLAGS    := -DBOOST_TEST_DYN_LINK
BOOST_LIBS     := -lboost_unit_test_framework
TEST_CXXFLAGS  := $(CXXFLAGS_BASE) -O0 -g
TEST_BUILD_DIR := $(CURDIR)/$(BUILD_DIR)/test
TEST_MODULES   := $(patsubst %/Makefile,%,$(wildcard $(TEST_DIR)/*/Makefile))

export CXX TEST_CXXFLAGS LIBS BOOST_FLAGS BOOST_LIBS TEST_BUILD_DIR

.PHONY: build run clean test check-format check-headers check-tidy check-cppcheck check-all fix-format

build: $(BIN)

$(BIN): $(OBJECTS)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(OBJECTS) -o $@ $(LIBS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

-include $(OBJECTS:.o=.d)

run: build
	./$(BIN)

clean:
	rm -rf $(BUILD_DIR)

test:
	@if [ -z "$(TEST_MODULES)" ]; then \
	  echo "Тестовых модулей не найдено (ожидаются $(TEST_DIR)/<модуль>/Makefile)"; \
	else \
	  failed=""; \
	  for d in $(TEST_MODULES); do \
	    echo "==> $$d"; \
	    $(MAKE) --no-print-directory -C $$d test || failed="$$failed $$(basename $$d)"; \
	  done; \
	  if [ -n "$$failed" ]; then echo "Тесты упали в модулях:$$failed"; exit 1; fi; \
	  echo "Все тесты пройдены"; \
	fi

test-%:
	@$(MAKE) --no-print-directory -C $(TEST_DIR)/$* test

check-format:
	@find $(SRC_DIR) \( -name '*.cpp' -o -name '*.hpp' -o -name '*.h' \) -print0 2>/dev/null \
	  | xargs -0 -r clang-format --dry-run --Werror

fix-format:
	@find $(SRC_DIR) \( -name '*.cpp' -o -name '*.hpp' -o -name '*.h' \) -print0 2>/dev/null \
	  | xargs -0 -r clang-format -i
	@echo "Форматирование применено"

check-headers:
	@failed=0; \
	for h in $(HEADERS); do \
	  $(CXX) $(CXXFLAGS_BASE) $(INCLUDES) -fsyntax-only -x c++ $$h || failed=1; \
	done; \
	exit $$failed

check-tidy:
	@find $(SRC_DIR) -path $(TEST_DIR) -prune -o -name '*.cpp' -print0 2>/dev/null \
	  | xargs -0 -r -I {} clang-tidy --quiet {} -- -std=c++20 -I$(SRC_DIR)

check-cppcheck:
	@cppcheck --enable=warning,style,performance,portability --inconclusive --force \
	  --template='{file}:{line}: ({severity}) {message} [{id}]' \
	  --std=c++20 --language=c++ \
	  --error-exitcode=1 \
	  -i $(TEST_DIR) \
	  $(SRC_DIR)/

check-all: check-format check-headers check-tidy check-cppcheck
	@echo "Все проверки пройдены"
