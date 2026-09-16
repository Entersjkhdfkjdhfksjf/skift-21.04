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

/* --- snapshot_restore() ----------------------------------------------------- */

struct CollectLivePagesContext
{
    List *live_pages; // List<SnapshotPageEntry*> -- reused purely as a
                      // {virtual, physical} pair holder, not a snapshot
};

static void collect_live_page_callback(void *raw_context, uintptr_t virtual_address, uintptr_t physical_address, bool writable)
{
    __unused(writable);

    auto context = reinterpret_cast<CollectLivePagesContext *>(raw_context);

    auto entry = new SnapshotPageEntry{virtual_address, physical_address};
    list_pushback(context->live_pages, entry);
}

static void destroy_live_page_entry(void *raw_entry)
{
    // Only frees the bookkeeping struct -- these entries never owned a
    // reference on the frame they describe, unlike a Snapshot's own
    // entries, so there's deliberately no physical_free() here.
    delete reinterpret_cast<SnapshotPageEntry *>(raw_entry);
}

static SnapshotPageEntry *find_page_entry(List *pages, uintptr_t virtual_address)
{
    list_foreach(SnapshotPageEntry, entry, pages)
    {
        if (entry->virtual_address == virtual_address)
        {
            return entry;
        }
    }

    return nullptr;
}

void snapshot_restore(Snapshot *snapshot)
{
    ASSERT_INTERRUPTS_RETAINED();

    void *address_space = snapshot->address_space;

    // Collect the live page set BEFORE mutating anything -- the walker
    // reads the same page tables the remapping below modifies, so doing
    // both at once would be walking a structure while changing it.
    List *live_pages = list_create();
    CollectLivePagesContext collect_context{live_pages};

    arch_virtual_for_each_present_page(
        address_space,
        USER_MEMORY_START,
        USER_MEMORY_END,
        collect_live_page_callback,
        &collect_context);

    // 1. Drop pages that exist live but aren't in the snapshot -- these
    //    were allocated after it was taken, and shouldn't survive a
    //    revert to a moment before they existed.
    list_foreach(SnapshotPageEntry, live_entry, live_pages)
    {
        if (find_page_entry(snapshot->pages, live_entry->virtual_address) == nullptr)
        {
            arch_virtual_free(address_space, MemoryRange{live_entry->virtual_address, ARCH_PAGE_SIZE});
            physical_free(MemoryRange{live_entry->physical_address, ARCH_PAGE_SIZE});
        }
    }

    // 2. Put every recorded page back, read-only so the restored state is
    //    itself COW-protected and this snapshot stays restorable.
    list_foreach(SnapshotPageEntry, snapshot_entry, snapshot->pages)
    {
        auto live_entry = find_page_entry(live_pages, snapshot_entry->virtual_address);

        if (live_entry != nullptr && live_entry->physical_address == snapshot_entry->physical_address)
        {
            // Never diverged -- still pointing at the snapshot's own
            // frame. Only the protection needs reasserting; deliberately
            // no extra retain here, since the reference this snapshot
            // already holds was never released.
            Result result = arch_virtual_protect(address_space, snapshot_entry->virtual_address, MEMORY_USER | MEMORY_READONLY);
            assert(result == SUCCESS);
            continue;
        }

        if (live_entry != nullptr)
        {
            // Diverged: COW gave the live mapping a private copy. Release
            // the live mapping's reference on that copy -- it's about to
            // be replaced and nothing else should be holding it.
            physical_free(MemoryRange{live_entry->physical_address, ARCH_PAGE_SIZE});
        }

        MemoryRange restored_range{snapshot_entry->physical_address, ARCH_PAGE_SIZE};
        Result result = arch_virtual_map(address_space, restored_range, snapshot_entry->virtual_address, MEMORY_USER | MEMORY_READONLY);
        assert(result == SUCCESS);

        // The live mapping now points at the snapshot's frame again, so
        // that frame has one more referent than it did a moment ago.
        physical_page_retain(snapshot_entry->physical_address);
    }

    list_destroy_with_callback(live_pages, destroy_live_page_entry);
}
