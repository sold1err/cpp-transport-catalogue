#include "json_reader.h"
#include "json_builder.h"
#include <vector>
#include <string>

using namespace std;

namespace json_reader {

struct StopInput {
    string name;
    double lat, lng;
    vector<pair<string, int>> distances;
};

struct BusInput {
    string name;
    vector<string> stops;
    bool is_roundtrip;
};

JsonReader::JsonReader(transport_catalogue::TransportCatalogue& catalogue)
    : catalogue_(catalogue) {}

void JsonReader::ProcessBaseRequests(const json::Document& doc) {
    const auto& root = doc.GetRoot().AsMap();
    if (!root.count("base_requests")) return;

    const auto& base_requests = root.at("base_requests").AsArray();
    vector<StopInput> stops_input;
    vector<BusInput> buses_input;

    for (const auto& node : base_requests) {
        const auto& req = node.AsMap();
        const string& type = req.at("type").AsString();
        if (type == "Stop") {
            StopInput si;
            si.name = req.at("name").AsString();
            si.lat = req.at("latitude").AsDouble();
            si.lng = req.at("longitude").AsDouble();
            for (const auto& [name, dist] : req.at("road_distances").AsMap()) {
                si.distances.push_back({name, dist.AsInt()});
            }
            stops_input.push_back(move(si));
        } else {
            BusInput bi;
            bi.name = req.at("name").AsString();
            bi.is_roundtrip = req.at("is_roundtrip").AsBool();
            for (const auto& s : req.at("stops").AsArray()) {
                bi.stops.push_back(s.AsString());
            }
            buses_input.push_back(move(bi));
        }
    }

    for (const auto& si : stops_input) catalogue_.AddStop(si.name, {si.lat, si.lng});
    for (const auto& si : stops_input) {
        const auto* from = catalogue_.FindStop(si.name);
        for (const auto& [to_name, dist] : si.distances) {
            catalogue_.SetDistanceBetweenStops(from, catalogue_.FindStop(to_name), dist);
        }
    }
    for (const auto& bi : buses_input) {
        vector<string_view> sv_stops;
        for (const auto& s : bi.stops) sv_stops.push_back(s);
        catalogue_.AddBus(bi.name, sv_stops, bi.is_roundtrip);
    }
}

map_renderer::RenderSettings JsonReader::ParseRenderSettings(const json::Document& doc) const {
    const auto& root = doc.GetRoot().AsMap();
    const auto& rs = root.at("render_settings").AsMap();
    map_renderer::RenderSettings settings;
    
    settings.width = rs.at("width").AsDouble();
    settings.height = rs.at("height").AsDouble();
    settings.padding = rs.at("padding").AsDouble();
    settings.line_width = rs.at("line_width").AsDouble();
    settings.stop_radius = rs.at("stop_radius").AsDouble();
    settings.bus_label_font_size = rs.at("bus_label_font_size").AsInt();
    settings.stop_label_font_size = rs.at("stop_label_font_size").AsInt();
    settings.underlayer_width = rs.at("underlayer_width").AsDouble();

    auto parse_color = [](const json::Node& n) -> svg::Color {
        if (n.IsString()) return n.AsString();
        const auto& a = n.AsArray();
        if (a.size() == 3) return svg::Rgb{static_cast<uint8_t>(a[0].AsInt()), static_cast<uint8_t>(a[1].AsInt()), static_cast<uint8_t>(a[2].AsInt())};
        return svg::Rgba{static_cast<uint8_t>(a[0].AsInt()), static_cast<uint8_t>(a[1].AsInt()), static_cast<uint8_t>(a[2].AsInt()), a[3].AsDouble()};
    };

    settings.underlayer_color = parse_color(rs.at("underlayer_color"));
    for (const auto& n : rs.at("color_palette").AsArray()) settings.color_palette.push_back(parse_color(n));
    
    const auto& b_off = rs.at("bus_label_offset").AsArray();
    settings.bus_label_offset = {b_off[0].AsDouble(), b_off[1].AsDouble()};
    const auto& s_off = rs.at("stop_label_offset").AsArray();
    settings.stop_label_offset = {s_off[0].AsDouble(), s_off[1].AsDouble()};

    return settings;
}

domain::RoutingSettings JsonReader::ParseRoutingSettings(const json::Document& doc) const {
    const auto& root = doc.GetRoot().AsMap();
    domain::RoutingSettings settings;
    if (root.count("routing_settings")) {
        const auto& rs = root.at("routing_settings").AsMap();
        settings.bus_wait_time = rs.at("bus_wait_time").AsInt();
        settings.bus_velocity = rs.at("bus_velocity").AsDouble();
    }
    return settings;
}

json::Document JsonReader::ProcessStatRequests(const json::Document& doc,
                                               const request_handler::RequestHandler& handler,
                                               const map_renderer::MapRenderer& renderer,
                                               const transport_router::TransportRouter& router) const {
    const auto& root = doc.GetRoot().AsMap();
    const auto& stat_requests = root.at("stat_requests").AsArray();
    json::Array responses;

    for (const auto& node : stat_requests) {
        const auto& req = node.AsMap();
        int id = req.at("id").AsInt();
        string type = req.at("type").AsString();

        if (type == "Stop") {
            string name = req.at("name").AsString();
            const auto* buses = handler.GetBusesByStop(name);
            if (!catalogue_.FindStop(name)) {
                responses.push_back(json::Builder{}.StartDict().Key("request_id").Value(id).Key("error_message").Value("not found").EndDict().Build());
            } else {
                json::Array b_list;
                for (auto b : *buses) b_list.push_back(string(b));
                responses.push_back(json::Builder{}.StartDict().Key("request_id").Value(id).Key("buses").Value(b_list).EndDict().Build());
            }
        } else if (type == "Bus") {
            string name = req.at("name").AsString();
            auto info = handler.GetBusStat(name);
            if (!info) {
                responses.push_back(json::Builder{}.StartDict().Key("request_id").Value(id).Key("error_message").Value("not found").EndDict().Build());
            } else {
                responses.push_back(json::Builder{}.StartDict()
                    .Key("request_id").Value(id)
                    .Key("curvature").Value(info->curvature)
                    .Key("route_length").Value(info->route_length)
                    .Key("stop_count").Value(info->stops_count)
                    .Key("unique_stop_count").Value(info->unique_stops_count).EndDict().Build());
            }
        } else if (type == "Map") {
            responses.push_back(json::Builder{}.StartDict().Key("request_id").Value(id).Key("map").Value(handler.RenderMap(renderer)).EndDict().Build());
        } else if (type == "Route") {
            auto route = router.FindRoute(req.at("from").AsString(), req.at("to").AsString());
            if (!route) {
                responses.push_back(json::Builder{}.StartDict()
                    .Key("request_id").Value(id)
                    .Key("error_message").Value("not found")
                    .EndDict().Build());
            } else {
                json::Array items;
                for (const auto& item : route->items) {
                    if (std::holds_alternative<domain::RouteItemWait>(item)) {
                        const auto& w = std::get<domain::RouteItemWait>(item);
                        items.push_back(json::Builder{}.StartDict()
                            .Key("type").Value("Wait")
                            .Key("stop_name").Value(w.stop_name)
                            .Key("time").Value(w.time)
                            .EndDict().Build());
                    } else {
                        const auto& b = std::get<domain::RouteItemBus>(item);
                        items.push_back(json::Builder{}.StartDict()
                            .Key("type").Value("Bus")
                            .Key("bus").Value(b.bus_name)
                            .Key("span_count").Value(b.span_count)
                            .Key("time").Value(b.time)
                            .EndDict().Build());
                    }
                }
                responses.push_back(json::Builder{}.StartDict()
                    .Key("request_id").Value(id)
                    .Key("total_time").Value(route->total_time)
                    .Key("items").Value(items)
                    .EndDict().Build());
            }
        }
    }
    return json::Document(move(responses));
}

} // namespace json_reader
