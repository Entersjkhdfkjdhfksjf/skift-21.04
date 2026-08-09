#pragma once

#include <abi/Memory.h>
#include <libsystem/Result.h>

#include "kernel/handover/Handover.h"
#include "kernel/memory/MemoryRange.h"

void memory_initialize(Handover *handover);

void memory_dump();

size_t memory_get_used();

size_t memory_get_total();

Result memory_map(void *address_space, MemoryRange range, MemoryFlags flags);

Result memory_map_identity(void *address_space, MemoryRange range, MemoryFlags flags);

Result memory_alloc(void *address_space, size_t size, MemoryFlags flags, uintptr_t *out_address);

Result memory_alloc_identity(void *address_space, MemoryFlags flags, uintptr_t *out_address);

Result memory_free(void *address_space, MemoryRange range);

// Attempts to resolve a page fault as a copy-on-write fault.
//
// If faulting_address falls on a page that's currently mapped and
// read-only: when something else still references the underlying
// physical frame (physical_page_refcount() > 1 -- e.g. a snapshot),
// allocates a fresh physical page, copies the old page's contents into
// it, remaps faulting_address onto the new page as writable, and
// releases this address space's reference on the original frame. When
// nothing else references it (refcount == 1, e.g. the snapshot that
// made it read-only was since discarded), skips the copy entirely and
// just makes the existing frame writable again in place.
//
// Returns true if the fault was resolved this way (safe to resume
// execution at the faulting instruction) or false if faulting_address
// isn't even mapped, meaning this wasn't a COW-resolvable fault at all
// -- the caller should fall through to normal fault handling.
bool memory_handle_cow_fault(void *address_space, uintptr_t faulting_address);
