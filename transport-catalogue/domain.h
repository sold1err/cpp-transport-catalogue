#pragma once

#include <string>
#include <vector>
#include <variant>
#include "geo.h"

namespace domain {

struct Stop {
    std::string name;
    geo::Coordinates coordinates;
};

struct Bus {
    std::string name;
    std::vector<const Stop*> stops;
    bool is_roundtrip = false;
};

struct BusInfo {
    int stops_count = 0;
    int unique_stops_count = 0;
    int route_length = 0;
    double curvature = 0.0;
};

struct RoutingSettings {
    int bus_wait_time = 0;
    double bus_velocity = 0.0;
};

struct RouteItemWait {
    std::string stop_name;
    double time = 0;
};

struct RouteItemBus {
    std::string bus_name;
    int span_count = 0;
    double time = 0;
};

using RouteItem = std::variant<RouteItemWait, RouteItemBus>;

struct RouteInfo {
    double total_time = 0;
    std::vector<RouteItem> items;
};

}  // namespace domain