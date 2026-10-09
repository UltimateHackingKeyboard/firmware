#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* A source identity, rather than a HID usage: two keys can emit the same usage. */
static inline void HostInputGate_Capture(uint8_t *bits, size_t source, bool held)
{
    if (held) {
        bits[source / 8] |= (uint8_t)(1u << (source % 8));
    }
}
static inline bool HostInputGate_Suppressed(uint8_t *bits, size_t source, bool released)
{
    uint8_t mask = (uint8_t)(1u << (source % 8));
    if (released) {
        bits[source / 8] &= (uint8_t)~mask;
    }
    return (bits[source / 8] & mask) != 0;
}
