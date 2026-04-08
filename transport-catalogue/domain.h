#pragma once

#include <string>
#include <vector>

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

}  // namespace domain