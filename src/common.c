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
#include <stdio.h>
#include <string.h>
#include <dc_posix/dc_unistd.h>
#include <ctype.h>

#define BUF_SIZE 1024

int display(const char *str)
{
    printf("%s\n", str);
    return 0;
}

uint16_t set_bit(uint16_t byte, uint16_t mask) {
    uint16_t set;
    set = byte | mask;
    return set;
}

uint8_t set_bit8(uint8_t byte, uint8_t mask) {
    uint8_t set;
    set = byte | mask;
    return set;
}

void print_mask(uint16_t byte, uint16_t mask) {
    uint16_t masked;
    masked = byte & mask;
    printf("%u\n", masked);
}

uint16_t get_mask(uint16_t byte, uint16_t  mask) {
    uint16_t masked;
    masked = byte & mask;
    return masked;
}

uint8_t get_mask8(uint8_t byte, uint16_t mask) {
    uint8_t masked;
    masked = byte & mask;
    return masked;
}


size_t powerOfTwo(size_t x)
{
   //checks whether a number is zero or not
   if (x == 0)
      return 0;

   //true till x is not equal to 1
   while( x != 1)
   {
      //checks whether a number is divisible by 2
      if(x % 2 != 0)
         return 0;
         x /= 2;
   }
   return 1;
}

bool isEven(size_t x) {
    return (x % 2 == 0);
}

// Return 1 for even, 0 for odd, 2 for failure
int isEvenParitySetting(const char * parity) {
    char arg [strlen(parity) + 1];
    // Get lowercase
    // char *arg = parity;
    strcpy(arg, parity);
    for (size_t i = 0; *(arg+i); ++i) {
        *(arg+i) = (char)tolower(*(arg+i));
    }

    if (strcmp(arg, "odd") == 0)
        return 0;
    else if (strcmp(arg, "even") == 0)
        return 1;
    // Default case, display warning
    else {
        display("Error: please enter only \"even\" or \"odd\" for the parity argument. Exiting.");
        return 2;
    }
}

char* constructFilePathArray(const char * prefix) {
    char pathBeginning[BUF_SIZE] = "";
    const char rel[] = "./";
    const char ext[] = ".hamming";
    const char sep[] = "-";

    char* pathArr = (char*)calloc(12*BUF_SIZE, sizeof(char));

    // Make common filepath start
    strncat(pathBeginning, rel, strlen(rel));
    strncat(pathBeginning, prefix, strlen(prefix));
    strncat(pathBeginning, sep, strlen(sep));

    // Make an array of strings containing the filepaths 1 through 12
    // Format: {prefix}-0.hamming, {prefix}-1.hamming , ...., {prefix}-11.hamming 
    for (size_t i = 0; i < 12 ; i++) {
        char path[BUF_SIZE] = "";

        strncpy(path, pathBeginning, strlen(pathBeginning));
        
        // Number append
        char buffer[10];
        snprintf(buffer, 10, "%zu", i);
        strncat(path, buffer, strlen(buffer));

        strncat(path, ext, strlen(ext));
        // printf("path: %s\n", path);
        strncpy((pathArr + (BUF_SIZE*i)), path, strlen(path));
        // printf("path: %s\n", (pathArr + (BUF_SIZE*i)));
    }

    return pathArr;
}

void destroyArray(char* arr) {
    free(arr);
}
