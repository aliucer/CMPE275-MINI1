#pragma once
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace nyc311 {

enum class Borough : uint8_t {
    UNSPECIFIED   = 0,
    MANHATTAN     = 1,
    BROOKLYN      = 2,
    QUEENS        = 3,
    BRONX         = 4,
    STATEN_ISLAND = 5
};

inline Borough boroughFromString(const std::string& s) {
    if (s == "MANHATTAN")      return Borough::MANHATTAN;
    if (s == "BROOKLYN")       return Borough::BROOKLYN;
    if (s == "QUEENS")         return Borough::QUEENS;
    if (s == "BRONX")          return Borough::BRONX;
    if (s == "STATEN ISLAND")  return Borough::STATEN_ISLAND;
    return Borough::UNSPECIFIED;
}

inline std::string boroughToString(Borough b) {
    switch (b) {
        case Borough::MANHATTAN:     return "MANHATTAN";
        case Borough::BROOKLYN:      return "BROOKLYN";
        case Borough::QUEENS:        return "QUEENS";
        case Borough::BRONX:         return "BRONX";
        case Borough::STATEN_ISLAND: return "STATEN ISLAND";
        default:                     return "UNSPECIFIED";
    }
}

struct Record311 {
    uint64_t unique_key   = 0;
    uint32_t created_ymd  = 0;     // YYYYMMDD
    Borough  borough      = Borough::UNSPECIFIED;
    double   latitude     = 0.0;
    double   longitude    = 0.0;

    // "03/09/2024 01:12:45 AM" → 20240309
    static uint32_t parseDate(const std::string& s) {
        if (s.size() < 10 || s[2] != '/')
            throw std::invalid_argument("bad date: " + s);
        uint32_t m = (s[0]-'0')*10 + (s[1]-'0');
        uint32_t d = (s[3]-'0')*10 + (s[4]-'0');
        uint32_t y = (s[6]-'0')*1000 + (s[7]-'0')*100 + (s[8]-'0')*10 + (s[9]-'0');
        return y * 10000 + m * 100 + d;
    }

    static bool fromFields(const std::vector<std::string>& f, Record311& r) {
        if (f.size() < 29) return false;

        // Reset to avoid stale values from previous row
        r = Record311{};

        try {
            r.unique_key  = std::stoull(f[0]);
            r.created_ymd = parseDate(f[1]);
            r.borough     = boroughFromString(f[28]);
            if (f.size() > 41 && !f[41].empty()) r.latitude  = std::stod(f[41]);
            if (f.size() > 42 && !f[42].empty()) r.longitude = std::stod(f[42]);
            return true;
        } catch (...) {
            return false;
        }
    }
};

}
