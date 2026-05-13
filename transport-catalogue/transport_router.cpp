#include "transport_router.h"

namespace transport_router {

TransportRouter::TransportRouter(const transport_catalogue::TransportCatalogue& catalogue, const domain::RoutingSettings& settings)
    : settings_(settings) {
    const auto& stops = catalogue.GetSortedStops();
    graph_ = std::make_unique<graph::DirectedWeightedGraph<double>>(stops.size() * 2);
    
    graph::VertexId v_id = 0;
    for (const auto& [name, stop_ptr] : stops) {
        stop_to_vertex_[name] = v_id;
        vertex_to_stop_name_[v_id] = name;
        graph_->AddEdge({v_id, v_id + 1, static_cast<double>(settings_.bus_wait_time)});
        v_id += 2;
    }

    const double velocity_m_min = settings_.bus_velocity * 1000.0 / 60.0;

    for (const auto& bus : catalogue.GetAllBuses()) {
        const auto& b_stops = bus.stops;
        if (b_stops.empty()) continue;

        auto add_edges = [&](size_t start_idx, size_t end_idx, bool forward) {
            for (size_t i = start_idx; i != end_idx; forward ? ++i : --i) {
                int dist_sum = 0;
                int span_count = 0;
                for (size_t j = forward ? i + 1 : i - 1; j != (forward ? end_idx + 1 : end_idx - 1); forward ? ++j : --j) {
                    dist_sum += catalogue.GetDistanceBetweenStops(b_stops[forward ? j - 1 : j + 1], b_stops[j]);
                    ++span_count;
                    
                    graph::EdgeId id = graph_->AddEdge({
                        stop_to_vertex_.at(b_stops[i]->name) + 1,
                        stop_to_vertex_.at(b_stops[j]->name),
                        static_cast<double>(dist_sum) / velocity_m_min
                    });
                    edge_id_to_info_[id] = {bus.name, span_count};
                }
            }
        };

        if (bus.is_roundtrip) {
            add_edges(0, b_stops.size() - 1, true);
        } else {
            add_edges(0, b_stops.size() - 1, true);
            add_edges(b_stops.size() - 1, 0, false);
        }
    }
    router_ = std::make_unique<graph::Router<double>>(*graph_);
}

std::optional<domain::RouteInfo> TransportRouter::FindRoute(std::string_view from, std::string_view to) const {
    if (from == to) return domain::RouteInfo{0, {}};
    if (!stop_to_vertex_.count(from) || !stop_to_vertex_.count(to)) return std::nullopt;

    auto route = router_->BuildRoute(stop_to_vertex_.at(from), stop_to_vertex_.at(to));
    if (!route) return std::nullopt;

    domain::RouteInfo res;
    res.total_time = route->weight;
    for (auto e_id : route->edges) {
        const auto& edge = graph_->GetEdge(e_id);
        if (edge_id_to_info_.count(e_id)) {
            const auto& info = edge_id_to_info_.at(e_id);
            res.items.push_back(domain::RouteItemBus{std::string(info.bus_name), info.span_count, edge.weight});
        } else {
            res.items.push_back(domain::RouteItemWait{std::string(vertex_to_stop_name_.at(edge.from)), edge.weight});
        }
    }
    return res;
}

} // namespace transport_router
