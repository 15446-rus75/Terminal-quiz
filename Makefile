CXX      := g++
CXXFLAGS := -std=c++20 -Wall -Wextra -Wpedantic -O2 -g -MMD -MP
INCLUDES := -Isrc
LIBS     := -lpqxx -lcurl

SRC_DIR   := src
BUILD_DIR := build
BIN       := $(BUILD_DIR)/terminal_quiz

SOURCES := $(shell find $(SRC_DIR) -name '*.cpp' 2>/dev/null)
OBJECTS := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SOURCES))

.PHONY: build run clean check-format check-tidy check-cppcheck check-all fix-format

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

check-format:
	@find $(SRC_DIR) test \( -name '*.cpp' -o -name '*.hpp' -o -name '*.h' \) -print0 2>/dev/null \
	  | xargs -0 -r clang-format --dry-run --Werror

fix-format:
	@find $(SRC_DIR) test \( -name '*.cpp' -o -name '*.hpp' -o -name '*.h' \) -print0 2>/dev/null \
	  | xargs -0 -r clang-format -i
	@echo "Форматирование применено"

check-tidy:
	@find $(SRC_DIR) -name '*.cpp' -print0 2>/dev/null \
	  | xargs -0 -r -I {} clang-tidy --quiet {} -- -std=c++20 -I$(SRC_DIR)

check-cppcheck:
	@cppcheck --enable=warning,style,performance,portability --inconclusive --force \
	  --template='{file}:{line}: ({severity}) {message} [{id}]' \
	  --std=c++20 --language=c++ \
	  --error-exitcode=1 \
	  $(SRC_DIR)/

check-all: check-format check-tidy check-cppcheck
	@echo "Все проверки пройдены"
