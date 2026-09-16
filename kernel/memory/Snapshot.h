#pragma once

#include <libsystem/Common.h>
#include <libsystem/Time.h>
#include <libsystem/utils/List.h>

// One page's record within a Snapshot: which virtual address pointed at
// which physical frame at the moment the snapshot was taken. This is the
// piece that was missing before -- memory_mark_address_space_cow() makes
// it SAFE for a snapshot to exist, but doesn't remember anything; a
// Snapshot's page list is what actually makes restoration possible later
// (restoration itself isn't built yet -- this is the data it will need
// once it is).
struct SnapshotPageEntry
{
    uintptr_t virtual_address;
    uintptr_t physical_address;
};

struct Snapshot
{
    int id;
    TimeStamp taken_at;
    void *address_space;
    List *pages; // List<SnapshotPageEntry*>
};

// Marks every currently-writable, present page in address_space's user
// region as copy-on-write (same underlying mechanism as
// memory_mark_address_space_cow()) AND records each one's
// {virtual_address, physical_address} pair, so this snapshot can (once
// restoration exists) actually be restored -- unlike
// memory_mark_address_space_cow() alone, which makes it safe for
// something else to reference these pages without recording what that
// something else would need in order to ever be restored.
//
// Never returns nullptr; an address space with no present user pages yet
// just produces a Snapshot with an empty page list, which is valid, not
// an error.
Snapshot *snapshot_take(void *address_space);

// Releases this snapshot's reference on every physical frame it
// recorded and frees the Snapshot itself. Does not touch the live
// address space's mappings at all -- only gives up this snapshot's own
// claim on the frames it was keeping alive. If nothing else references a
// given frame anymore, it becomes available for reuse; if the live
// mapping (or another snapshot) still does, it survives.
void snapshot_destroy(Snapshot *snapshot);

// Restores address_space's user memory to exactly the state recorded in
// snapshot. Three things happen:
//
//   1. Every page the snapshot recorded is remapped back to the physical
//      frame it pointed at then -- read-only, so the restored state is
//      itself COW-protected and the snapshot stays valid for a future
//      restore. Where the live mapping had diverged (COW gave it a
//      private copy), its reference on that private copy is released.
//   2. Pages that exist live but are ABSENT from the snapshot (allocated
//      after it was taken) are unmapped and their frames released --
//      otherwise the restored state would carry leftovers from a future
//      that no longer happened.
//   3. The snapshot takes a fresh reference on every frame it restored,
//      so it remains independently valid afterward and can be restored
//      again later.
//
// The snapshot is NOT consumed -- call snapshot_destroy() separately when
// it's genuinely no longer wanted.
//
// Only touches memory. Kernel-side state (the task table, open handles,
// IPC connections, timers) is untouched, so this alone is not yet a full
// "revert the system" -- that's the next piece of work.
void snapshot_restore(Snapshot *snapshot);

// Marks every currently-writable, present page in address_space's user
// region as copy-on-write: retains an extra reference
// (physical_page_retain()) on each physical frame and marks writable
// ones read-only (arch_virtual_protect()), so the next write to any of
// them transparently triggers memory_handle_cow_fault() instead of
// corrupting whatever this call was meant to protect.
//
// Lower-level than snapshot_take() -- doesn't record anything, so there's
// no way to know afterward which pages were marked, or restore them.
// Kept for cow_self_test()'s existing coverage of the marking mechanism
// in isolation; snapshot_take() is what anything building real
// snapshot/restore functionality should actually use.
//
// Returns the number of pages marked. 0 is a valid result, not an error.
size_t memory_mark_address_space_cow(void *address_space);
