#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <tuple>
#include <vector>

namespace nyc311 {

class IDataStore {
public:
    virtual ~IDataStore() = default;

    virtual void load(const std::string& path) = 0;
    virtual void loadMultiple(const std::vector<std::string>& paths) = 0;

    virtual size_t size() const = 0;
    virtual size_t memoryBytes() const = 0;

    virtual std::vector<size_t> filterByBorough(const std::string& b) const = 0;
    virtual std::vector<size_t> filterByComplaintType(const std::string& type) const = 0;
    virtual std::vector<size_t> filterByGeoBox(double minLat, double maxLat,
                                                double minLon, double maxLon) const = 0;
    virtual std::vector<size_t> filterByDateRange(uint32_t startYmd, uint32_t endYmd) const = 0;
    virtual std::tuple<double, double, size_t> computeCentroid() const = 0;
};

}
