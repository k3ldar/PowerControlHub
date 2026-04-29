/*
 * PowerControlHub
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
#pragma once

#include <stddef.h>

// Convert `input` into phonetic words placed into `outBuf` (null-terminated).
// `outBufSize` is the total size of `outBuf`. `separator` defaults to ", ".
// Returns number of bytes written (excluding terminating NUL).
size_t phoneticize(const char* input, char* outBuf, size_t outBufSize, const char* separator = ", ");