#include "stat_reader.h"

#include <iomanip>

using namespace std;

namespace stat_reader {

void PrintBusInfo(const transport_catalogue::TransportCatalogue& transport_catalogue,
                  string_view bus_name,
                  ostream& output) {
    const auto info = transport_catalogue.GetBusInfo(bus_name);

    if (!info) {
        output << "Bus " << bus_name << ": not found" << endl;
    } else {
        output << "Bus " << bus_name << ": "
               << info->stops_count << " stops on route, "
               << info->unique_stops_count << " unique stops, "
               << info->route_length << " route length"
               << info->curvature << " curvature" << endl;
    }
}

void PrintStopInfo(const transport_catalogue::TransportCatalogue& transport_catalogue,
                   string_view stop_name,
                   ostream& output) {
    if (transport_catalogue.FindStop(stop_name) == nullptr) {
        output << "Stop " << stop_name << ": not found\n";
        return;
    }

    const auto& buses = transport_catalogue.GetBusesByStop(stop_name);

    if (buses.empty()) {
        output << "Stop " << stop_name << ": no buses\n";
    } else {
        output << "Stop " << stop_name << ": buses";
        for (std::string_view bus : buses) {
            output << ' ' << bus;
        }
        output << '\n';
    }
}

void ParseAndPrintStat(const transport_catalogue::TransportCatalogue& transport_catalogue,
                       string_view request,
                       ostream& output) {
    output << setprecision(6);

    const string_view bus_prefix = "Bus ";
    const string_view stop_prefix = "Stop ";

    if (request.substr(0, bus_prefix.size()) == bus_prefix) {
        PrintBusInfo(transport_catalogue, request.substr(bus_prefix.size()), output);
    } else if (request.substr(0, stop_prefix.size()) == stop_prefix) {
        PrintStopInfo(transport_catalogue, request.substr(stop_prefix.size()), output);
    }
}

}
