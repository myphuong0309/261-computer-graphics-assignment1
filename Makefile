# Top-level Makefile - builds / runs both parts of Assignment 1.
#   make            build part 1 and part 2
#   make part1      build only part 1          make run1   run part 1
#   make part2      build only part 2          make run2   run part 2
#   make test       run part 1 unit tests      make clean  remove build output of both

.PHONY: all part1 part2 run1 run2 test clean

all: part1 part2

part1:
	$(MAKE) -C part1_shapes

part2:
	$(MAKE) -C part2_molecules

run1:
	$(MAKE) -C part1_shapes run

run2:
	$(MAKE) -C part2_molecules run

test:
	$(MAKE) -C part1_shapes test

clean:
	$(MAKE) -C part1_shapes clean
	$(MAKE) -C part2_molecules clean
