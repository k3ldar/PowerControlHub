#pragma once
#include <Arduino.h>
#include <stdint.h>

#include "SharedConstants.h"

enum class VesselType : uint8_t {
    Motor = 0x00,                   // Power boat
    Sail = 0x01,                    // Sailing boat
    Fishing = 0x02,                 // Fishing boat
    Yacht = 0x03,                   // Yacht
};

//
// Layout:
// - version (uint8_t)
// - boatName (30 chars + null)
// - relayShortNames[8][6] (5 chars + null each)
// - relayLongNames[8][21] (20 chars + null each)
// - homePageMapping[4] (values 0..7 or 0xFF for empty)
// - homePageButtonImage[4] (button color image IDs)
// - vesselType (VesselType)
// - hornRelayIndex (uint8_t) 0..7 or 0xFF = none
// - checksum (uint16_t)
//
// Keep struct packed and stable. Increase 'VERSION' when you change layout.
// Packed POD for persistent configuration.
constexpr uint8_t ConfigVersion = 3;
constexpr uint8_t ConfigRelayCount = 8;
constexpr uint8_t ConfigHomeButtons = 4;
constexpr uint8_t ConfigMaxBoatNameLength = 31; // max characters (inc null)
constexpr uint8_t ConfigShortRelayName = 6; // max characters (inc null) - for home page
constexpr uint8_t ConfigLongRelayName = 21; // max characters (inc null) - for buttons page

struct Config {
    uint8_t version;
    char boatName[ConfigMaxBoatNameLength];
    char relayShortNames[ConfigRelayCount][ConfigShortRelayName];
    char relayLongNames[ConfigRelayCount][ConfigLongRelayName];
    uint8_t homePageMapping[ConfigHomeButtons]; // 0..7 or 0xFF = empty
    uint8_t buttonImage[ConfigRelayCount]; // 0..7 or 0xFF = empty
    VesselType vesselType;
	uint8_t hornRelayIndex; // 0..7 or 0xFF = none
	uint16_t soundStartDelayMs; // 500ms default
    uint16_t checksum;
} __attribute__((packed));