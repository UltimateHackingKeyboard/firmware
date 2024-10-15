#ifndef __CONFIG_PARSER_ERROR_REPORTING_H__
#define __CONFIG_PARSER_ERROR_REPORTING_H__

// Includes:

    #include "basic_types.h"
    #include "parse_config.h"
    #include <stdint.h>
    #include <stdbool.h>

// Typedefs:

// Functions:

    void ConfigParser_Error(config_buffer_t *buffer, const char *fmt, ...);

#endif
