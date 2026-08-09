#pragma once

#include <libsystem/Common.h>

#include "kernel/memory/MemoryRange.h"

extern size_t TOTAL_MEMORY;
extern size_t USED_MEMORY;
extern uint8_t MEMORY[1024 * 1024 / 8];

MemoryRange physical_alloc(size_t size);

void physical_free(MemoryRange range);

bool physical_is_used(MemoryRange range);

void physical_set_used(MemoryRange range);

void physical_set_free(MemoryRange range);

// Every physical page transitions to refcount=1 the moment it becomes
// used (see physical_set_used()) -- one implicit owner, same as before
// refcounting existed. physical_free()/physical_set_free() decrement on
// release and only actually return the frame to the free pool once the
// count reaches 0.
//
// Nothing outside physical_page_retain() itself needs to call this yet --
// it exists now so later work (COW snapshots sharing an already-allocated
// page between a live mapping and a frozen snapshot) has a correct
// primitive to build on, without physical_free() needing to change again
// when that lands.
//
// Capacity note: refcounts are only tracked for pages within the first
// 1GiB of physical memory -- comfortably past any RAM this hobby OS
// targets today, but not the bitmap's full theoretical 32GiB range,
// which would cost 8MiB of permanently-reserved kernel memory for
// tracking capacity that will never be used. physical_page_retain()
// asserts (loudly, rather than silently corrupting adjacent memory) if
// ever asked to track a page outside that range.
void physical_page_retain(uintptr_t address);

uint8_t physical_page_refcount(uintptr_t address);
