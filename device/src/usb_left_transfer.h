#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    const void *session;
    const uint8_t *data;
    size_t size;
    uint64_t token;
    uint32_t sequence, generation;
    bool pending, cancelling, relay, neutral, usb, cancelQueued, queued, canonical;
    uint32_t route;
} relay_transfer_ticket_t;

typedef enum {
    RelayTransfer_Ignored,
    RelayTransfer_Failed,
    RelayTransfer_Success
} relay_transfer_result_t;

/* A worker must retain its queued reservation until it replaces it with the
 * actual endpoint transfer. Otherwise the route coordinator could observe a
 * false drain between encoding and submission. Called with the lock held. */
static inline bool RelayTransfer_IsReservation(
    const relay_transfer_ticket_t *ticket, const relay_transfer_ticket_t *reservation)
{
    return reservation && ticket->pending && ticket->queued && ticket->data == reservation->data &&
           ticket->size == reservation->size && ticket->generation == reservation->generation &&
           ticket->route == reservation->route && ticket->token == reservation->token &&
           ticket->sequence == reservation->sequence;
}

/* Called with the adapter's lock held. A zero-length callback is an error or
 * cancellation, never successful delivery. Retire only the matching transfer. */
static inline relay_transfer_result_t RelayTransfer_Complete(relay_transfer_ticket_t *ticket,
    const void *session, const void *data, size_t size, uint32_t generation,
    relay_transfer_ticket_t *completed)
{
    if (!ticket->pending || ticket->session != session || (size && ticket->data != data)) {
        return RelayTransfer_Ignored;
    }
    *completed = *ticket;
    ticket->pending = false;
    return !completed->cancelling && size == completed->size && size &&
                   (!completed->usb || completed->generation == generation)
               ? RelayTransfer_Success
               : RelayTransfer_Failed;
}
