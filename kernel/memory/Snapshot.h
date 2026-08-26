#pragma once

#include <libsystem/Common.h>

// Marks every currently-present, user-accessible page in address_space's
// user region as copy-on-write: retains an extra reference on each
// physical frame (physical_page_retain()) and -- for pages that are
// currently writable -- marks them read-only (arch_virtual_protect()), so
// the next write to any of them transparently triggers
// memory_handle_cow_fault() instead of corrupting whatever this call was
// meant to preserve.
//
// This is Stage 1's primitive (retain + protect + the fault-handler
// resolution) applied across an entire address space instead of one
// manually-chosen test page -- proving the mechanism scales, which
// cow_self_test() alone didn't.
//
// IMPORTANT GAP, not yet addressed: this only makes it SAFE for a
// snapshot to exist -- it does not yet RECORD one anywhere. Nothing
// remembers which virtual address pointed at which physical frame at the
// moment this was called, which means there is currently no way to
// actually restore anything afterward. That's the next piece: a real
// Snapshot data structure (id, timestamp, and the {virtual_address ->
// physical_address} table this function's callback has all the
// information to build, but currently discards). Calling this function
// today only proves the marking mechanism itself is correct and safe;
// it is not yet a usable "take a snapshot" operation on its own.
//
// Returns the number of pages marked. 0 is a valid result (e.g. an
// address space with no present user pages yet), not an error.
size_t memory_mark_address_space_cow(void *address_space);
