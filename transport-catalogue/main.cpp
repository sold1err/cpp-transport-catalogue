#include <iostream>

#include "json.h"
#include "json_reader.h"
#include "map_renderer.h"
#include "request_handler.h"
#include "transport_catalogue.h"

int main() {
    transport_catalogue::TransportCatalogue catalogue;
    json_reader::JsonReader reader(catalogue);

    const json::Document input_doc = json::Load(std::cin);

    reader.ProcessBaseRequests(input_doc);
    const map_renderer::RenderSettings render_settings = reader.ParseRenderSettings(input_doc);

    request_handler::RequestHandler handler(catalogue);
    map_renderer::MapRenderer renderer(render_settings);

    const json::Document output_doc = reader.ProcessStatRequests(input_doc, handler, renderer);
    json::Print(output_doc, std::cout);
}