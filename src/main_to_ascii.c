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
#include <dc_posix/dc_unistd.h>
#include <dc_util/bits.h>
#include <dc_util/dump.h>
#include <dc_util/types.h>

#define BUF_SIZE 1024


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

struct application_settings
{
    struct dc_opt_settings opts;
    struct dc_setting_string *parity;
    struct dc_setting_string *prefix;
};

static struct dc_application_settings *create_settings( const struct dc_posix_env *env,
                                                        struct dc_error *err);

static int destroy_settings(const struct dc_posix_env *env,
                            struct dc_error *err,
                            struct dc_application_settings **psettings);
static int run( const struct dc_posix_env *env,
                struct dc_error *err,
                struct dc_application_settings *settings);
static void error_reporter(const struct dc_error *err);
static void trace_reporter(const struct dc_posix_env *env,
                          const char *file_name,
                          const char *function_name,
                          size_t line_number);
char decodeCodeWord(const struct dc_posix_env *env, struct dc_error *err, uint16_t *codeWord, bool isEvenParity);



int main(int argc, char * argv[]) {
    dc_posix_tracer tracer;
    dc_error_reporter reporter;
    struct dc_posix_env env;
    struct dc_error err;
    struct dc_application_info *info;
    int ret_val;

    reporter = error_reporter;
    tracer = NULL;
    // tracer = trace_reporter;
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
    static bool default_verbose = false;
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
                    "even"},
            {(struct dc_setting *)settings->prefix,
                    dc_options_set_string,
                    "prefix",
                    required_argument,
                    'r',
                    "PREFIX",
                    dc_string_from_string,
                    "prefix",
                    dc_string_from_config,
                    "abc"}
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
    dc_setting_string_destroy(env, &app_settings->parity);
    dc_setting_string_destroy(env, &app_settings->prefix);
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

    // Create settings
    app_settings = (struct application_settings *)settings;
    // Get settings
    parity = dc_setting_string_get(env, app_settings->parity);
    prefix = dc_setting_string_get(env, app_settings->prefix);
    isEvenParity = isEvenParitySetting(parity);
   
    if (dc_error_has_error(err)) {
        display("setup error");
        error_reporter(err);
    }
    
    pathArr = constructFilePathArray(env, err, prefix);
    codeWords = (uint16_t*)calloc(8, sizeof(uint16_t));

    for (size_t i = 0; i < 12; i++) {
        fd = dc_open(env, err, (pathArr+(i*BUF_SIZE)), DC_O_RDONLY, DC_S_IRUSR);

        // TODO: be able to handle more than 1 byte
        if (dc_error_has_no_error(err)) {
            char* chars = calloc(BUF_SIZE, 1024);
            while((nread = dc_read(env, err, fd, chars, BUF_SIZE)) > 0) {
                for (size_t j = 0; j < 8; j++) {
                    if (get_mask(*chars, masks_16[j])) {
                        *(codeWords+j) = set_bit(*(codeWords+j), masks_16[i]);
                    
                    }
                }


                // dc_write(env, err, STDOUT_FILENO, chars, (size_t)nread);

            }
            free(chars);
        }
        else {error_reporter(err);}
        
    }
    // dc_write(env, err, STDOUT_FILENO, codeWords, 8*sizeof(uint16_t));
    
    // decode codeWords
    char decoded[8] = ""; 
    for (size_t i = 0; i < 8; i++) {
        char c = decodeCodeWord(env, err, codeWords+i, isEvenParity);
        decoded[i] = c;
    }

    dc_write(env, err, STDOUT_FILENO, decoded, 8);

    free(codeWords);
    return ret_val;
}

char decodeCodeWord(const struct dc_posix_env *env, struct dc_error *err, uint16_t *codeWord, bool isEvenParity) {
    char c = '\0';
    size_t parityCount;
    size_t j;
    size_t k;

    // TODO: parity check and error correction
    for (size_t i = pow(2,0); i < pow(2,4); i <<= 1) {
        parityCount = 0;
        j = i;
        while ( j < pow(2,4)) {
            k = 0;
            while ( k < i ) {
                if ( get_mask(*codeWord, masks_16[j++])) {
                    parityCount++;
                }
                k++;
            }
            j += i;
        }

        if ( (!isEven(parityCount) && isEvenParity ) || ( isEven(parityCount) && !isEvenParity) ) {
            printf("error detected\n");
        }
    }


    // Break apart codeWord into data word
    size_t l = 0;
    for (size_t i = 0; i < 12; ++i) {
        if (!powerOfTwo(i)) {
            if( get_mask(*codeWord, masks_16[i]) ) {
                c = set_bit(c, masks_16[l]);
            }
            l++;
        } else {
            while(powerOfTwo(i)) {
                i++;
            }
            if (get_mask(*codeWord, masks_16[i])) {
                c = set_bit(c, masks_16[l]);
            }
            l++;
        }
        
    }
    return c;
}

static void error_reporter(const struct dc_error *err) {
    fprintf(stderr, "ERROR: %s : %s : @ %zu : %d\n", err->file_name, err->function_name, err->line_number, 0);
    fprintf(stderr, "ERROR: %s\n", err->message);
}

static void trace_reporter(__attribute__((unused))  const struct dc_posix_env *env,
                                                    const char *file_name,
                                                    const char *function_name,
                                                    size_t line_number) {
    fprintf(stdout, "TRACE: %s : %s : @ %zu\n", file_name, function_name, line_number);
}
