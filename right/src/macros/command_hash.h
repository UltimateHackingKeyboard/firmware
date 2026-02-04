#ifndef __COMMAND_HASH_H__
#define __COMMAND_HASH_H__

#include <stddef.h>
#include "command_ids.h"

struct command_entry { const char *name; command_id_t id; };

struct command_entry *command_lookup(const char *str, size_t len);

#endif
