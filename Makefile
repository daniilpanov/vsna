ROOT_DIR := .
SOURCE_EXTENSIONS := cpp h
EXCLUDED_DIRS := ./.git ./out ./libs ./vcpkg

CLANG_FORMAT ?= clang-format
CMAKE_BIN ?= cmake

.PHONY: format configure build
.DEFAULT_GOAL := format

list-format-files:
	@find "$(ROOT_DIR)" -type f \
		\( \
			$(foreach ext,$(SOURCE_EXTENSIONS), -name '*.$(ext)' -o) \
			-name '*.cpp' \
		\) \
		$(foreach dir,$(EXCLUDED_DIRS), -not -path '$(dir)' -not -path '$(dir)/*') \
		-print

format:
	@files="$$( $(MAKE) --no-print-directory list-format-files )"; \
	if [ -z "$$files" ]; then \
		echo "Нет файлов для форматирования"; \
	else \
		echo "Форматирование файлов..."; \
		echo "$$files"; \
		${CLANG_FORMAT} -i $$files;\
	fi

configure:
	git submodule init vcpkg
	"$(CMAKE_BIN)" --preset default

build:
	"$(CMAKE_BIN)" --build --preset default
