#include <libsystem/Assert.h>

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

struct MarkCowContext
{
    void *address_space;
    size_t pages_marked;
};

static void mark_page_cow(void *raw_context, uintptr_t virtual_address, uintptr_t physical_address, bool writable)
{
    auto context = reinterpret_cast<MarkCowContext *>(raw_context);

    if (writable)
    {
        Result result = arch_virtual_protect(context->address_space, virtual_address, MEMORY_USER | MEMORY_READONLY);
        assert(result == SUCCESS);
    }

    // Retained either way: a page that's already read-only here means an
    // earlier call already COW-shared it, and this reference needs to be
    // added on top of that one, not replace it -- a future write has to
    // correctly satisfy every outstanding reference, not just the most
    // recent one.
    physical_page_retain(physical_address);

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
        mark_page_cow,
        &context);

    return context.pages_marked;
}
