#pragma once

#include "json.h"
#include "map_renderer.h"
#include "request_handler.h"
#include "transport_catalogue.h"
#include "transport_router.h"

namespace json_reader {

class JsonReader {
public:
    explicit JsonReader(transport_catalogue::TransportCatalogue& catalogue);

    void ProcessBaseRequests(const json::Document& doc);
    map_renderer::RenderSettings ParseRenderSettings(const json::Document& doc) const;
    domain::RoutingSettings ParseRoutingSettings(const json::Document& doc) const;

    json::Document ProcessStatRequests(const json::Document& doc,
                                       const request_handler::RequestHandler& handler,
                                       const map_renderer::MapRenderer& renderer,
                                       const transport_router::TransportRouter& router) const;

private:
    transport_catalogue::TransportCatalogue& catalogue_;
};

}  // namespace json_reader