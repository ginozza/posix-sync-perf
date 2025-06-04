CC := gcc
CFLAGS := -Wall -Wextra -lpthread
TARGETS := Dining\ Philosopher/mutex-cond Dining\ Philosopher/sem Dining\ Philosopher/spin-lock Dining\ Philosopher/starving

.PHONY: all clean

all: $(TARGETS)
	@echo "+ Compilacion exitosa"

Dining\ Philosopher/%: Dining\ Philosopher/%.c
	@"$(CC)" $(CFLAGS) -o "$@" "$<"

clean:
	@rm -f $(TARGETS)
