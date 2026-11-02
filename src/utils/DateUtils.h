#pragma once
// Date and time helpers. Dates use the ISO format YYYY-MM-DD and times use
// HH:MM, so plain string comparison gives chronological order.

#include <cctype>
#include <cstdio>
#include <ctime>
#include <string>

namespace scapms::utils {

inline bool isLeapYear(int year) {
    return (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
}

inline int daysInMonth(int year, int month) {
    static const int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month == 2 && isLeapYear(year)) {
        return 29;
    }
    return days[month - 1];
}

inline bool allDigits(const std::string& text, std::size_t from, std::size_t count) {
    for (std::size_t i = from; i < from + count; ++i) {
        if (!std::isdigit(static_cast<unsigned char>(text[i]))) {
            return false;
        }
    }
    return true;
}

inline bool isValidDate(const std::string& date) {
    if (date.size() != 10 || date[4] != '-' || date[7] != '-') {
        return false;
    }
    if (!allDigits(date, 0, 4) || !allDigits(date, 5, 2) || !allDigits(date, 8, 2)) {
        return false;
    }
    const int year = std::stoi(date.substr(0, 4));
    const int month = std::stoi(date.substr(5, 2));
    const int day = std::stoi(date.substr(8, 2));
    if (year < 1900 || year > 2100 || month < 1 || month > 12) {
        return false;
    }
    return day >= 1 && day <= daysInMonth(year, month);
}

inline bool isValidTime(const std::string& time) {
    if (time.size() != 5 || time[2] != ':' || !allDigits(time, 0, 2) || !allDigits(time, 3, 2)) {
        return false;
    }
    const int hour = std::stoi(time.substr(0, 2));
    const int minute = std::stoi(time.substr(3, 2));
    return hour >= 0 && hour <= 23 && minute >= 0 && minute <= 59;
}

inline std::string formatTm(const std::tm& tm, const char* pattern) {
    char buffer[32];
    std::strftime(buffer, sizeof(buffer), pattern, &tm);
    return buffer;
}

inline std::tm localNow() {
    std::time_t now = std::time(nullptr);
    return *std::localtime(&now);
}

inline std::string today() { return formatTm(localNow(), "%Y-%m-%d"); }
inline std::string currentTime() { return formatTm(localNow(), "%H:%M"); }

inline int minutesOfDay(const std::string& time) {
    return std::stoi(time.substr(0, 2)) * 60 + std::stoi(time.substr(3, 2));
}

// Returns date shifted by the given number of days (negative moves back).
inline std::string addDays(const std::string& date, int days) {
    std::tm tm{};
    tm.tm_year = std::stoi(date.substr(0, 4)) - 1900;
    tm.tm_mon = std::stoi(date.substr(5, 2)) - 1;
    tm.tm_mday = std::stoi(date.substr(8, 2)) + days;
    tm.tm_hour = 12; // midday avoids daylight-saving edge cases
    std::mktime(&tm);
    return formatTm(tm, "%Y-%m-%d");
}

} // namespace scapms::utils
