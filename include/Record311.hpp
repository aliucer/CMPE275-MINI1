#pragma once
#include <cstdint>
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

struct ColumnMap {
    int unique_key     = -1;
    int created_date   = -1;
    int complaint_type = -1;
    int borough        = -1;
    int latitude       = -1;
    int longitude      = -1;

    bool valid() const { return unique_key >= 0 && created_date >= 0; }

    static ColumnMap fromHeader(const std::vector<std::string>& hdr) {
        ColumnMap m;
        for (int i = 0; i < static_cast<int>(hdr.size()); ++i) {
            const auto& h = hdr[i];
            if      (h == "unique_key"   || h == "Unique Key")   m.unique_key   = i;
            else if (h == "created_date" || h == "Created Date")     m.created_date   = i;
            else if (h == "complaint_type" || h == "Complaint Type") m.complaint_type = i;
            else if (h == "borough"      || h == "Borough")          m.borough        = i;
            else if (h == "latitude"     || h == "Latitude")         m.latitude       = i;
            else if (h == "longitude"    || h == "Longitude")        m.longitude      = i;
        }
        return m;
    }
};

struct Record311 {
    uint64_t    unique_key     = 0;
    uint32_t    created_ymd    = 0;     // YYYYMMDD
    std::string complaint_type;
    Borough     borough        = Borough::UNSPECIFIED;
    double      latitude       = 0.0;
    double      longitude      = 0.0;

    // "2020-03-09T01:12:45.000" -> 20200309
    static uint32_t parseDate(const std::string& s) {
        if (s.size() < 10) return 0;
        uint32_t y = (s[0]-'0')*1000 + (s[1]-'0')*100 + (s[2]-'0')*10 + (s[3]-'0');
        uint32_t m = (s[5]-'0')*10 + (s[6]-'0');
        uint32_t d = (s[8]-'0')*10 + (s[9]-'0');
        return y * 10000 + m * 100 + d;
    }

    static bool fromFields(const std::vector<std::string>& f,
                           const ColumnMap& cm, Record311& r) {
        int n = static_cast<int>(f.size());
        if (cm.unique_key >= n || cm.created_date >= n) return false;

        r = Record311{};
        try {
            r.unique_key  = std::stoull(f[cm.unique_key]);
            r.created_ymd = parseDate(f[cm.created_date]);
            if (cm.complaint_type >= 0 && cm.complaint_type < n)
                r.complaint_type = f[cm.complaint_type];
            if (cm.borough >= 0 && cm.borough < n)
                r.borough = boroughFromString(f[cm.borough]);
            if (cm.latitude >= 0 && cm.latitude < n && !f[cm.latitude].empty())
                r.latitude = std::stod(f[cm.latitude]);
            if (cm.longitude >= 0 && cm.longitude < n && !f[cm.longitude].empty())
                r.longitude = std::stod(f[cm.longitude]);
            return r.unique_key > 0;
        } catch (...) {
            return false;
        }
    }
};

}
