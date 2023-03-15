CC = gcc
CFLAGS = -Wall -Werror

all: a2

a2: a2.o a2.h
	$(CC) $(CFLAGS)	$^ -o $@ 

a2.o: a2.c a2.h
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: help
help:
	@echo "Usage: make [target]"
	@echo ""
	@echo "Targets:"
	@echo "  all    - Build the a2 executable (default target)"
	@echo "  clean  - Remove all generated files"
	@echo "  help   - Display this help message"

.PHONY: clean
clean:
	rm -f a2.o a2