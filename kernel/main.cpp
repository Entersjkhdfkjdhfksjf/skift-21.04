/*                       _   _     _ _____ ____ _____                         */
/*                      | | | |   | | ____|  _ \_   _|                        */
/*                      | |_| |_  | |  _| | |_) || |                          */
/*                      |  _  | |_| | |___|  _ < | |                          */
/*                      |_| |_|\___/|_____|_| \_\|_|                          */
/*                                                                            */

#include <libsystem/Assert.h>
#include <libsystem/Logger.h>

#include "kernel/devices/Devices.h"
#include "kernel/devices/Driver.h"
#include "kernel/filesystem/DevicesFileSystem.h"
#include "kernel/filesystem/Filesystem.h"
#include "kernel/graphics/Graphics.h"
#include "kernel/interrupts/Interupts.h"
#include "kernel/modules/Modules.h"
#include "kernel/node/DevicesInfo.h"
#include "kernel/node/ProcessInfo.h"
#include "kernel/scheduling/Scheduler.h"
#include "kernel/system/CowSelfTest.h"
#include "kernel/system/System.h"
#include "kernel/tasking/Tasking.h"
#include "kernel/tasking/Userspace.h"

static void splash_screen()
{
    stream_format(log_stream, "\n");
    stream_format(log_stream, "                         _   _     _ _____ ____ _____                           \n");
    stream_format(log_stream, "                        | | | |   | | ____|  _ \\_   _|                          \n");
    stream_format(log_stream, "                        | |_| |_  | |  _| | |_) || |                            \n");
    stream_format(log_stream, "                        |  _  | |_| | |___|  _ < | |                            \n");
    stream_format(log_stream, "                        |_| |_|\\___/|_____|_| \\_\\|_|                            \n");
    stream_format(log_stream, "                                                                                \n");
    stream_format(log_stream, "\u001b[34;1m--------------------------------------------------------------------------------\e[0m\n");
    stream_format(log_stream, "                              Copyright (c) 2018-2026 The skiftOS contributors \n");
    stream_format(log_stream, "\n");
}

// Temporary -- pauses boot for a few real seconds so a message printed
// just before this is called actually has time to be read before more
// boot logging scrolls it away (the early console has no scrollback).
// Needs interrupts already enabled (system_get_tick() only advances once
// the PIT's IRQ0 is actually firing) -- that's why this is only safe to
// call after interrupts_initialize(), not before.
static void pause_for_seconds(int seconds)
{
    uint32_t until = system_get_tick() + (seconds * 1000);

    while (system_get_tick() < until)
    {
    }
}

void system_main(Handover *handover)
{
    splash_screen();
    system_initialize();
    memory_initialize(handover);

    scheduler_initialize();
    tasking_initialize();
    interrupts_initialize();

    // Temporary -- verifies the copy-on-write primitive (Physical.h's
    // refcounting + Memory.h's memory_handle_cow_fault()) actually works
    // before anything else depends on it. Needs to run after
    // interrupts_initialize() specifically, so pause_for_seconds() below
    // can rely on system_get_tick() actually advancing. See
    // CowSelfTest.h for why this (and the banner/pause around it) is
    // meant to be deleted later, once Stage 2 has real coverage of the
    // same mechanism through actual usage.
    stream_format(log_stream, "\n\n\e[33;1m==================== COW SELF-TEST ====================\e[0m\n");
    cow_self_test();
    stream_format(log_stream, "\e[33;1m========================================================\e[0m\n\n");
    pause_for_seconds(5);

    filesystem_initialize();
    modules_initialize(handover);
    driver_initialize();
    device_initialize();
    process_info_initialize();
    device_info_initialize();
    devices_filesystem_initialize();
    graphic_initialize(handover);
    userspace_initialize();

    ASSERT_NOT_REACHED();
}
