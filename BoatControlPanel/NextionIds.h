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
#pragma once

constexpr uint8_t PageSplash = 0;
constexpr uint8_t PageHome = 1;
constexpr uint8_t PageWarning = 2;
constexpr uint8_t PageRelay = 3;
constexpr uint8_t PageSoundSignals = 4;
constexpr uint8_t PageSoundManeuveringSignals = 5;
constexpr uint8_t PageSoundFogSignals = 6;
constexpr uint8_t PageSoundOvertaking = 7;
constexpr uint8_t PageSoundEmergency = 8;
constexpr uint8_t PageSoundOther = 9;
constexpr uint8_t PageSystem = 10;
constexpr uint8_t PageFlags = 11;
constexpr uint8_t PageCardinalMarkers = 12;
constexpr uint8_t PageBuoys = 13;
constexpr uint8_t PageAbout = 14;
constexpr uint8_t PageMoon = 15;
constexpr uint8_t PageMoonPhases = 15;
constexpr uint8_t PageVhfRadio = 16;
constexpr uint8_t PageVhfDistress = 17;
constexpr uint8_t PageVhfChannels = 18;
constexpr uint8_t PageSettingsRelays = 19;
constexpr uint8_t PageSettings = 20;
constexpr uint8_t PageEnvironment = 21;

constexpr uint8_t InvalidButtonIndex = 0xFF;

// Button color constants (Nextion picture IDs)
constexpr uint8_t ImageButtonColorBlue = 2;
constexpr uint8_t ImageButtonColorGreen = 3;
constexpr uint8_t ImageButtonColorGrey = 4;
constexpr uint8_t ImageButtonColorOrange = 5;
constexpr uint8_t ImageButtonColorRed = 6;
constexpr uint8_t ImageButtonColorYellow = 7;
constexpr uint8_t ImageButtonColorOffset = 12; // Offset for large button images

// Image Ids for Nextion
constexpr uint8_t ImageCompass = 8;
constexpr uint8_t ImageSettings = 9;
constexpr uint8_t ImageWarning = 10;
constexpr uint8_t ImageBlank = 11;
constexpr uint8_t ImageBackButton = 12;
constexpr uint8_t ImageNextButton = 13;


constexpr uint8_t ImageMoonPhaseNew = 73;
constexpr uint8_t ImageMoonPhaseWaxingCrescent = 74;
constexpr uint8_t ImageMoonPhaseFirstQuarter = 75;
constexpr uint8_t ImageMoonPhaseWaxingGibbous = 76;
constexpr uint8_t ImageMoonPhaseFull = 77;
constexpr uint8_t ImageMoonPhaseWaningGibbous = 78;
constexpr uint8_t ImageMoonPhaseLastQuarter = 79;
constexpr uint8_t ImageMoonPhaseWaningCrescent = 80;


constexpr uint8_t ImageGreenCircle = 82;
constexpr uint8_t ImageOrangeCircle = 83;
constexpr uint8_t ImageRedCircle = 84;

constexpr uint8_t ImageDownArrow = 85;
constexpr uint8_t ImageInwardArrow = 86;
constexpr uint8_t ImageUpArrow = 87;
constexpr uint8_t ImageQuestionMark = 88;

constexpr uint8_t MoonImages[8] = {
    ImageMoonPhaseNew,
    ImageMoonPhaseWaxingCrescent,
    ImageMoonPhaseFirstQuarter,
    ImageMoonPhaseWaxingGibbous,
    ImageMoonPhaseFull,
    ImageMoonPhaseWaningGibbous,
    ImageMoonPhaseLastQuarter,
    ImageMoonPhaseWaningCrescent
};

// page
constexpr char PageOne[] = "page 1";

