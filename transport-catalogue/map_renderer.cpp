#include "map_renderer.h"

#include <utility>
#include <vector>

namespace map_renderer {

MapRenderer::MapRenderer(RenderSettings settings)
    : settings_(std::move(settings)) {
}

svg::Document MapRenderer::Render(const request_handler::RequestHandler& handler) const {
    svg::Document doc;

    const std::vector<const domain::Bus*> all_buses = handler.GetSortedBuses();
    const std::vector<const domain::Stop*> stops_for_map = handler.GetSortedStopsForMap();

    std::vector<const domain::Bus*> buses_to_render;
    std::vector<geo::Coordinates> route_points;

    for (const domain::Bus* bus : all_buses) {
        if (bus->stops.empty()) {
            continue;
        }

        buses_to_render.push_back(bus);

        for (const domain::Stop* stop : handler.GetRouteStops(*bus)) {
            route_points.push_back(stop->coordinates);
        }
    }

    SphereProjector projector(route_points.begin(), route_points.end(),
                              settings_.width, settings_.height, settings_.padding);

    RenderBusLines(doc, buses_to_render, projector, handler);
    RenderBusLabels(doc, buses_to_render, projector);
    RenderStopPoints(doc, stops_for_map, projector);
    RenderStopLabels(doc, stops_for_map, projector);

    return doc;
}

void MapRenderer::RenderBusLines(svg::Document& doc,
                                 const std::vector<const domain::Bus*>& buses,
                                 const SphereProjector& projector,
                                 const request_handler::RequestHandler& handler) const {
    size_t color_index = 0;

    for (const domain::Bus* bus : buses) {
        svg::Polyline polyline;
        polyline.SetFillColor(svg::NoneColor)
                .SetStrokeColor(settings_.color_palette[color_index % settings_.color_palette.size()])
                .SetStrokeWidth(settings_.line_width)
                .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
                .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

        for (const domain::Stop* stop : handler.GetRouteStops(*bus)) {
            polyline.AddPoint(projector(stop->coordinates));
        }

        doc.Add(polyline);
        ++color_index;
    }
}

void MapRenderer::RenderBusLabels(svg::Document& doc,
                                  const std::vector<const domain::Bus*>& buses,
                                  const SphereProjector& projector) const {
    size_t color_index = 0;

    for (const domain::Bus* bus : buses) {
        const svg::Color color = settings_.color_palette[color_index % settings_.color_palette.size()];

        const domain::Stop* first_stop = bus->stops.front();
        const svg::Point first_point = projector(first_stop->coordinates);

        doc.Add(MakeBusLabel(*bus, first_point, color, true));
        doc.Add(MakeBusLabel(*bus, first_point, color, false));

        if (!bus->is_roundtrip) {
            const domain::Stop* last_stop = bus->stops.back();
            if (last_stop != first_stop) {
                const svg::Point last_point = projector(last_stop->coordinates);
                doc.Add(MakeBusLabel(*bus, last_point, color, true));
                doc.Add(MakeBusLabel(*bus, last_point, color, false));
            }
        }

        ++color_index;
    }
}

void MapRenderer::RenderStopPoints(svg::Document& doc,
                                   const std::vector<const domain::Stop*>& stops,
                                   const SphereProjector& projector) const {
    for (const domain::Stop* stop : stops) {
        svg::Circle circle;
        circle.SetCenter(projector(stop->coordinates))
              .SetRadius(settings_.stop_radius)
              .SetFillColor(std::string("white"));
        doc.Add(circle);
    }
}

void MapRenderer::RenderStopLabels(svg::Document& doc,
                                   const std::vector<const domain::Stop*>& stops,
                                   const SphereProjector& projector) const {
    for (const domain::Stop* stop : stops) {
        const svg::Point point = projector(stop->coordinates);
        doc.Add(MakeStopLabel(*stop, point, true));
        doc.Add(MakeStopLabel(*stop, point, false));
    }
}

svg::Text MapRenderer::MakeBusLabel(const domain::Bus& bus,
                                    svg::Point position,
                                    svg::Color color,
                                    bool underlayer) const {
    svg::Text text;
    text.SetPosition(position)
        .SetOffset(settings_.bus_label_offset)
        .SetFontSize(static_cast<uint32_t>(settings_.bus_label_font_size))
        .SetFontFamily("Verdana")
        .SetFontWeight("bold")
        .SetData(bus.name);

    if (underlayer) {
        text.SetFillColor(settings_.underlayer_color)
            .SetStrokeColor(settings_.underlayer_color)
            .SetStrokeWidth(settings_.underlayer_width)
            .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
            .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
    } else {
        text.SetFillColor(color);
    }

    return text;
}

svg::Text MapRenderer::MakeStopLabel(const domain::Stop& stop,
                                     svg::Point position,
                                     bool underlayer) const {
    svg::Text text;
    text.SetPosition(position)
        .SetOffset(settings_.stop_label_offset)
        .SetFontSize(static_cast<uint32_t>(settings_.stop_label_font_size))
        .SetFontFamily("Verdana")
        .SetData(stop.name);

    if (underlayer) {
        text.SetFillColor(settings_.underlayer_color)
            .SetStrokeColor(settings_.underlayer_color)
            .SetStrokeWidth(settings_.underlayer_width)
            .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
            .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
    } else {
        text.SetFillColor(std::string("black"));
    }

    return text;
}

}  // namespace map_renderer