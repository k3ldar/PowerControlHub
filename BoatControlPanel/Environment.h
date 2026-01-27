#pragma once

constexpr char comfortColdDamp[] PROGMEM = "Cold & Damp";
constexpr char comfortCold[] PROGMEM = "Cold";
constexpr char comfortComfortable[] PROGMEM = "Comfortable";
constexpr char comfortWarm[] PROGMEM = "Warm";
constexpr char comfortHotHumid[] PROGMEM = "Hot & Humid";
constexpr char comfortHumid[] PROGMEM = "Humid";
constexpr char comfortDry[] PROGMEM = "Dry";
constexpr char comfortFair[] PROGMEM = "Fair";

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
};