#include "request_handler.h"

#include <algorithm>
#include <set>

namespace request_handler {

RequestHandler::RequestHandler(const transport_catalogue::TransportCatalogue& catalogue)
    : catalogue_(catalogue) {
}

std::optional<domain::BusInfo> RequestHandler::GetBusStat(std::string_view bus_name) const {
    return catalogue_.GetBusInfo(bus_name);
}

const std::set<std::string_view>* RequestHandler::GetBusesByStop(std::string_view stop_name) const {
    if (catalogue_.FindStop(stop_name) == nullptr) {
        return nullptr;
    }
    return &catalogue_.GetBusesByStop(stop_name);
}

std::vector<const domain::Bus*> RequestHandler::GetSortedBuses() const {
    std::vector<const domain::Bus*> result;
    for (const auto& bus : catalogue_.GetAllBuses()) {
        result.push_back(&bus);
    }

    std::sort(result.begin(), result.end(),
              [](const domain::Bus* lhs, const domain::Bus* rhs) {
                  return lhs->name < rhs->name;
              });
    return result;
}

std::vector<const domain::Stop*> RequestHandler::GetRouteStops(const domain::Bus& bus) const {
    std::vector<const domain::Stop*> result = bus.stops;

    if (!bus.is_roundtrip && !bus.stops.empty()) {
        for (int i = static_cast<int>(bus.stops.size()) - 2; i >= 0; --i) {
            result.push_back(bus.stops[static_cast<size_t>(i)]);
        }
    }

    return result;
}

std::vector<const domain::Stop*> RequestHandler::GetSortedStopsForMap() const {
    std::set<const domain::Stop*, bool(*)(const domain::Stop*, const domain::Stop*)> unique_stops(
        [](const domain::Stop* lhs, const domain::Stop* rhs) {
            return lhs->name < rhs->name;
        }
    );

    for (const domain::Bus& bus : catalogue_.GetAllBuses()) {
        for (const domain::Stop* stop : bus.stops) {
            unique_stops.insert(stop);
        }
    }

    return {unique_stops.begin(), unique_stops.end()};
}

}  // namespace request_handler