#  Copyright (c) 2024, Andrei Rusanescu <andreirusanescu154gmail.com>

# Memory Management System

# Overview

This project implements a memory management system that provides a simulated
heap with support for dynamic memory allocation, deallocation, and
fragmentation management, using doubly linked lists. It includes custom
implementations of malloc and free, along with functions to read from and write
to allocated memory blocks, maintaining a record of allocated and free memory
blocks.

# Features:

1. `MALLOC` Command: Simulated malloc implementation that allocates memory blocks
of requested sizes in bytes. Throws "Out of memory" error if there is not enough
free memory.

2. `FREE` Command: Simulated free implementation that returns allocated blocks
back to the free list, handling fragmentation and rebuilding originally adjacent
free blocks. Tracks fragmentation and combines adjacent blocks where
possible to reduce fragmentation.

3. `INIT_HEAP` Command: Sets up the heap with a configurable number of free lists
and memory block sizes.

4. `DUMP_MEMORY` Command: Dumps information about the current state of memory,
including total memory, free memory, allocated blocks, fragmentations, and other
memory operations.

5. `READ` or `WRITE` Operations: Allows reading from and writing to allocated
memory blocks with error handling for invalid memory accesses.
