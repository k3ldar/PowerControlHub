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

// page
constexpr char PageOne[] = "page 1";

