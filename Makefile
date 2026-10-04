.PHONY: check-format check-tidy check-cppcheck check-all

check-format:
	@find src test \( -name '*.cpp' -o -name '*.hpp' -o -name '*.h' \) -print0 2>/dev/null \
	  | xargs -0 -r clang-format --dry-run --Werror
	@echo "Форматирование корректно"

check-tidy:
	@find src -name '*.cpp' -print0 2>/dev/null \
	  | xargs -0 -r clang-tidy --quiet -- -std=c++20 -Isrc

check-cppcheck:
	@if [ ! -d src ]; then \
	  exit 0; \
	fi
	@cppcheck --enable=all --inconclusive --force \
	  --template='{file}:{line}: ({severity}) {message} [{id}]' \
	  --std=c++20 --language=c++ \
	  --suppress=missingIncludeSystem \
	  --suppress=unknownMacro \
	  --error-exitcode=1 \
	  src/

check-all: check-format check-tidy check-cppcheck
	@echo "Все проверки пройдены"
