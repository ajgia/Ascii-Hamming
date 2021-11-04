/*
 * This file is part of dc_dump.
 *
 *  dc_dump is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Foobar is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with dc_dump.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <math.h>
#include <dc_posix/dc_unistd.h>
#include <dc_application/command_line.h>
#include <dc_application/config.h>
#include <dc_application/defaults.h>
#include <dc_application/environment.h>
#include <dc_application/options.h>
#include <dc_posix/dc_stdlib.h>
#include <dc_posix/dc_string.h>
#include <dc_posix/dc_fcntl.h>
#include <dc_util/bits.h>
#include <dc_util/dump.h>
#include <dc_util/types.h>

#define BUF_SIZE 1024


/**
 * Bit masks
 */ 
const uint16_t MASK_00000000_00000001 = UINT16_C(0x00000001);
const uint16_t MASK_00000000_00000010 = UINT16_C(0x00000002);
const uint16_t MASK_00000000_00000100 = UINT16_C(0x00000004);
const uint16_t MASK_00000000_00001000 = UINT16_C(0x00000008);
const uint16_t MASK_00000000_00010000 = UINT16_C(0x00000010);
const uint16_t MASK_00000000_00100000 = UINT16_C(0x00000020);
const uint16_t MASK_00000000_01000000 = UINT16_C(0x00000040);
const uint16_t MASK_00000000_10000000 = UINT16_C(0x00000080);
const uint16_t MASK_00000001_00000000 = UINT16_C(0x00000100);
const uint16_t MASK_00000010_00000000 = UINT16_C(0x00000200);
const uint16_t MASK_00000100_00000000 = UINT16_C(0x00000400);
const uint16_t MASK_00001000_00000000 = UINT16_C(0x00000800);
const uint16_t MASK_00010000_00000000 = UINT16_C(0x00001000);
const uint16_t MASK_00100000_00000000 = UINT16_C(0x00002000);
const uint16_t MASK_01000000_00000000 = UINT16_C(0x00004000);
const uint16_t MASK_10000000_00000000 = UINT16_C(0x00008000);

/**
 * Bit mask array
 */ 
static const uint16_t masks_16[] = {
    MASK_00000000_00000001,
    MASK_00000000_00000010,
    MASK_00000000_00000100, 
    MASK_00000000_00001000,
    MASK_00000000_00010000,
    MASK_00000000_00100000,
    MASK_00000000_01000000,
    MASK_00000000_10000000,
    MASK_00000001_00000000,
    MASK_00000010_00000000,
    MASK_00000100_00000000,
    MASK_00001000_00000000,
    MASK_00010000_00000000,
    MASK_00100000_00000000,
    MASK_01000000_00000000,
    MASK_10000000_00000000
};

/**
 * DC Application Settings
 * Has default options, plus settings for bit parity and file prefix.
 */ 
struct application_settings
{
    struct dc_opt_settings opts;
    struct dc_setting_string *parity;
    struct dc_setting_string *prefix;
};

// Defining this here. I don't know why it's not working through the includes
struct dc_setting_string
{
    struct dc_setting parent;
    char *string;
};

/**
 * Create DC Application settings
 */ 
static struct dc_application_settings *create_settings( const struct dc_posix_env *env,
                                                        struct dc_error *err);

/**
 * Destroy DC Application settings
 */ 
static int destroy_settings(const struct dc_posix_env *env,
                            struct dc_error *err,
                            struct dc_application_settings **psettings);
/**
 * Error reporter
 */ 
static void error_reporter(const struct dc_error *err);
/**
 * Trace reporter
 */ 
static void trace_reporter( const struct dc_posix_env *env,
                            const char *file_name,
                            const char *function_name,
                            size_t line_number);

/**
 * Translate Hamming to ASCII
 */ 
static int run( const struct dc_posix_env *env,
                struct dc_error *err,
                struct dc_application_settings *settings);
/**
 * Decode a Hamming Codeword and return the translated character.
 * @param env dc_env
 * @param er dc_err
 * @param codeWord the codeword to decode
 * @param isEvenParity the parity to use to decode the codeword
 * @return the decoded character
 */ 
unsigned char decodeCodeWord(uint16_t *codeWord, bool isEvenParity);


/**
 * Main
 */ 
int main(int argc, char * argv[]) {
    dc_posix_tracer tracer;
    dc_error_reporter reporter;
    struct dc_posix_env env;
    struct dc_error err;
    struct dc_application_info *info;
    int ret_val;

    reporter = error_reporter;
    tracer = trace_reporter;
    tracer = NULL;
    dc_error_init(&err, reporter);
    dc_posix_env_init(&env, tracer);
    info = dc_application_info_create(&env, &err, "To ASCII Application");
    ret_val = dc_application_run(&env, &err, info, create_settings, destroy_settings, run, dc_default_create_lifecycle, dc_default_destroy_lifecycle, NULL, argc, argv);
    dc_application_info_destroy(&env, &info);
    dc_error_reset(&err);
    return ret_val;
}

static struct dc_application_settings *create_settings(const struct dc_posix_env *env, struct dc_error *err)
{
    struct application_settings *settings;

    DC_TRACE(env);
    settings = dc_malloc(env, err, sizeof(struct application_settings));

    if(settings == NULL)
    {
        return NULL;
    }

    settings->opts.parent.config_path = dc_setting_path_create(env, err);
    settings->parity = dc_setting_string_create(env, err);
    settings->prefix = dc_setting_string_create(env, err);

    struct options opts[] = {
            {(struct dc_setting *)settings->opts.parent.config_path,
                    dc_options_set_path,
                    "config",
                    required_argument,
                    'c',
                    "CONFIG",
                    dc_string_from_string,
                    NULL,
                    dc_string_from_config,
                    NULL},
            {(struct dc_setting *)settings->parity,
                    dc_options_set_string,
                    "parity",
                    required_argument,
                    'p',
                    "PARITY",
                    dc_string_from_string,
                    "parity",
                    dc_string_from_config,
                    NULL},
            {(struct dc_setting *)settings->prefix,
                    dc_options_set_string,
                    "prefix",
                    required_argument,
                    'f',
                    "PREFIX",
                    dc_string_from_string,
                    "prefix",
                    dc_string_from_config,
                    NULL}
    };

    // note the trick here - we use calloc and add 1 to ensure the last line is all 0/NULL
    settings->opts.opts_count = (sizeof(opts) / sizeof(struct options)) + 1;
    settings->opts.opts_size = sizeof(struct options);
    settings->opts.opts = dc_calloc(env, err, settings->opts.opts_count, settings->opts.opts_size);
    dc_memcpy(env, settings->opts.opts, opts, sizeof(opts));
    settings->opts.flags = "m:";
    settings->opts.env_prefix = "DC_EXAMPLE_";

    return (struct dc_application_settings *)settings;
}

static int destroy_settings(const struct dc_posix_env *env,
                            __attribute__((unused)) struct dc_error *err,
                            struct dc_application_settings **psettings) 
{
    struct application_settings *app_settings;

    DC_TRACE(env);
    app_settings = (struct application_settings *)*psettings;
    if ( app_settings->parity->string != NULL)
        dc_setting_string_destroy(env, &(app_settings->parity));
    if ( app_settings->prefix->string != NULL)
        dc_setting_string_destroy(env, &(app_settings->prefix));
    dc_free(env, app_settings->opts.opts, app_settings->opts.opts_count);
    dc_free(env, *psettings, sizeof(struct application_settings));

    if(env->null_free)
    {
        *psettings = NULL;
    }

    return 0;
}


static int run(const struct dc_posix_env *env, struct dc_error *err, struct dc_application_settings *settings) {
    struct application_settings *app_settings;
    const char *parity;
    const char *prefix;
    char * pathArr;
    uint16_t * codeWords;
    
    ssize_t nread;
    int ret_val;
    bool isEvenParity;
    int fd;
    
    DC_TRACE(env);
    ret_val = 0;

    // Create and get settings
    app_settings = (struct application_settings *)settings;
    parity = dc_setting_string_get(env, app_settings->parity);
    prefix = dc_setting_string_get(env, app_settings->prefix);

    if (prefix == NULL || parity == NULL) {
        display("Error: prefix and parity arguments are required. Exiting.");
        return EXIT_FAILURE;
    }

    int paritySetting = isEvenParitySetting(parity);
    if ( paritySetting != 0 && paritySetting != 1 ) {
        return EXIT_FAILURE;
    }
    else 
        isEvenParity = (bool)paritySetting;
   
    // File path array
    pathArr = constructFilePathArray(prefix);
    // Holds each decoded code word
    codeWords = (uint16_t*)calloc(BUF_SIZE, sizeof(uint16_t));

    size_t maxRead = 0;
    // Loop over input files
    for (size_t i = 0; i < 12; i++) {
        // fd = dc_open(env, err, (pathArr+(i*BUF_SIZE)), DC_O_RDONLY, DC_S_IRUSR);
        fd = open((pathArr+(i*BUF_SIZE)), DC_O_RDONLY, DC_S_IRUSR);

        // Catch bad open
        if (fd == -1) {
            return -1;
        }

        if (dc_error_has_no_error(err)) {
            unsigned char* chars = calloc(BUF_SIZE, 1024);
            while((nread = dc_read(env, err, fd, chars, BUF_SIZE)) > 0) {

                // loop through bytes
                for (size_t k = 0; k < (size_t)nread; k++) {
                    
                    // loop through bits in byte
                    for (size_t l = 0; l < 8; l++) {
                        if (get_mask8(*(chars+k), masks_16[l])) {
                            *(codeWords+l+8*k) = set_bit(*(codeWords+l+8*k), masks_16[i]);
                    
                        }
                    }

                }


                // dc_write(env, err, STDOUT_FILENO, chars, (size_t)nread);


                // store nread
                if (maxRead < (size_t)nread)
                    maxRead = (size_t)nread;
            }
            free(chars);
        }
        else { 
            error_reporter(err); 
        }

        close(fd);
        // dc_close(env, err, fd);
    }
    
    size_t numCodeWords = maxRead *8;
    // dc_write(env, err, STDOUT_FILENO, codeWords, numCodeWords * sizeof(uint16_t));
    
    // decode codeWords
    // char decoded[ 8] = "";
    size_t nonNulls = 0;

    unsigned char *decoded = (unsigned char*)calloc(numCodeWords, sizeof(char));

    for (size_t i = 0; i < numCodeWords; i++) {
        unsigned char c = decodeCodeWord(codeWords+i, isEvenParity);
        if (c != '\0') {
            decoded[nonNulls] = c;
            ++nonNulls;
        }
        
    }

    dc_write(env, err, STDOUT_FILENO, decoded, nonNulls);

    free(codeWords);
    free(decoded);
    return ret_val;
}

unsigned char decodeCodeWord(uint16_t *codeWord, bool isEvenParity) {
    unsigned char c = '\0';
    size_t parityCount;
    size_t j;
    size_t k;
    uint8_t *errorLocation = (uint8_t*)calloc(1, sizeof(uint8_t));
    // dc_write(env, err, STDOUT_FILENO, errorLocation, 1);

    for (size_t i = 1; i < 16; i <<= 1) {
        parityCount = 0;
        j = i;
        while ( j < pow(2,4)) {
            k = 0;
            while ( k < i ) {
                if ( get_mask(*codeWord, masks_16[j-1])) {
                    parityCount++;
                }
                ++j;
                ++k;
            }
            j += i;
        }

        if ( (!isEven(parityCount) && isEvenParity ) || ( isEven(parityCount) && !isEvenParity) ) {
            // we have an error in this parity check
            uint8_t cBit = (uint8_t)(log(i)/log(2));
            *errorLocation = set_bit8(*errorLocation, masks_16[cBit]);
        }
    }

    // dc_write(env, err, STDOUT_FILENO, "e", 1);
    // dc_write(env, err, STDOUT_FILENO, errorLocation, 1);
    if (*errorLocation) {
        // toggle error-location'th bit
        *codeWord ^= masks_16[*errorLocation - 1];
    }
    
    // Break apart codeWord into data word
    size_t l = 0;
    for (size_t i = 1; i <= 12; ++i) {
        if (!powerOfTwo(i)) {
            if( get_mask(*codeWord, masks_16[i-1]) ) {
                c = set_bit8(c, masks_16[l]);
            }
            l++;
        } else {
            while(powerOfTwo(i)) {
                i++;
            }
            if (get_mask(*codeWord, masks_16[i-1])) {
                c = set_bit8(c, masks_16[l]);
            }
            l++;
        }
        
    }

    free(errorLocation);
    return c;
}

static void error_reporter(const struct dc_error *err) {
    fprintf(stderr, "ERROR: %s : %s : @ %zu : %d\n", err->file_name, err->function_name, err->line_number, 0);
    fprintf(stderr, "ERROR: %s\n", err->message);
}

static void trace_reporter( const struct dc_posix_env *env,
                            const char *file_name,
                            const char *function_name,
                            size_t line_number) {
    fprintf(stdout, "TRACE: %s : %s : @ %zu\n", file_name, function_name, line_number);

}

