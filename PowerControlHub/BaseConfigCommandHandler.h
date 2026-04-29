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

#include "BaseCommandHandler.h"
#include "ConfigManager.h"
#include "SystemDefinitions.h"
#include "SystemFunctions.h"


class BaseConfigCommandHandler
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

	bool getParamValueU8t(const StringKeyValue params[], uint8_t paramCount, const char* key, uint8_t& out) const
	{
		const char* v = getParamValue(params, paramCount, key);

		if (!v)
			return false;

		return SystemFunctions::parseUnsigned(v, out);
	}

	bool getParamValue8t(const StringKeyValue params[], uint8_t paramCount, const char* key, int8_t& out) const
	{
		const char* v = getParamValue(params, paramCount, key);

		if (!v)
			return false;

		return SystemFunctions::parseSigned(v, out);
	}

	bool getParamValue16t(const StringKeyValue params[], uint8_t paramCount, const char* key, int16_t& out) const
	{
		const char* v = getParamValue(params, paramCount, key);

		if (!v)
			return false;

		return SystemFunctions::parseSigned(v, out);
	}

	bool getParamValueU16t(const StringKeyValue params[], uint8_t paramCount, const char* key, uint16_t& out) const
	{
		const char* v = getParamValue(params, paramCount, key);

		if (!v)
			return false;

		return SystemFunctions::parseUnsigned(v, out);
	}

	bool getParamValueU32t(const StringKeyValue params[], uint8_t paramCount, const char* key, uint32_t& out) const
	{
		const char* v = getParamValue(params, paramCount, key);

		if (!v)
			return false;

		return SystemFunctions::parseUnsigned(v, out);
	}

	bool getParamValueBool(const StringKeyValue params[], uint8_t paramCount, const char* key, bool& out) const
	{
		const char* v = getParamValue(params, paramCount, key);

		if (!v)
			return false;

		out = SystemFunctions::parseBooleanValue(v);
		return true;
	}
};