#pragma once

#include <deque>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "domain.h"

namespace transport_catalogue {

class TransportCatalogue {
public:
    void AddStop(std::string name, geo::Coordinates coordinates);
    void AddBus(std::string name, const std::vector<std::string_view>& stop_names, bool is_roundtrip);

    void SetDistanceBetweenStops(const domain::Stop* from, const domain::Stop* to, int distance);
    int GetDistanceBetweenStops(const domain::Stop* from, const domain::Stop* to) const;

    const domain::Stop* FindStop(std::string_view name) const;
    const domain::Bus* FindBus(std::string_view name) const;

    std::optional<domain::BusInfo> GetBusInfo(std::string_view bus_name) const;
    const std::set<std::string_view>& GetBusesByStop(std::string_view stop_name) const;
    const std::deque<domain::Bus>& GetAllBuses() const;

private:
    struct StringViewHasher {
        size_t operator()(std::string_view sv) const {
            return std::hash<std::string_view>{}(sv);
        }
    };

    struct StopPairHasher {
        size_t operator()(const std::pair<const domain::Stop*, const domain::Stop*>& stops) const {
            return std::hash<const void*>{}(stops.first)
                 + 37u * std::hash<const void*>{}(stops.second);
        }
    };

    std::deque<domain::Stop> stops_;
    std::deque<domain::Bus> buses_;

    std::unordered_map<std::string_view, const domain::Stop*, StringViewHasher> stops_by_name_;
    std::unordered_map<std::string_view, const domain::Bus*, StringViewHasher> buses_by_name_;
    std::unordered_map<std::string_view, std::set<std::string_view>, StringViewHasher> stop_to_buses_;
    std::unordered_map<std::pair<const domain::Stop*, const domain::Stop*>, int, StopPairHasher> distances_;
};

}  // namespace transport_catalogue