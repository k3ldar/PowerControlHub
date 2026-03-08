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

#include "BaseSensor.h"

class SensorController
{
private:
    BaseSensor** _sensorHandlers;
    uint8_t _sensorCount;
public:
    explicit SensorController(BaseSensor** sensorHandlers, uint8_t sensorCount)
        : _sensorHandlers(sensorHandlers), _sensorCount(sensorCount)
    {

    }

    uint8_t sensorCount() { return _sensorCount; }

    BaseSensor* sensorGet(uint8_t index)
    {
        if (index >= _sensorCount)
            return nullptr;

        return _sensorHandlers[index];
    }
};