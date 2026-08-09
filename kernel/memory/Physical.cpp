#include "archs/Memory.h"

#include "kernel/interrupts/Interupts.h"
#include "kernel/memory/Physical.h"
#include "kernel/system/System.h"

size_t TOTAL_MEMORY = 0;
size_t USED_MEMORY = 0;

size_t best_bet = 0;
uint8_t MEMORY[1024 * 1024 / 8] = {};

// See the capacity note on physical_page_retain() in Physical.h for why
// this is deliberately smaller than MEMORY[]'s theoretical range.
#define PHYSICAL_REFCOUNT_MAX_PAGES ((1024u * 1024u * 1024u) / ARCH_PAGE_SIZE)
static uint8_t PAGE_REFCOUNT[PHYSICAL_REFCOUNT_MAX_PAGES] = {};

static size_t physical_refcount_index(uintptr_t address)
{
    uintptr_t page = address / ARCH_PAGE_SIZE;

    assert(page < PHYSICAL_REFCOUNT_MAX_PAGES);

    return page;
}

void physical_page_retain(uintptr_t address)
{
    ASSERT_INTERRUPTS_RETAINED();

    size_t index = physical_refcount_index(address);

    if (PAGE_REFCOUNT[index] == 0)
    {
        // This page predates refcounting being wired into whatever path
        // allocated it (e.g. the early-boot memory-map bootstrap below,
        // which marks pages used via a raw bitmap write before
        // physical_set_used() is ever called on them) but is legitimately
        // in use -- treat it as having had exactly one implicit owner.
        PAGE_REFCOUNT[index] = 1;
    }

    assert(PAGE_REFCOUNT[index] < 255); // don't silently wrap a uint8_t

    PAGE_REFCOUNT[index]++;
}

uint8_t physical_page_refcount(uintptr_t address)
{
    ASSERT_INTERRUPTS_RETAINED();

    return PAGE_REFCOUNT[physical_refcount_index(address)];
}

// Returns true if this was the last reference and the frame is now free
// to actually return to the allocator.
static bool physical_page_refcount_release(uintptr_t address)
{
    size_t index = physical_refcount_index(address);

    if (PAGE_REFCOUNT[index] == 0)
    {
        // Same bootstrap-gap case as above: never had a refcount
        // initialized, but is being freed now -- treat it as the one
        // implicit owner departing.
        return true;
    }

    PAGE_REFCOUNT[index]--;

    return PAGE_REFCOUNT[index] == 0;
}

bool physical_page_is_used(uintptr_t address)
{
    uintptr_t page = address / ARCH_PAGE_SIZE;

    return MEMORY[page / 8] & (1 << (page % 8));
}

void physical_page_set_used(uintptr_t address)
{
    uintptr_t page = address / ARCH_PAGE_SIZE;

    if (page == best_bet)
    {
        best_bet++;
    }

    MEMORY[page / 8] |= 1 << (page % 8);
}

void physical_page_set_free(uintptr_t address)
{
    uintptr_t page = address / ARCH_PAGE_SIZE;

    if (page < best_bet)
    {
        best_bet = page;
    }

    MEMORY[page / 8] &= ~(1 << (page % 8));
}

MemoryRange physical_alloc(size_t size)
{
    ASSERT_INTERRUPTS_RETAINED();

    assert(IS_PAGE_ALIGN(size));

    for (size_t i = best_bet; i < ((TOTAL_MEMORY - size) / ARCH_PAGE_SIZE); i++)
    {
        MemoryRange range(i * ARCH_PAGE_SIZE, size);

        if (!physical_is_used(range))
        {
            physical_set_used(range);
            return range;
        }
    }

    system_panic("Out of physical memory!\tTrying to allocat %dkio but free memory is %dkio !", size / 1024, (TOTAL_MEMORY - USED_MEMORY) / 1024);
    return MemoryRange();
}

void physical_free(MemoryRange range)
{
    ASSERT_INTERRUPTS_RETAINED();

    assert(range.is_page_aligned());

    physical_set_free(range);
}

bool physical_is_used(MemoryRange range)
{
    ASSERT_INTERRUPTS_RETAINED();

    assert(range.is_page_aligned());

    for (size_t i = 0; i < range.page_count(); i++)
    {
        uintptr_t address = range.base() + i * ARCH_PAGE_SIZE;

        if (physical_page_is_used(address))
        {
            return true;
        }
    }

    return false;
}

void physical_set_used(MemoryRange range)
{
    ASSERT_INTERRUPTS_RETAINED();

    assert(range.is_page_aligned());

    for (size_t i = 0; i < range.page_count(); i++)
    {
        uintptr_t address = range.base() + i * ARCH_PAGE_SIZE;

        if (!physical_page_is_used(address))
        {
            USED_MEMORY += ARCH_PAGE_SIZE;
            physical_page_set_used(address);

            // Freshly transitioned free->used: exactly one owner so far.
            size_t index = physical_refcount_index(address);
            PAGE_REFCOUNT[index] = 1;
        }
    }
}

void physical_set_free(MemoryRange range)
{
    ASSERT_INTERRUPTS_RETAINED();

    assert(range.is_page_aligned());

    for (size_t i = 0; i < range.page_count(); i++)
    {
        uintptr_t address = range.base() + i * ARCH_PAGE_SIZE;

        if (physical_page_is_used(address))
        {
            if (!physical_page_refcount_release(address))
            {
                // Another owner still holds this page (e.g. a COW
                // snapshot, once that exists) -- this caller's reference
                // is released, but the frame stays allocated until the
                // last owner frees it.
                continue;
            }

            USED_MEMORY -= ARCH_PAGE_SIZE;
            physical_page_set_free(address);
        }
    }
}
