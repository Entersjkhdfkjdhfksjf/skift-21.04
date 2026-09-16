#include <libsystem/Assert.h>

#include "archs/Architectures.h"
#include "archs/VirtualMemory.h"

#include "kernel/interrupts/Interupts.h"
#include "kernel/memory/Physical.h"
#include "kernel/memory/Snapshot.h"

// Matches arch_virtual_alloc()'s own user-memory range (see
// archs/x86_32/kernel/VirtualMemory.cpp): page indices 256*1024 to
// 1024*1024, i.e. the upper 3GiB.
//
// The true exclusive end of that range is page index 1024*1024, which as
// an address is exactly 2^32 -- not representable in a 32-bit uintptr_t
// (it silently wraps to 0). Using 0xFFFFFFFF here instead of that
// unrepresentable value still walks the very last page correctly: the
// walker's own bounds check is virtual_address >= end, and the last
// valid page starts at 0xFFFFF000, which is still < 0xFFFFFFFF.
#define USER_MEMORY_START ((uintptr_t)256u * 1024u * ARCH_PAGE_SIZE)
#define USER_MEMORY_END ((uintptr_t)0xFFFFFFFF)

// Shared by both memory_mark_address_space_cow() and snapshot_take():
// retain a reference on this physical frame, and if it's currently
// writable, make it read-only so a future write has to go through
// memory_handle_cow_fault(). An already-read-only page (an earlier call
// already COW-shared it) still gets an additional reference here rather
// than being skipped -- a future write has to correctly satisfy every
// outstanding reference, not just the most recent one.
static void mark_single_page_cow(void *address_space, uintptr_t virtual_address, uintptr_t physical_address, bool writable)
{
    if (writable)
    {
        Result result = arch_virtual_protect(address_space, virtual_address, MEMORY_USER | MEMORY_READONLY);
        assert(result == SUCCESS);
    }

    physical_page_retain(physical_address);
}

/* --- memory_mark_address_space_cow() -------------------------------------- */

struct MarkCowContext
{
    void *address_space;
    size_t pages_marked;
};

static void mark_page_cow_callback(void *raw_context, uintptr_t virtual_address, uintptr_t physical_address, bool writable)
{
    auto context = reinterpret_cast<MarkCowContext *>(raw_context);

    mark_single_page_cow(context->address_space, virtual_address, physical_address, writable);

    context->pages_marked++;
}

size_t memory_mark_address_space_cow(void *address_space)
{
    ASSERT_INTERRUPTS_RETAINED();

    MarkCowContext context{address_space, 0};

    arch_virtual_for_each_present_page(
        address_space,
        USER_MEMORY_START,
        USER_MEMORY_END,
        mark_page_cow_callback,
        &context);

    return context.pages_marked;
}

/* --- snapshot_take() / snapshot_destroy() ---------------------------------- */

static int _next_snapshot_id = 1;

struct SnapshotBuildContext
{
    void *address_space;
    List *pages;
};

static void record_and_mark_page_callback(void *raw_context, uintptr_t virtual_address, uintptr_t physical_address, bool writable)
{
    auto context = reinterpret_cast<SnapshotBuildContext *>(raw_context);

    mark_single_page_cow(context->address_space, virtual_address, physical_address, writable);

    auto entry = new SnapshotPageEntry{virtual_address, physical_address};
    list_pushback(context->pages, entry);
}

Snapshot *snapshot_take(void *address_space)
{
    ASSERT_INTERRUPTS_RETAINED();

    auto snapshot = new Snapshot{};
    snapshot->id = _next_snapshot_id++;
    snapshot->taken_at = arch_get_time();
    snapshot->address_space = address_space;
    snapshot->pages = list_create();

    SnapshotBuildContext context{address_space, snapshot->pages};

    arch_virtual_for_each_present_page(
        address_space,
        USER_MEMORY_START,
        USER_MEMORY_END,
        record_and_mark_page_callback,
        &context);

    return snapshot;
}

static void destroy_snapshot_page_entry(void *raw_entry)
{
    auto entry = reinterpret_cast<SnapshotPageEntry *>(raw_entry);

    // Same call physical_free() always was -- this just happens to be a
    // single-page range with no virtual mapping behind it from this
    // snapshot's point of view. Decrements the frame's refcount, and
    // actually frees it if this was the last reference.
    physical_free(MemoryRange{entry->physical_address, ARCH_PAGE_SIZE});

    delete entry;
}

void snapshot_destroy(Snapshot *snapshot)
{
    list_destroy_with_callback(snapshot->pages, destroy_snapshot_page_entry);

    delete snapshot;
}
