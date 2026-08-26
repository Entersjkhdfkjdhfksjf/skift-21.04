#pragma once

#include <abi/Memory.h>

#include <libsystem/Result.h>

#include "kernel/memory/MemoryRange.h"

void *arch_kernel_address_space();

void arch_virtual_initialize();

void arch_virtual_memory_enable();

bool arch_virtual_present(void *address_space, uintptr_t virtual_address);

uintptr_t arch_virtual_to_physical(void *address_space, uintptr_t virtual_address);

Result arch_virtual_map(void *address_space, MemoryRange physical_range, uintptr_t virtual_address, MemoryFlags flags);

// Changes the protection flags on a page that is ALREADY mapped, without
// touching what physical frame it points to. Returns ERR_BAD_ADDRESS if
// the page isn't currently present -- this is a protection change, not an
// implicit map.
//
// Currently only MEMORY_READONLY (clear vs set the hardware Write bit)
// and MEMORY_USER are respected; anything else in flags is ignored.
Result arch_virtual_protect(void *address_space, uintptr_t virtual_address, MemoryFlags flags);

MemoryRange arch_virtual_alloc(void *address_space, MemoryRange physical_range, MemoryFlags flags);

void arch_virtual_free(void *address_space, MemoryRange virtual_range);

// Calls callback once for every currently PRESENT page whose virtual
// address falls within [start, end) in address_space, passing the page's
// virtual address, its physical address, and whether it's currently
// writable.
//
// Skips whole not-present page-directory regions (4MiB at a time on
// x86_32) rather than probing every possible page address individually
// via arch_virtual_present() in a loop -- callers walking a large,
// mostly-sparse range (like a task's entire user address space) should
// prefer this over doing that themselves.
void arch_virtual_for_each_present_page(
    void *address_space,
    uintptr_t start,
    uintptr_t end,
    void (*callback)(void *context, uintptr_t virtual_address, uintptr_t physical_address, bool writable),
    void *context);

void *arch_address_space_create();

void arch_address_space_destroy(void *address_space);

void arch_address_space_switch(void *address_space);
