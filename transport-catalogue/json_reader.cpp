#include "json_reader.h"

#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "json_builder.h"

using namespace std;

namespace json_reader {

namespace {

struct StopInput {
    string name;
    double latitude = 0.0;
    double longitude = 0.0;
    vector<pair<string, int>> road_distances;
};

struct BusInput {
    string name;
    vector<string> stops;
    bool is_roundtrip = false;
};

svg::Color ParseColor(const json::Node& node) {
    if (node.IsString()) {
        return node.AsString();
    }

    const auto& arr = node.AsArray();

    if (arr.size() == 3) {
        return svg::Rgb{
            static_cast<uint8_t>(arr[0].AsInt()),
            static_cast<uint8_t>(arr[1].AsInt()),
            static_cast<uint8_t>(arr[2].AsInt())
        };
    }

    return svg::Rgba{
        static_cast<uint8_t>(arr[0].AsInt()),
        static_cast<uint8_t>(arr[1].AsInt()),
        static_cast<uint8_t>(arr[2].AsInt()),
        arr[3].AsDouble()
    };
}

svg::Point ParseOffset(const json::Node& node) {
    const auto& arr = node.AsArray();
    return {arr[0].AsDouble(), arr[1].AsDouble()};
}

StopInput ParseStop(const json::Dict& dict) {
    StopInput stop;
    stop.name = dict.at("name").AsString();
    stop.latitude = dict.at("latitude").AsDouble();
    stop.longitude = dict.at("longitude").AsDouble();

    const auto& distances = dict.at("road_distances").AsMap();
    for (const auto& [other_stop, distance_node] : distances) {
        stop.road_distances.push_back({other_stop, distance_node.AsInt()});
    }

    return stop;
}

BusInput ParseBus(const json::Dict& dict) {
    BusInput bus;
    bus.name = dict.at("name").AsString();
    bus.is_roundtrip = dict.at("is_roundtrip").AsBool();

    for (const auto& stop_node : dict.at("stops").AsArray()) {
        bus.stops.push_back(stop_node.AsString());
    }

    return bus;
}

void AddStopsToCatalogue(transport_catalogue::TransportCatalogue& catalogue,
                         const vector<StopInput>& stops) {
    for (const auto& stop : stops) {
        catalogue.AddStop(stop.name, {stop.latitude, stop.longitude});
    }
}

void AddDistancesToCatalogue(transport_catalogue::TransportCatalogue& catalogue,
                             const vector<StopInput>& stops) {
    for (const auto& stop : stops) {
        const domain::Stop* from = catalogue.FindStop(stop.name);
        for (const auto& [to_name, distance] : stop.road_distances) {
            const domain::Stop* to = catalogue.FindStop(to_name);
            if (from != nullptr && to != nullptr) {
                catalogue.SetDistanceBetweenStops(from, to, distance);
            }
        }
    }
}

void AddBusesToCatalogue(transport_catalogue::TransportCatalogue& catalogue,
                         const vector<BusInput>& buses) {
    for (const auto& bus : buses) {
        vector<string_view> stop_names;
        stop_names.reserve(bus.stops.size());

        for (const string& stop_name : bus.stops) {
            stop_names.push_back(stop_name);
        }

        catalogue.AddBus(bus.name, stop_names, bus.is_roundtrip);
    }
}

json::Node MakeErrorResponse(int request_id) {
    return json::Builder{}
        .StartDict()
            .Key("request_id").Value(request_id)
            .Key("error_message").Value(string("not found"))
        .EndDict()
        .Build();
}

json::Node MakeStopResponse(int request_id, const set<string_view>& buses) {
    json::Array buses_array;
    for (string_view bus : buses) {
        buses_array.push_back(string(bus));
    }

    json::Dict result = json::Builder{}
        .StartDict()
            .Key("request_id").Value(request_id)
            .Key("buses").Value(buses_array)
        .EndDict()
        .Build()
        .AsMap();

    return result;
}

json::Node MakeBusResponse(int request_id, const domain::BusInfo& info) {
    json::Dict result = json::Builder{}
        .StartDict()
            .Key("curvature").Value(info.curvature)
            .Key("request_id").Value(request_id)
            .Key("route_length").Value(info.route_length)
            .Key("stop_count").Value(info.stops_count)
            .Key("unique_stop_count").Value(info.unique_stops_count)
        .EndDict()
        .Build()
        .AsMap();

    return result;
}

json::Node MakeMapResponse(int request_id,
                           const request_handler::RequestHandler& handler,
                           const map_renderer::MapRenderer& renderer) {
    json::Dict result = json::Builder{}
        .StartDict()
            .Key("map").Value(handler.RenderMap(renderer))
            .Key("request_id").Value(request_id)
        .EndDict()
        .Build()
        .AsMap();

    return result;
}

}  // namespace

JsonReader::JsonReader(transport_catalogue::TransportCatalogue& catalogue)
    : catalogue_(catalogue) {
}

void JsonReader::ProcessBaseRequests(const json::Document& doc) {
    const auto& root = doc.GetRoot().AsMap();
    const auto& base_requests = root.at("base_requests").AsArray();

    vector<StopInput> stops;
    vector<BusInput> buses;

    for (const auto& request_node : base_requests) {
        const auto& request = request_node.AsMap();
        const string& type = request.at("type").AsString();

        if (type == "Stop") {
            stops.push_back(ParseStop(request));
        } else if (type == "Bus") {
            buses.push_back(ParseBus(request));
        }
    }

    AddStopsToCatalogue(catalogue_, stops);
    AddDistancesToCatalogue(catalogue_, stops);
    AddBusesToCatalogue(catalogue_, buses);
}

map_renderer::RenderSettings JsonReader::ParseRenderSettings(const json::Document& doc) const {
    const auto& root = doc.GetRoot().AsMap();
    const auto& settings_dict = root.at("render_settings").AsMap();

    map_renderer::RenderSettings settings;
    settings.width = settings_dict.at("width").AsDouble();
    settings.height = settings_dict.at("height").AsDouble();

    settings.padding = settings_dict.at("padding").AsDouble();

    settings.line_width = settings_dict.at("line_width").AsDouble();
    settings.stop_radius = settings_dict.at("stop_radius").AsDouble();

    settings.bus_label_font_size = settings_dict.at("bus_label_font_size").AsInt();
    settings.bus_label_offset = ParseOffset(settings_dict.at("bus_label_offset"));

    settings.stop_label_font_size = settings_dict.at("stop_label_font_size").AsInt();
    settings.stop_label_offset = ParseOffset(settings_dict.at("stop_label_offset"));

    settings.underlayer_color = ParseColor(settings_dict.at("underlayer_color"));
    settings.underlayer_width = settings_dict.at("underlayer_width").AsDouble();

    for (const auto& color_node : settings_dict.at("color_palette").AsArray()) {
        settings.color_palette.push_back(ParseColor(color_node));
    }

    return settings;
}

json::Document JsonReader::ProcessStatRequests(const json::Document& doc,
                                               const request_handler::RequestHandler& handler,
                                               const map_renderer::MapRenderer& renderer) const {
    const auto& root = doc.GetRoot().AsMap();
    const auto& stat_requests = root.at("stat_requests").AsArray();

    json::Array responses;

    for (const auto& request_node : stat_requests) {
        const auto& request = request_node.AsMap();
        const int request_id = request.at("id").AsInt();
        const string& type = request.at("type").AsString();

        if (type == "Map") {
            responses.push_back(MakeMapResponse(request_id, handler, renderer));
        } else if (type == "Stop") {
            const string& name = request.at("name").AsString();
            const auto* buses = handler.GetBusesByStop(name);
            if (buses == nullptr) {
                responses.push_back(MakeErrorResponse(request_id));
            } else {
                responses.push_back(MakeStopResponse(request_id, *buses));
            }
        } else if (type == "Bus") {
            const string& name = request.at("name").AsString();
            const auto info = handler.GetBusStat(name);
            if (!info) {
                responses.push_back(MakeErrorResponse(request_id));
            } else {
                responses.push_back(MakeBusResponse(request_id, *info));
            }
        }
    }

    return json::Document(json::Node(move(responses)));
}

}  // namespace json_reader