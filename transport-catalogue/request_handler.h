#pragma once

#include <optional>
#include <set>
#include <string_view>
#include <vector>

#include "domain.h"
#include "transport_catalogue.h"

namespace request_handler {

class RequestHandler {
public:
    explicit RequestHandler(const transport_catalogue::TransportCatalogue& catalogue);

    std::optional<domain::BusInfo> GetBusStat(std::string_view bus_name) const;
    const std::set<std::string_view>* GetBusesByStop(std::string_view stop_name) const;

    std::vector<const domain::Bus*> GetSortedBuses() const;
    std::vector<const domain::Stop*> GetRouteStops(const domain::Bus& bus) const;
    std::vector<const domain::Stop*> GetSortedStopsForMap() const;

private:
    const transport_catalogue::TransportCatalogue& catalogue_;
};

}  // namespace request_handler