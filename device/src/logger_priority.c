#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "logger_priority.h"
#include <zephyr/kernel.h>
#include "zephyr/device.h"
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/arch/arch_interface.h>

// Structure to pass data to the callback
struct thread_search_data {
    const char *target_name;
    k_tid_t found_thread;
};

// Callback function for k_thread_foreach
static void thread_search_cb(const struct k_thread *thread, void *user_data) {
    struct thread_search_data *search_data = (struct thread_search_data *)user_data;
    const char *thread_name = k_thread_name_get((k_tid_t)thread);

    if (thread_name && strcmp(thread_name, search_data->target_name) == 0) {
        search_data->found_thread = (k_tid_t)thread;
    }
}

// Your custom getThreadByName function
k_tid_t getThreadByName(const char *name) {
    struct thread_search_data search_data = {
        .target_name = name,
        .found_thread = NULL
    };

    k_thread_foreach(thread_search_cb, &search_data);

    return search_data.found_thread;
}

// Function to find thread by name and set priority
int set_thread_priority_by_name(const char *thread_name, int new_priority) {
    k_tid_t thread_id;

    // Iterate through all threads to find the one with matching name
    thread_id = getThreadByName(thread_name);
    if (thread_id != NULL) {
        k_thread_priority_set(thread_id, new_priority);
        return 0;
    }
    return -ENOENT; // Thread not found
}

// The logging thread has to stay *strictly* above the shell threads that feed it.
// If they share a priority, a shell command can emit a whole burst of printks (e.g.
// `uhk connections`) without the logging thread ever getting scheduled; the deferred
// log buffer then overflows and its oldest messages are overwritten, so the dump
// comes out truncated at the beginning.
#define SHELL_THREAD_PRIORITY K_PRIO_PREEMPT(K_LOWEST_APPLICATION_THREAD_PRIO)

// High: cooperative, so that logging preempts even the main thread (used during boot).
// Low: preemptible, just above the shell threads, so that it stays out of the way of
// the event loop while still being able to drain shell output as it is produced.
#define LOG_THREAD_PRIORITY_HIGH K_PRIO_COOP(CONFIG_NUM_COOP_PRIORITIES - 1)
#define LOG_THREAD_PRIORITY_LOW K_PRIO_PREEMPT(K_LOWEST_APPLICATION_THREAD_PRIO - 1)

void Logger_SetPriority(bool high) {
    set_thread_priority_by_name("logging", high ? LOG_THREAD_PRIORITY_HIGH : LOG_THREAD_PRIORITY_LOW);
    set_thread_priority_by_name("UhkShell", SHELL_THREAD_PRIORITY);
    set_thread_priority_by_name("shell_rtt", SHELL_THREAD_PRIORITY);
}

