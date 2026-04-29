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
#include <Arduino.h>

// Firmware version constants.
// On release builds these values are overwritten by the CI workflow from the
// git tag (v<major>.<minor>.<patch>.<build>) — do not edit the four lines below.
// For local/development builds these fallback values are used instead.
constexpr uint8_t FirmwareMajor = 0;
constexpr uint8_t FirmwareMinor = 9;
constexpr uint8_t FirmwarePatch = 0;
constexpr uint8_t FirmwareBuild = 3;
