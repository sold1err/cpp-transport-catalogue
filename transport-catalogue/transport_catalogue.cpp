#include "transport_catalogue.h"
#include <unordered_set>

using namespace std;

namespace transport_catalogue {

void TransportCatalogue::AddStop(string name, geo::Coordinates coordinates) {
    stops_.push_back({move(name), coordinates});
    const domain::Stop* stop_ptr = &stops_.back();
    stops_by_name_[stop_ptr->name] = stop_ptr;
    sorted_stops_[stop_ptr->name] = stop_ptr;
}

void TransportCatalogue::AddBus(string name, const vector<string_view>& stop_names, bool is_roundtrip) {
    buses_.push_back(domain::Bus{});
    domain::Bus& bus = buses_.back();
    bus.name = move(name);
    bus.is_roundtrip = is_roundtrip;

    for (string_view stop_name : stop_names) {
        if (const domain::Stop* stop = FindStop(stop_name)) {
            bus.stops.push_back(stop);
        }
    }

    const domain::Bus* bus_ptr = &bus;
    buses_by_name_[bus_ptr->name] = bus_ptr;

    for (const domain::Stop* stop : bus.stops) {
        stop_to_buses_[stop->name].insert(bus_ptr->name);
    }
}

void TransportCatalogue::SetDistanceBetweenStops(const domain::Stop* from, const domain::Stop* to, int distance) {
    distances_[{from, to}] = distance;
}

int TransportCatalogue::GetDistanceBetweenStops(const domain::Stop* from, const domain::Stop* to) const {
    if (auto it = distances_.find({from, to}); it != distances_.end()) {
        return it->second;
    }
    if (auto it = distances_.find({to, from}); it != distances_.end()) {
        return it->second;
    }
    return 0;
}

const domain::Stop* TransportCatalogue::FindStop(string_view name) const {
    if (auto it = stops_by_name_.find(name); it != stops_by_name_.end()) {
        return it->second;
    }
    return nullptr;
}

const domain::Bus* TransportCatalogue::FindBus(string_view name) const {
    if (auto it = buses_by_name_.find(name); it != buses_by_name_.end()) {
        return it->second;
    }
    return nullptr;
}

optional<domain::BusInfo> TransportCatalogue::GetBusInfo(string_view bus_name) const {
    const domain::Bus* bus = FindBus(bus_name);
    if (!bus) return nullopt;

    domain::BusInfo info;
    const size_t stop_count = bus->stops.size();
    if (stop_count == 0) return info;

    unordered_set<string_view, StringViewHasher> unique_stops;
    for (const domain::Stop* stop : bus->stops) unique_stops.insert(stop->name);

    info.unique_stops_count = static_cast<int>(unique_stops.size());
    
    double geo_length = 0.0;
    int road_length = 0;

    for (size_t i = 1; i < stop_count; ++i) {
        geo_length += geo::ComputeDistance(bus->stops[i - 1]->coordinates, bus->stops[i]->coordinates);
        road_length += GetDistanceBetweenStops(bus->stops[i - 1], bus->stops[i]);
    }

    if (bus->is_roundtrip) {
        info.stops_count = static_cast<int>(stop_count);
    } else {
        info.stops_count = static_cast<int>(stop_count * 2 - 1);
        for (size_t i = stop_count - 1; i > 0; --i) {
            geo_length += geo::ComputeDistance(bus->stops[i]->coordinates, bus->stops[i - 1]->coordinates);
            road_length += GetDistanceBetweenStops(bus->stops[i], bus->stops[i - 1]);
        }
    }

    info.route_length = road_length;
    info.curvature = road_length / geo_length;
    return info;
}

const set<string_view>& TransportCatalogue::GetBusesByStop(string_view stop_name) const {
    static const set<string_view> empty_buses;
    if (auto it = stop_to_buses_.find(stop_name); it != stop_to_buses_.end()) {
        return it->second;
    }
    return empty_buses;
}

const deque<domain::Bus>& TransportCatalogue::GetAllBuses() const {
    return buses_;
}

const map<string_view, const domain::Stop*>& TransportCatalogue::GetSortedStops() const {
    return sorted_stops_;
}

}  // namespace transport_catalogue