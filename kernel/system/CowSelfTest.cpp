#include <libsystem/Assert.h>
#include <libsystem/Logger.h>

#include "archs/VirtualMemory.h"

#include "kernel/memory/Memory.h"
#include "kernel/memory/Physical.h"
#include "kernel/memory/Snapshot.h"
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

    physical_page_retain(original_physical);
    assert(SUCCESS == arch_virtual_protect(address_space, test_address, MEMORY_READONLY));
    assert(physical_page_refcount(original_physical) == 2);

    assert(memory_handle_cow_fault(address_space, test_address));

    uintptr_t new_physical = arch_virtual_to_physical(address_space, test_address);
    assert(new_physical != original_physical);

    *((volatile uint32_t *)test_address) = 0xBBBBBBBB;
    assert(*((volatile uint32_t *)test_address) == 0xBBBBBBBB);

    MemoryRange original_range{original_physical, ARCH_PAGE_SIZE};
    MemoryRange scratch = arch_virtual_alloc(address_space, original_range, MEMORY_NONE);
    uint32_t original_value = *((volatile uint32_t *)scratch.base());
    arch_virtual_free(address_space, scratch);

    assert(original_value == 0xAAAAAAAA);
    assert(physical_page_refcount(original_physical) == 1);

    logger_info("[COW self-test] shared-page copy: PASS");

    /* --- Test 2: an unshared page (refcount == 1) takes the fast path --- */

    uintptr_t test_address_2 = 0;
    assert(SUCCESS == memory_alloc(address_space, ARCH_PAGE_SIZE, MEMORY_CLEAR, &test_address_2));

    uintptr_t physical_2 = arch_virtual_to_physical(address_space, test_address_2);

    assert(SUCCESS == arch_virtual_protect(address_space, test_address_2, MEMORY_READONLY));
    assert(physical_page_refcount(physical_2) == 1);

    assert(memory_handle_cow_fault(address_space, test_address_2));

    uintptr_t physical_2_after = arch_virtual_to_physical(address_space, test_address_2);
    assert(physical_2_after == physical_2);

    *((volatile uint32_t *)test_address_2) = 0xCCCCCCCC;
    assert(*((volatile uint32_t *)test_address_2) == 0xCCCCCCCC);

    logger_info("[COW self-test] unshared-page fast path: PASS");

    /* --- Test 3: whole-address-space marking (the page-table walker) --- */

    uintptr_t user_page_1 = 0, user_page_2 = 0;
    assert(SUCCESS == memory_alloc(address_space, ARCH_PAGE_SIZE, MEMORY_USER | MEMORY_CLEAR, &user_page_1));
    assert(SUCCESS == memory_alloc(address_space, ARCH_PAGE_SIZE, MEMORY_USER | MEMORY_CLEAR, &user_page_2));

    *((volatile uint32_t *)user_page_1) = 0x11111111;
    *((volatile uint32_t *)user_page_2) = 0x22222222;

    uintptr_t user_physical_1 = arch_virtual_to_physical(address_space, user_page_1);
    uintptr_t user_physical_2 = arch_virtual_to_physical(address_space, user_page_2);

    size_t marked = memory_mark_address_space_cow(address_space);
    assert(marked >= 2);

    assert(physical_page_refcount(user_physical_1) == 2);
    assert(physical_page_refcount(user_physical_2) == 2);

    assert(memory_handle_cow_fault(address_space, user_page_1));
    assert(memory_handle_cow_fault(address_space, user_page_2));

    assert(arch_virtual_to_physical(address_space, user_page_1) != user_physical_1);
    assert(arch_virtual_to_physical(address_space, user_page_2) != user_physical_2);

    *((volatile uint32_t *)user_page_1) = 0x33333333;
    *((volatile uint32_t *)user_page_2) = 0x44444444;
    assert(*((volatile uint32_t *)user_page_1) == 0x33333333);
    assert(*((volatile uint32_t *)user_page_2) == 0x44444444);

    logger_info("[COW self-test] whole-address-space marking: PASS");

    /* --- Test 4: snapshot_take()/snapshot_destroy() record + release --- */

    uintptr_t snap_page = 0;
    assert(SUCCESS == memory_alloc(address_space, ARCH_PAGE_SIZE, MEMORY_USER | MEMORY_CLEAR, &snap_page));
    *((volatile uint32_t *)snap_page) = 0x55555555;

    uintptr_t snap_physical = arch_virtual_to_physical(address_space, snap_page);

    Snapshot *snapshot = snapshot_take(address_space);
    assert(snapshot != nullptr);
    assert(snapshot->id > 0);

    // Confirm our page is actually recorded, with the correct physical
    // address -- not just that SOME pages got marked.
    bool found = false;
    list_foreach(SnapshotPageEntry, entry, snapshot->pages)
    {
        if (entry->virtual_address == snap_page)
        {
            assert(entry->physical_address == snap_physical);
            found = true;
        }
    }
    assert(found);

    assert(physical_page_refcount(snap_physical) == 2); // live mapping + this snapshot's reference

    // Diverge: write through the live mapping, triggering COW.
    assert(memory_handle_cow_fault(address_space, snap_page));
    *((volatile uint32_t *)snap_page) = 0x66666666;
    assert(*((volatile uint32_t *)snap_page) == 0x66666666);

    // The snapshot's OWN recorded frame must still hold the original
    // value, untouched by the write above.
    MemoryRange snap_original_range{snap_physical, ARCH_PAGE_SIZE};
    MemoryRange snap_scratch = arch_virtual_alloc(address_space, snap_original_range, MEMORY_NONE);
    uint32_t snap_original_value = *((volatile uint32_t *)snap_scratch.base());
    arch_virtual_free(address_space, snap_scratch);
    assert(snap_original_value == 0x55555555);

    assert(physical_page_refcount(snap_physical) == 1); // live mapping released; snapshot's reference remains

    snapshot_destroy(snapshot);

    // Reading refcount on a now-freed frame is only meaningful here, to
    // confirm destroy actually released it -- not a generally supported
    // thing to query afterward.
    assert(physical_page_refcount(snap_physical) == 0);

    logger_info("[COW self-test] snapshot_take/snapshot_destroy: PASS");

    /* --- Test 5: snapshot_restore() actually reverts memory ------------- */

    uintptr_t restore_page = 0;
    assert(SUCCESS == memory_alloc(address_space, ARCH_PAGE_SIZE, MEMORY_USER | MEMORY_CLEAR, &restore_page));
    *((volatile uint32_t *)restore_page) = 0x77777777;

    uintptr_t restore_physical = arch_virtual_to_physical(address_space, restore_page);

    Snapshot *restore_snapshot = snapshot_take(address_space);

    // Diverge the live mapping away from the snapshot.
    assert(memory_handle_cow_fault(address_space, restore_page));
    *((volatile uint32_t *)restore_page) = 0x88888888;
    assert(*((volatile uint32_t *)restore_page) == 0x88888888);
    assert(arch_virtual_to_physical(address_space, restore_page) != restore_physical);

    // Allocate a page AFTER the snapshot -- restore should drop it.
    uintptr_t after_page = 0;
    assert(SUCCESS == memory_alloc(address_space, ARCH_PAGE_SIZE, MEMORY_USER | MEMORY_CLEAR, &after_page));
    *((volatile uint32_t *)after_page) = 0x99999999;
    assert(arch_virtual_present(address_space, after_page));

    snapshot_restore(restore_snapshot);

    // The diverged page is back to its snapshot frame and value.
    assert(arch_virtual_to_physical(address_space, restore_page) == restore_physical);
    assert(*((volatile uint32_t *)restore_page) == 0x77777777);

    // The page allocated after the snapshot is gone.
    assert(!arch_virtual_present(address_space, after_page));

    // Restored state is COW-protected again: writing triggers a real
    // copy rather than silently corrupting the snapshot's own frame.
    assert(memory_handle_cow_fault(address_space, restore_page));
    *((volatile uint32_t *)restore_page) = 0xABABABAB;
    assert(*((volatile uint32_t *)restore_page) == 0xABABABAB);

    // ...and the snapshot survived that write intact, so it could be
    // restored again.
    MemoryRange restore_check_range{restore_physical, ARCH_PAGE_SIZE};
    MemoryRange restore_scratch = arch_virtual_alloc(address_space, restore_check_range, MEMORY_NONE);
    uint32_t restore_snapshot_value = *((volatile uint32_t *)restore_scratch.base());
    arch_virtual_free(address_space, restore_scratch);
    assert(restore_snapshot_value == 0x77777777);

    snapshot_destroy(restore_snapshot);

    logger_info("[COW self-test] snapshot_restore: PASS");

    memory_free(address_space, MemoryRange{test_address, ARCH_PAGE_SIZE});
    memory_free(address_space, MemoryRange{test_address_2, ARCH_PAGE_SIZE});
    memory_free(address_space, MemoryRange{user_page_1, ARCH_PAGE_SIZE});
    memory_free(address_space, MemoryRange{user_page_2, ARCH_PAGE_SIZE});
    memory_free(address_space, MemoryRange{snap_page, ARCH_PAGE_SIZE});
    memory_free(address_space, MemoryRange{restore_page, ARCH_PAGE_SIZE});

    logger_info("[COW self-test] ALL TESTS PASSED");
}
