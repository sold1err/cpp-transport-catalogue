#include "stat_reader.h"

#include <iomanip>
#include <string>

using namespace std;

namespace stat_reader {

void ParseAndPrintStat(const transport_catalogue::TransportCatalogue& transport_catalogue,
                       string_view request,
                       ostream& output) {
    output << setprecision(6);

    const string_view bus_prefix = "Bus ";
    const string_view stop_prefix = "Stop ";

    if (request.substr(0, bus_prefix.size()) == bus_prefix) {
        const string_view bus_name = request.substr(bus_prefix.size());
        const auto info = transport_catalogue.GetBusInfo(bus_name);

        if (!info) {
            output << "Bus " << bus_name << ": not found\n";
        } else {
            output << "Bus " << bus_name << ": "
                   << info->stops_count << " stops on route, "
                   << info->unique_stops_count << " unique stops, "
                   << info->route_length << " route length\n";
        }
    } else if (request.substr(0, stop_prefix.size()) == stop_prefix) {
        const string_view stop_name = request.substr(stop_prefix.size());
        const auto buses = transport_catalogue.GetBusesByStop(stop_name);

        if (buses == nullptr) {
            output << "Stop " << stop_name << ": not found\n";
        } else if (buses->empty()) {
            output << "Stop " << stop_name << ": no buses\n";
        } else {
            output << "Stop " << stop_name << ": buses";
            for (string_view bus : *buses) {
                output << ' ' << bus;
            }
            output << '\n';
        }
    }
}

}
