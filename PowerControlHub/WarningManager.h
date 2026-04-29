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
#include <stdint.h>

#include "Local.h"
#include "SystemDefinitions.h"
#include "WarningType.h"

#include "RgbLedFade.h"
class ToneManager;

/**
 * @class WarningManager
 * @brief Simple, centralized warning management.
 * 
 * This class manages all system warnings.
 * Warnings can be raised/cleared from anywhere in the code.
 * 
 * Features:
 * - Bitmap-based warning tracking using bit flags (supports up to 32 warnings)
 * - Extensible WarningType enum for adding new warnings
 * - Query methods to check active warnings
 * 
 * Usage:
 * @code
 * WarningManager warningMgr;
 * 
 * // Raise warnings from anywhere:
 * warningMgr.raiseWarning(WarningType::HighCompassTemperature);
 * 
 * // Check warnings:
 * if (warningMgr.hasWarnings())
 * {
 *     // Show warning indicator
 * }
 * @endcode
 */
class WarningManager
{
private:
	RgbLedFade* _warningStatus;			    // LED indicator for warnings
	ToneManager* _toneManager;              // Sound alert manager
	uint32_t _localWarnings;                // Bitmap of LOCAL warnings (raised by this device)
	uint32_t _previousWarnings;             // Previous warning state for change detection
	uint64_t _lastTonePlayed;               // Last time bad tone was played

	void updateLedStatus();

public:
	/**
	 * @brief Constructor.
	 * @param warningStatus Pointer to RgbLedFade for warning status LED
	 */
    explicit WarningManager();

    // Set runtime-owned LED indicator (can be attached after construction)
    void setWarningStatus(RgbLedFade* warningStatus);

    // Set runtime-owned ToneManager (can be attached after construction)
    void setToneManager(ToneManager* toneManager);
    /**
     * @brief Update warning state.
     * @param now Current time in milliseconds (SystemFunctions::millis64())
     */
    void update(uint64_t now);

    /**
     * @brief Raise (activate) a warning.
     * @param type The warning type to activate
     */
    void raiseWarning(WarningType type);

    /**
     * @brief Clear (deactivate) a warning.
     * @param type The warning type to deactivate
     */
    void clearWarning(WarningType type);

    /**
     * @brief Clear all warnings.
     */
    void clearAllWarnings();

    /**
     * @brief Check if any warnings are active.
     * @return true if at least one warning is active
     */
    bool hasWarnings() const;

    /**
     * @brief Check if a specific warning is active.
     * @param type The warning type to check
     * @return true if the warning is active
     */
    bool isWarningActive(WarningType type) const;

    /**
     * @brief Get the bitmask of all active warnings.
     * @return uint32_t containing all active warning flags
     */
    uint32_t getActiveWarningsMask() const;
};
