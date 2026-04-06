#pragma once

#include <deque>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "geo.h"

namespace transport_catalogue {

struct Stop {
    std::string name;
    geo::Coordinates coordinates;
};

struct Bus {
    std::string name;
    std::vector<const Stop*> stops;
};

struct BusInfo {
    int stops_count = 0;
    int unique_stops_count = 0;
    double route_length = 0.0;
};

class TransportCatalogue {
public:
    void AddStop(std::string name, geo::Coordinates coordinates);
    void AddBus(std::string name, const std::vector<std::string_view>& stop_names);

    const Stop* FindStop(std::string_view name) const;
    const Bus* FindBus(std::string_view name) const;

    std::optional<BusInfo> GetBusInfo(std::string_view bus_name) const;
    const std::set<std::string_view>* GetBusesByStop(std::string_view stop_name) const;

private:
    struct StringViewHasher {
        size_t operator()(std::string_view sv) const {
            return std::hash<std::string_view>{}(sv);
        }
    };

    std::deque<Stop> stops_;
    std::deque<Bus> buses_;

    std::unordered_map<std::string_view, const Stop*, StringViewHasher> stops_by_name_;
    std::unordered_map<std::string_view, const Bus*, StringViewHasher> buses_by_name_;
    std::unordered_map<std::string_view, std::set<std::string_view>, StringViewHasher> stop_to_buses_;
};

}
