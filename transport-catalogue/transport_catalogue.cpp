#include "transport_catalogue.h"

using namespace std;

namespace transport_catalogue {

void TransportCatalogue::AddStop(const string& name, geo::Coordinates coordinates) {
    stops_.push_back({move(name), coordinates});
    const Stop* stop_ptr = &stops_.back();
    stops_by_name_[stop_ptr->name] = stop_ptr;
}

void TransportCatalogue::AddBus(const string& name, const vector<string_view>& stop_names) {
    buses_.push_back(Bus{});
    Bus& bus = buses_.back();
    bus.name = move(name);

    for (string_view stop_name : stop_names) {
        if (const Stop* stop = FindStop(stop_name)) {
            bus.stops.push_back(stop);
        }
    }

    const Bus* bus_ptr = &bus;
    buses_by_name_[bus_ptr->name] = bus_ptr;

    for (const Stop* stop : bus.stops) {
        stop_to_buses_[stop->name].insert(bus_ptr->name);
    }
}

void TransportCatalogue::SetDistanceBetweenStops(const Stop* from, const Stop* to, int distance) {
    distances_[{from, to}] = distance;
}

int TransportCatalogue::GetDistanceBetweenStops(const Stop* from, const Stop* to) const {
    if (auto it = distances_.find({from, to}); it != distances_.end()) {
        return it->second;
    }
    if (auto it = distances_.find({to, from}); it != distances_.end()) {
        return it->second;
    }
    return 0;
}

const Stop* TransportCatalogue::FindStop(string_view name) const {
    if (auto it = stops_by_name_.find(name); it != stops_by_name_.end()) {
        return it->second;
    }
    return nullptr;
}

const Bus* TransportCatalogue::FindBus(string_view name) const {
    if (auto it = buses_by_name_.find(name); it != buses_by_name_.end()) {
        return it->second;
    }
    return nullptr;
}

optional<BusInfo> TransportCatalogue::GetBusInfo(string_view bus_name) const {
    const Bus* bus = FindBus(bus_name);
    if (!bus) {
        return nullopt;
    }

    BusInfo info;
    info.stops_count = static_cast<int>(bus->stops.size());

    unordered_set<string_view, StringViewHasher> unique_stops;
    double geo_length = 0.0;
    int road_length = 0;

    for (const Stop* stop : bus->stops) {
        unique_stops.insert(stop->name);
    }
    info.unique_stops_count = static_cast<int>(unique_stops.size());

    for (size_t i = 1; i < bus->stops.size(); ++i) {
        const Stop* from = bus->stops[i - 1];
        const Stop* to = bus->stops[i];

        geo_length += geo::ComputeDistance(from->coordinates, to->coordinates);
        road_length += GetDistanceBetweenStops(from, to);
    }

    info.route_length = road_length;
    info.curvature = (geo_length == 0.0 ? 0.0 : road_length / geo_length);

    return info;
}

const set<string_view>& TransportCatalogue::GetBusesByStop(string_view stop_name) const {
    static const std::set<std::string_view> empty_buses;

    if (auto it = stop_to_buses_.find(stop_name); it != stop_to_buses_.end()) {
        return it->second;
    }

    return empty_buses;
}

}
