#pragma once

#define MEMORY_NONE (0)
#define MEMORY_USER (1 << 0)
#define MEMORY_CLEAR (1 << 1)
#define MEMORY_READONLY (1 << 2)
typedef unsigned int MemoryFlags;
