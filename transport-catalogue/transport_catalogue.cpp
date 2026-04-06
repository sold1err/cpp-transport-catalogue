#include "transport_catalogue.h"

using namespace std;

namespace transport_catalogue {

void TransportCatalogue::AddStop(string name, geo::Coordinates coordinates) {
    stops_.push_back({move(name), coordinates});
    const Stop* stop_ptr = &stops_.back();
    stops_by_name_[stop_ptr->name] = stop_ptr;
}

void TransportCatalogue::AddBus(string name, const vector<string_view>& stop_names) {
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
    for (const Stop* stop : bus->stops) {
        unique_stops.insert(stop->name);
    }
    info.unique_stops_count = static_cast<int>(unique_stops.size());

    for (size_t i = 1; i < bus->stops.size(); ++i) {
        info.route_length += geo::ComputeDistance(
            bus->stops[i - 1]->coordinates,
            bus->stops[i]->coordinates
            );
    }

    return info;
}

const set<string_view>* TransportCatalogue::GetBusesByStop(string_view stop_name) const {
    if (!FindStop(stop_name)) {
        return nullptr;
    }

    if (auto it = stop_to_buses_.find(stop_name); it != stop_to_buses_.end()) {
        return &it->second;
    }

    static const set<string_view> empty_buses;
    return &empty_buses;
}

}
