#pragma once

#include <iosfwd>
#include <string_view>

#include "transport_catalogue.h"

namespace stat_reader {

void PrintBusInfo(const transport_catalogue::TransportCatalogue& transport_catalogue,
                  std::string_view bus_name,
                  std::ostream& output);

void PrintStopInfo(const transport_catalogue::TransportCatalogue& transport_catalogue,
                   std::string_view stop_name,
                   std::ostream& output);

void ParseAndPrintStat(const transport_catalogue::TransportCatalogue& transport_catalogue,
                       std::string_view request,
                       std::ostream& output);

}
