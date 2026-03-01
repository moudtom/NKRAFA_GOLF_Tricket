#pragma once
#include <QString>
#include <QDate>

enum class DayType { Weekday, Weekend };
enum class HoleType { H18, H9 };
enum class CustomerType {
    Member,
    Thai,
    Foreigner,
    Cadet_NKRAFA,
    AirForceOfficer,
    MilitaryPolice
};

struct PriceBreakdown {
    int greenFee = 0;
    int cartFee  = 0;
    int caddyFee = 0;
    int total() const { return greenFee + cartFee + caddyFee; }
};

inline DayType detectDayType(const QDate& d)
{
    int wd = d.dayOfWeek(); // 1=Mon ... 7=Sun
    return (wd >= 6) ? DayType::Weekend : DayType::Weekday;
}

inline QString toString(CustomerType c)
{
    switch (c) {
    case CustomerType::Member:          return "Member";
    case CustomerType::Thai:            return "Thai";
    case CustomerType::Foreigner:       return "Foreigner";
    case CustomerType::Cadet_NKRAFA:    return "Cadet (NKRAFA)";
    case CustomerType::AirForceOfficer: return "Air Force Officer";
    case CustomerType::MilitaryPolice:  return "Military/Police";
    }
    return "Unknown";
}

inline QString toString(DayType d)  { return (d==DayType::Weekend) ? "Weekend" : "Weekday"; }
inline QString toString(HoleType h) { return (h==HoleType::H18) ? "18 Holes" : "9 Holes"; }
