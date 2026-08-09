#include <libsystem/Assert.h>
#include <libsystem/Logger.h>

#include "archs/VirtualMemory.h"

#include "kernel/memory/Memory.h"
#include "kernel/memory/Physical.h"
#include "kernel/system/CowSelfTest.h"

void cow_self_test()
{
    logger_info("[COW self-test] starting...");

    void *address_space = arch_kernel_address_space();

    /* --- Test 1: a genuinely shared page (refcount > 1) gets copied ------ */

    uintptr_t test_address = 0;
    assert(SUCCESS == memory_alloc(address_space, ARCH_PAGE_SIZE, MEMORY_CLEAR, &test_address));

    *((volatile uint32_t *)test_address) = 0xAAAAAAAA;

    uintptr_t original_physical = arch_virtual_to_physical(address_space, test_address);

    // Simulate "a snapshot exists": something else now also references
    // this frame, same as the real snapshot mechanism will do later.
    physical_page_retain(original_physical);
    assert(SUCCESS == arch_virtual_protect(address_space, test_address, MEMORY_READONLY));
    assert(physical_page_refcount(original_physical) == 2);

    // This is the exact call the fault handler makes. Called directly
    // here rather than by actually triggering a hardware fault, since the
    // fault handler only attempts this for userspace (ring 3) faults --
    // this test runs in kernel context, before any user task exists yet.
    assert(memory_handle_cow_fault(address_space, test_address));

    uintptr_t new_physical = arch_virtual_to_physical(address_space, test_address);
    assert(new_physical != original_physical); // a real, distinct copy was made

    // The live mapping is writable again, with its own private copy.
    *((volatile uint32_t *)test_address) = 0xBBBBBBBB;
    assert(*((volatile uint32_t *)test_address) == 0xBBBBBBBB);

    // The ORIGINAL frame must be untouched -- read it back through a
    // temporary mapping, since nothing points at it directly anymore
    // after the remap above.
    MemoryRange original_range{original_physical, ARCH_PAGE_SIZE};
    MemoryRange scratch = arch_virtual_alloc(address_space, original_range, MEMORY_NONE);
    uint32_t original_value = *((volatile uint32_t *)scratch.base());
    arch_virtual_free(address_space, scratch);

    assert(original_value == 0xAAAAAAAA); // the "snapshot" is intact, unmodified

    // The live mapping released its reference; only the simulated
    // snapshot's reference remains.
    assert(physical_page_refcount(original_physical) == 1);

    logger_info("[COW self-test] shared-page copy: PASS");

    /* --- Test 2: an unshared page (refcount == 1) takes the fast path --- */

    uintptr_t test_address_2 = 0;
    assert(SUCCESS == memory_alloc(address_space, ARCH_PAGE_SIZE, MEMORY_CLEAR, &test_address_2));

    uintptr_t physical_2 = arch_virtual_to_physical(address_space, test_address_2);

    // Marked read-only, but nothing else was ever made to reference it --
    // refcount is still 1, same as any normal, non-shared page.
    assert(SUCCESS == arch_virtual_protect(address_space, test_address_2, MEMORY_READONLY));
    assert(physical_page_refcount(physical_2) == 1);

    assert(memory_handle_cow_fault(address_space, test_address_2));

    uintptr_t physical_2_after = arch_virtual_to_physical(address_space, test_address_2);
    assert(physical_2_after == physical_2); // no copy -- same frame, made writable in place

    *((volatile uint32_t *)test_address_2) = 0xCCCCCCCC;
    assert(*((volatile uint32_t *)test_address_2) == 0xCCCCCCCC);

    logger_info("[COW self-test] unshared-page fast path: PASS");

    memory_free(address_space, MemoryRange{test_address, ARCH_PAGE_SIZE});
    memory_free(address_space, MemoryRange{test_address_2, ARCH_PAGE_SIZE});

    logger_info("[COW self-test] ALL TESTS PASSED");
}
