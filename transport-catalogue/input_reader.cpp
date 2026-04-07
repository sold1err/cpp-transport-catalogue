#include "input_reader.h"

#include <string>

using namespace std;

namespace input_reader {

namespace {

struct StopDistanceDescription {
    string_view stop_name;
    int distance = 0;
};

string_view Trim(string_view str) {
    const size_t start = str.find_first_not_of(' ');
    if (start == str.npos) {
        return {};
    }
    const size_t end = str.find_last_not_of(' ');
    return str.substr(start, end - start + 1);
}

vector<string_view> Split(string_view str, string_view delim) {
    vector<string_view> result;

    while (true) {
        size_t pos = str.find(delim);
        if (pos == str.npos) {
            str = Trim(str);
            if (!str.empty()) {
                result.push_back(str);
            }
            break;
        }

        string_view part = Trim(str.substr(0, pos));
        if (!part.empty()) {
            result.push_back(part);
        }

        str.remove_prefix(pos + delim.size());
    }

    return result;
}

geo::Coordinates ParseCoordinates(string_view str) {
    str = Trim(str);

    const size_t first_comma = str.find(',');
    const size_t second_comma = str.find(',', first_comma + 1);

    const string_view lat_sv = Trim(str.substr(0, first_comma));
    const string_view lng_sv = Trim(
        second_comma == str.npos
            ? str.substr(first_comma + 1)
            : str.substr(first_comma + 1, second_comma - first_comma - 1)
        );

    return {
        stod(string(lat_sv)),
        stod(string(lng_sv))
    };
}

vector<string_view> ParseRoute(string_view route) {
    if (route.find(" > ") != route.npos) {
        return Split(route, " > ");
    }

    vector<string_view> stops = Split(route, " - ");
    vector<string_view> result = stops;

    for (int i = static_cast<int>(stops.size()) - 2; i >= 0; --i) {
        result.push_back(stops[i]);
    }

    return result;
}

vector<StopDistanceDescription> ParseStopDistances(string_view description) {
    vector<StopDistanceDescription> result;

    description = Trim(description);

    const size_t first_comma = description.find(',');
    if (first_comma == description.npos) {
        return result;
    }

    const size_t second_comma = description.find(',', first_comma + 1);
    if (second_comma == description.npos) {
        return result;
    }

    string_view tail = description.substr(second_comma + 1);

    for (string_view item : Split(tail, ",")) {
        item = Trim(item);

        const size_t m_pos = item.find("m to ");
        const int distance = stoi(string(Trim(item.substr(0, m_pos))));
        const string_view stop_name = Trim(item.substr(m_pos + 5));

        result.push_back({stop_name, distance});
    }

    return result;
}

CommandDescription ParseCommandDescription(string_view line) {
    const size_t colon_pos = line.find(':');
    if (colon_pos == line.npos) {
        return {};
    }

    const size_t space_pos = line.find(' ');
    if (space_pos == line.npos || space_pos >= colon_pos) {
        return {};
    }

    const string_view command = line.substr(0, space_pos);
    const string_view id = Trim(line.substr(space_pos + 1, colon_pos - space_pos - 1));
    const string_view description = line.substr(colon_pos + 1);

    return {string(command), string(id), string(description)};
}

}  // namespace

void InputReader::ParseLine(string_view line) {
    auto command_description = ParseCommandDescription(line);
    if (command_description) {
        commands_.push_back(move(command_description));
    }
}

void InputReader::ApplyCommands(transport_catalogue::TransportCatalogue& catalogue) const {
    for (const auto& command : commands_) {
        if (command.command == "Stop") {
            catalogue.AddStop(command.id, ParseCoordinates(command.description));
        }
    }

    for (const auto& command : commands_) {
        if (command.command == "Stop") {
            const auto* from = catalogue.FindStop(command.id);
            for (const auto& [stop_name, distance] : ParseStopDistances(command.description)) {
                const auto* to = catalogue.FindStop(stop_name);
                catalogue.SetDistanceBetweenStops(from, to, distance);
            }
        }
    }

    for (const auto& command : commands_) {
        if (command.command == "Bus") {
            catalogue.AddBus(command.id, ParseRoute(command.description));
        }
    }
}

}  // namespace input_reader
