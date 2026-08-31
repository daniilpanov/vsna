CLANG_FORMAT ?= clang-format
.DEFAULT_GOAL := format

ROOT_DIR := .
SOURCE_EXTENSIONS := \
	cpp \
	h
EXCLUDED_DIRS := \
	./.git \
	./out \
	./libs \
	./vcpkg

ifeq ($(OS),Windows_NT)
    CMAKE_BIN := $(shell where cmake 2>nul | head -n 1)
else
    CMAKE_BIN := $(shell which cmake 2>/dev/null)
endif

.PHONY: format build

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

build:
	git submodule init vcpkg
	"$(CMAKE_BIN)" --preset default
	"$(CMAKE_BIN)" --build --preset default
