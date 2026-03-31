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

#include "BaseCommandHandler.h"
#include "ConfigManager.h"
#include "SystemDefinitions.h"
#include "SystemFunctions.h"


class BaseConfigCommandHandler : public virtual BaseCommandHandler
{
protected:
	const char* getParamValue(const StringKeyValue params[], uint8_t paramCount, const char* key) const
	{
		for (uint8_t i = 0; i < paramCount; ++i)
		{
			if (strcmp(params[i].key, key) == 0)
				return params[i].value;
		}

		return nullptr;
	}

	uint8_t getParamValueU8t(const StringKeyValue params[], uint8_t paramCount, const char* key) const
	{
		const char* v = getParamValue(params, paramCount, key);

		if (!v)
			return DefaultValue;

		uint8_t out;
		return SystemFunctions::parseUnsigned(v, out) ? out : DefaultValue;
	}

	int8_t getParamValue8t(const StringKeyValue params[], uint8_t paramCount, const char* key) const
	{
		const char* v = getParamValue(params, paramCount, key);

		if (!v)
			return 0;

		int8_t out;
		return SystemFunctions::parseSigned(v, out) ? out : 0;
	}

	int16_t getParamValue16t(const StringKeyValue params[], uint8_t paramCount, const char* key) const
	{
		const char* v = getParamValue(params, paramCount, key);

		if (!v)
			return 0;

		int16_t out;
		return SystemFunctions::parseSigned(v, out) ? out : 0;
	}

	uint16_t getParamValueU16t(const StringKeyValue params[], uint8_t paramCount, const char* key) const
	{
		const char* v = getParamValue(params, paramCount, key);

		if (!v)
			return 0;

		uint16_t out;
		return SystemFunctions::parseUnsigned(v, out) ? out : 0;
	}

	uint32_t getParamValueU32t(const StringKeyValue params[], uint8_t paramCount, const char* key) const
	{
		const char* v = getParamValue(params, paramCount, key);

		if (!v)
			return 0;

		uint32_t out;
		return SystemFunctions::parseUnsigned(v, out) ? out : 0;
	}

	bool getParamValueBool(const StringKeyValue params[], uint8_t paramCount, const char* key) const
	{
		const char* v = getParamValue(params, paramCount, key);
		return v ? SystemFunctions::parseBooleanValue(v) : false;
	}

};