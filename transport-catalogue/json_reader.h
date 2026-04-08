#pragma once

#include "json.h"
#include "map_renderer.h"
#include "request_handler.h"
#include "transport_catalogue.h"

namespace json_reader {

class JsonReader {
public:
    explicit JsonReader(transport_catalogue::TransportCatalogue& catalogue);

    void ProcessBaseRequests(const json::Document& doc);
    map_renderer::RenderSettings ParseRenderSettings(const json::Document& doc) const;

    json::Document ProcessStatRequests(const json::Document& doc,
                                       const request_handler::RequestHandler& handler,
                                       const map_renderer::MapRenderer& renderer) const;

private:
    transport_catalogue::TransportCatalogue& catalogue_;
};

}  // namespace json_reader