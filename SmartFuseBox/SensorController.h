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
        if (index > _sensorCount)
            return nullptr;

        return _sensorHandlers[index];
    }
};