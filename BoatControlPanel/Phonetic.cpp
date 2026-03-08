/*
 * SmartFuseBox
 * Copyright (C) 2025 Simon Carter (s1cart3r@gmail.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "Phonetic.h"

#include <avr/pgmspace.h>
#include <ctype.h>
#include <string.h>

// NATO phonetic A-Z then 0-9 stored in PROGMEM using a fixed-width 2D array.
static const char phoneticTable[][10] PROGMEM = {
    "Alpha",  "Bravo",  "Charlie", "Delta",   "Echo",    "Foxtrot", "Golf",   "Hotel",
    "India",  "Juliet", "Kilo",    "Lima",    "Mike",    "November","Oscar",  "Papa",
    "Quebec", "Romeo",  "Sierra",  "Tango",   "Uniform", "Victor",  "Whiskey","Xray",
    "Yankee", "Zulu",
    "Zero", "One", "Two", "Three", "Four", "Five", "Six", "Seven", "Eight", "Nine"
};

static bool copy_phonetic_from_table(uint8_t idx, char* dest, size_t destSize)
{
    if (destSize == 0) return false;
    strncpy_P(dest, phoneticTable[idx], destSize - 1);
    dest[destSize - 1] = '\0';
    return true;
}

size_t phoneticize(const char* input, char* outBuf, size_t outBufSize, const char* separator)
{
    if (!input || !outBuf || outBufSize == 0 || !separator)
        return 0;

    outBuf[0] = '\0';
    size_t outLen = 0;
    bool first = true;
    char tmp[16]; // temporary buffer for a single phonetic word

    for (const char* p = input; *p != '\0'; ++p)
    {
        char c = *p;
        // skip control chars except space, dash and dot which we'll spell out
        if ((unsigned char)c < 32) continue;

        // Map to phonetic
        if (c >= 'a' && c <= 'z') c = (char)toupper((unsigned char)c);

        bool found = false;
        if (c >= 'A' && c <= 'Z')
        {
            uint8_t idx = (uint8_t)(c - 'A');
            copy_phonetic_from_table(idx, tmp, sizeof(tmp));
            found = true;
        }
        else if (c >= '0' && c <= '9')
        {
            uint8_t idx = 26 + (uint8_t)(c - '0'); // digits start at 26
            copy_phonetic_from_table(idx, tmp, sizeof(tmp));
            found = true;
        }
        else
        {
            // Some common punctuation
            if (c == '-')
            {
                strncpy_P(tmp, PSTR("Dash"), sizeof(tmp) - 1);
                tmp[sizeof(tmp) - 1] = '\0';
                found = true;
            }
            else if (c == '.')
            {
                strncpy_P(tmp, PSTR("Dot"), sizeof(tmp) - 1);
                tmp[sizeof(tmp) - 1] = '\0';
                found = true;
            }
            else if (c == ' ')
            {
                strncpy_P(tmp, PSTR("Space"), sizeof(tmp) - 1);
                tmp[sizeof(tmp) - 1] = '\0';
                found = true;
            }
        }

        if (!found)
            continue; // skip characters we don't handle

        size_t wordLen = strlen(tmp);
        size_t sepLen = first ? 0 : strlen(separator);

        if (outLen + sepLen + wordLen + 1 > outBufSize)
            break; // not enough space, stop safely

        if (!first)
        {
            memcpy(outBuf + outLen, separator, sepLen);
            outLen += sepLen;
        }

        memcpy(outBuf + outLen, tmp, wordLen);
        outLen += wordLen;
        outBuf[outLen] = '\0';
        first = false;
    }

    return outLen;
}