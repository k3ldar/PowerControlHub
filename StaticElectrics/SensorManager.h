#pragma once

/**
 * @brief Base class for sensor handlers that are managed by SensorManager.
 * 
 * Sensors inherit from this class to implement their own initialization and
 * update logic. The SensorManager calls update() at scheduled intervals based
 * on the time returned by each sensor's update method.
 */
class BaseSensorHandler
{
	friend class SensorManager;
private:
	unsigned long _nextUpdateTime = 0;
protected:
	/**
	 * @brief Initialize the sensor hardware and perform initial setup.
	 * 
	 * Called once during SensorManager initialization.
	 */
	virtual void initialize() = 0;

	/**
	 * @brief Update the sensor reading and return the next scheduled update time.
	 * 
	 * @return The delay in ms when this sensor should be updated again.
	 */
	virtual unsigned long update() = 0;
};

/**
 * @brief Manages multiple sensor handlers with non-blocking, time-based updates.
 * 
 * This class coordinates sensor polling by maintaining individual update schedules
 * for each sensor. Each sensor determines its own update interval by returning
 * the next update time from its update() method.
 * 
 * Sensors are processed in the order they appear in the array. Place higher
 * priority sensors first for predictable execution order.
 */
class SensorManager
{
private:
	BaseSensorHandler** _handlers;
	uint8_t _handlerCount;
public:
	/**
	 * @brief Construct a SensorManager with an array of sensor handlers.
	 * 
	 * @param handler Pointer to array of BaseSensorHandler pointers.
	 * @param handlerCount Number of handlers in the array.
	 */
	SensorManager(BaseSensorHandler** handler, uint8_t handlerCount)
	{
		_handlerCount = handlerCount;
		_handlers = handler;
	};

	~SensorManager()
	{
		_handlers = nullptr;
	}

	/**
	 * @brief Initialize all registered sensor handlers.
	 * 
	 * Calls initialize() on each sensor handler. Should be called once during setup.
	 */
	void setup()
	{
		if (!_handlers || _handlerCount == 0)
			return;

		for (uint8_t i = 0; i < _handlerCount; i++)
		{
			_handlers[i]->initialize();
		}
	};

	/**
	 * @brief Update all sensors that are due for an update.
	 * 
	 * Should be called frequently from the main loop. Only updates sensors
	 * whose scheduled time has been reached or passed.
	 * 
	 * @param currentTime The current time in milliseconds (from millis()).
	 */
	void update(unsigned long currentTime)
	{
		for (uint8_t i = 0; i < _handlerCount; i++)
		{
			BaseSensorHandler* handler = _handlers[i];
			if (currentTime >= handler->_nextUpdateTime)
			{
				handler->_nextUpdateTime = currentTime + handler->update();
			}
		}
	};
};