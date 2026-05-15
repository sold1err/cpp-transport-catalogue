#pragma once

#include "graph.h"
#include "router.h"
#include "transport_catalogue.h"
#include "domain.h"
#include <memory>
#include <map>
#include <string_view>
#include <unordered_map>

namespace transport_router {

class TransportRouter {
public:
    TransportRouter(const transport_catalogue::TransportCatalogue& catalogue, const domain::RoutingSettings& settings);
    std::optional<domain::RouteInfo> FindRoute(std::string_view from, std::string_view to) const;

private:
    struct EdgeInfo {
        std::string_view bus_name;
        int span_count;
    };

    void FillGraphWithStops(const std::vector<const domain::Stop*>& stops);
    void FillGraphWithBuses(const transport_catalogue::TransportCatalogue& catalogue);
    void AddBusEdges(const transport_catalogue::TransportCatalogue& catalogue, const domain::Bus&, size_t start_idx, size_t end_idx, bool forward);

    const domain::RoutingSettings settings_;
    graph::DirectedWeightedGraph<double> graph_;
    std::unique_ptr<graph::Router<double>> router_;
    std::unordered_map<graph::EdgeId, EdgeInfo> edge_id_to_info_;
    std::unordered_map<std::string_view, graph::VertexId> stop_to_vertex_;
    std::unordered_map<graph::VertexId, std::string_view> vertex_to_stop_name_;
};

} // namespace transport_router
