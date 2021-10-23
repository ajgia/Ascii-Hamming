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
#include <stdlib.h>
#include "common.h"
#include <stdio.h>
#include <dc_posix/dc_unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define BUF_SIZE 1024

int main(void)
{
    

    struct dc_error     err;
    struct dc_posix_env env;
    char                chars[BUF_SIZE];
    ssize_t             nread;
    int                 ret_val;

    dc_error_init(&err, NULL);
    dc_posix_env_init(&env, NULL);
    ret_val = EXIT_SUCCESS;

    display("To ASCII");

    while((nread = dc_read(&env, &err, STDIN_FILENO, chars, BUF_SIZE)) > 0)
    {
        if(dc_error_has_error(&err))
        {
            ret_val = 1;
        }

        dc_write(&env, &err, STDOUT_FILENO, chars, (size_t)nread);

        if(dc_error_has_error(&err))
        {
            ret_val = 2;
        }
    }

    return ret_val;
}
