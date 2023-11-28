#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <fstream>

#include <libxml/HTMLparser.h>
#include <libxml/HTMLtree.h>
#include "httplib.h"

struct TimeBlock {
    std::string Label;
    std::chrono::system_clock::time_point Start;
    std::chrono::system_clock::time_point End;

    // Function to convert std::chrono::time_point to a formatted string
    std::string TimePointToString(const std::chrono::system_clock::time_point& timePoint) const {
        auto time = std::chrono::system_clock::to_time_t(timePoint);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }
};

struct Location {
    std::string Name;
    std::string StrHours;
    bool Favorite;
    bool Open;
    std::vector<TimeBlock> Hours;

    Location(const std::string& name, const std::vector<TimeBlock>& hours)
        : Name(name), Hours(hours), Favorite(false), Open(false) {
        // Initialize the 'Open' flag based on the current time
        auto now = std::chrono::system_clock::now();
        for (const auto& timeBlock : Hours) {
            if (timeBlock.Start <= now && now <= timeBlock.End) {
                Open = true;
                break;
            }
        }

        // Generate the formatted hours string
        for (size_t i = 0; i < Hours.size() - 1; ++i) {
            StrHours += Hours[i].Label.empty() ? "" : Hours[i].Label + ": ";
            StrHours += Hours[i].TimePointToString(Hours[i].Start) + " to " +
                        Hours[i].TimePointToString(Hours[i].End) + "\n";
        }
        StrHours += Hours.back().Label.empty() ? "" : Hours.back().Label + ": ";
        StrHours += Hours.back().TimePointToString(Hours.back().Start) + " to " +
                    Hours.back().TimePointToString(Hours.back().End);
    }
};

// Function to convert string to std::chrono::time_point
std::chrono::system_clock::time_point StringToTimePoint(const std::string& str) {
    std::tm tm = {};
    std::istringstream ss(str);
    ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
    return std::chrono::system_clock::from_time_t(std::mktime(&tm));
}

std::vector<Location> GetScheduleData(const std::string& date, bool debugMode) {
    std::vector<Location> locations;

    std::string html;
    if (debugMode) {
        // Use cached file for debugging
        std::ifstream file("locations.html");
        if (file.is_open()) {
            html = std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            file.close();
        } else {
            std::cerr << "Failed to open cached file." << std::endl;
            return locations;
        }
    } else {
        // Fetch data from Mizzou website
        httplib::Client cli("https://dining.missouri.edu");
        cli.enable_server_certificate_verification(false);

        std::string path = "/locations/?hoursForDate=" + date;
        auto res = cli.Get(path.c_str());
        if (res && res->status == 200) {
            html = res->body;
        } else {
            std::cerr << "Error fetching data from Mizzou website." << std::endl;
            return locations;
        }
    }

    // Parse HTML using libxml2
    xmlDoc* doc = htmlReadMemory(html.c_str(), html.size() + 1, NULL, NULL, 0);
    if (doc == NULL) {
        std::cerr << "Could not parse HTML." << std::endl;
        return locations;
    }

    // Find tables
    xmlNode* root_element = xmlDocGetRootElement(doc);
    for (xmlNode* table = root_element->children; table; table = table->next) {
        if (table->type == XML_ELEMENT_NODE && xmlStrEqual(table->name, BAD_CAST "table")) {
            // Process each row in the table
            for (xmlNode* row = table->children; row; row = row->next) {
                if (row->type == XML_ELEMENT_NODE && xmlStrEqual(row->name, BAD_CAST "tr")) {
                    std::string name;
                    std::vector<TimeBlock> hours;

                    // Process each column in the row
                    for (xmlNode* col = row->children; col; col = col->next) {
                        if (col->type == XML_ELEMENT_NODE && xmlStrEqual(col->name, BAD_CAST "td")) {
                            if (xmlChar* content = xmlNodeGetContent(col)) {
                                std::string value(reinterpret_cast<const char*>(content));
                                xmlFree(content);

                                if (col->children && xmlStrEqual(col->children->name, BAD_CAST "br")) {
                                    // Process line break in hoursData
                                    continue;
                                }

                                if (name.empty()) {
                                    // First column contains the name
                                    name = value;
                                } else {
                                    // Subsequent columns contain time blocks
                                    std::vector<std::string> timeData;
                                    size_t pos = 0;
                                    while ((pos = value.find(" - ", pos)) != std::string::npos) {
                                        timeData.push_back(value.substr(0, pos));
                                        pos += 3;
                                    }
                                    timeData.push_back(value.substr(pos));

                                    if (timeData.size() == 1) {
                                        // Single time block
                                        std::string label;
                                        hours.emplace_back(TimeBlock{label,
                                                                     StringToTimePoint(timeData[0]),
                                                                     StringToTimePoint(timeData[0])});
                                    } else {
                                        // Multiple time blocks
                                        for (size_t i = 0; i + 1 < timeData.size(); i += 2) {
                                            std::string label = timeData[i];
                                            hours.emplace_back(TimeBlock{label,
                                                                         StringToTimePoint(timeData[i + 1]),
                                                                         StringToTimePoint(timeData[i + 1])});
                                        }
                                    }
                                }
                            }
                        }
                    }

                    // Create Location object and add to the list
                    locations.emplace_back(name, hours);
                }
            }
        }
    }

    xmlFreeDoc(doc);
    xmlCleanupParser();

    return locations;
}

int main() {
    std::string date = "2023-11-20";  // Replace with the desired date
    bool debugMode = false;  // Set to true for debugging

    std::vector<Location> locations = GetScheduleData(date, debugMode);

    // Print information
    for (const auto& location : locations) {
        std::cout << "Name: " << location.Name << std::endl;
        std::cout << "Hours:\n" << location.StrHours << std::endl;
        std::cout << "Open: " << (location.Open ? "Yes" : "No") << std::endl;
        std::cout << "====================\n";
    }

    return 0;
}
