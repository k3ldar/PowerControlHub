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

constexpr char comfortColdDamp[] PROGMEM = "Cold & Damp";
constexpr char comfortCold[] PROGMEM = "Cold";
constexpr char comfortComfortable[] PROGMEM = "Comfortable";
constexpr char comfortWarm[] PROGMEM = "Warm";
constexpr char comfortHotHumid[] PROGMEM = "Hot & Humid";
constexpr char comfortHumid[] PROGMEM = "Humid";
constexpr char comfortDry[] PROGMEM = "Dry";
constexpr char comfortFair[] PROGMEM = "Fair";

constexpr char condensationLow[] PROGMEM = "Low";
constexpr char condensationWatch[] PROGMEM = "Watch";
constexpr char condensationHigh[] PROGMEM = "High";

enum class CondensationRisk
{ 
    Low, 
    Watch,
    High
};


class Environment
{
public:
    static double dewPoint(double tempC, double relativeHumidity)
    {
        const double a = 17.62;
        const double b = 243.12; // °C
        double gamma = (a * tempC) / (b + tempC) + log(relativeHumidity / 100.0);
        return (b * gamma) / (a - gamma);
    };

    static double estimatedSurfaceTemp(double airTemp, bool night)
    {
        return night ? airTemp - 2.0 : airTemp - 0.5;
    };

    static CondensationRisk condensationRisk(
        double airTemp,
        double dewPt,
        bool night)
    {
        double surfaceTemp = estimatedSurfaceTemp(airTemp, night);
        double delta = surfaceTemp - dewPt;

        if (delta <= 0.5) return CondensationRisk::High;
        if (delta <= 2.0) return CondensationRisk::Watch;
        return CondensationRisk::Low;
    }

    /**
     * @brief Return a PROGMEM comfort description string based on temperature, humidity and dew point.
     *
     * @param tempC    Air temperature in degrees Celsius
     * @param humidity Relative humidity (0-100 %)
     * @param dewPoint Pre-calculated dew point in degrees Celsius
     * @return PROGMEM string pointer — copy with strncpy_P before use
     */
    static const char* getComfortDescription(double tempC, double humidity, double dewPoint)
    {
        if (tempC < 10 && humidity > 70)
            return comfortColdDamp;

        if (tempC < 12)
            return comfortCold;

        if (tempC > 26 && humidity > 65)
            return comfortHotHumid;

        if (dewPoint > 18)
            return comfortHumid;

        if (tempC >= 18 && tempC <= 24 && humidity < 65)
            return comfortComfortable;

        return comfortFair;
    }

    /**
     * @brief Return a PROGMEM label string for a CondensationRisk value.
     *
     * @param risk Condensation risk level
     * @return PROGMEM string pointer — copy with strncpy_P before use
     */
    static const char* getCondensationRiskLabel(CondensationRisk risk)
    {
        switch (risk)
        {
            case CondensationRisk::High:
                return condensationHigh;
            case CondensationRisk::Watch:
                return condensationWatch;
            default:
                return condensationLow;
        }
    }
};