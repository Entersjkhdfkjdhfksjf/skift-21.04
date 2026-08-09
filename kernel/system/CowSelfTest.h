#pragma once

// Temporary, self-contained verification for the copy-on-write primitive
// (kernel/memory/Physical.h's refcounting + Memory.h's
// memory_handle_cow_fault()). Not a permanent feature -- delete this file
// and its one call site in kernel/main.cpp once Stage 2 (real snapshot
// capture) has its own, more meaningful test coverage exercising the same
// mechanism through actual usage.
//
// Panics (via assert()) with a stack trace pointing at the exact failing
// check if anything is wrong, same as any other kernel assertion --
// there's no separate pass/fail return value to check.
void cow_self_test();
