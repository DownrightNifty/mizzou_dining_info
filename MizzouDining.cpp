#include <string>
#include <iostream>
#include <vector>
#include <cctype>
#include <ctime>
#include <fstream>

#include <libxml/HTMLparser.h>
#include <libxml/HTMLtree.h>
#include "httplib.h"

#define D_MODE true // use the cached HTML file instead of live data
#define D_TIME false // use a fake value for current time instead of the real time
#define D_TIME_VAL "1:00 PM"

// convert time str (relative to today's date) to an int for comparison
// the int is simply the number of minutes since the start of the day
int timeStrToInt(std::string timeStr) {
    // timeStr is like "6:30 AM"

    // std::size_t pos = timeStr.find(":");
    int pos = timeStr.find(":");
    int hr12 = stoi(timeStr.substr(0, pos));
    int min = stoi(timeStr.substr(pos + 1, 2));

    // first convert to 24hr
    int hr24;
    if (timeStr.substr(timeStr.size() - 2, 2) == "AM") {
        if (hr12 == 12) {
            hr24 = 0;
        }
        else {
            hr24 = hr12;
        }
    }
    else {
        if (hr12 == 12) {
            hr24 = 12;
        }
        else {
            hr24 = hr12 + 12;
        }
    }

    // finally, convert entire time to a single int
    return min + hr24*60;
}

std::string intToTimeStr(int timeInt) {
    int hr24 = timeInt / 60;
    int min = timeInt % 60;

    int hr12;
    bool isAM;
    if (hr24 == 0) { hr12 = 12; isAM = true; }
    else if (hr24 == 12) { hr12 = 12; isAM = false; }
    else if (hr24 < 12) {
        hr12 = hr24; isAM = true;
    }
    else { // hr24 >= 13
        hr12 = hr24 - 12; isAM = false;
    }

    std::string pStr;
    if (isAM) { pStr = " AM"; }
    else { pStr = " PM"; }
    std::string minStr;
    if (min < 10) { minStr = "0" + std::to_string(min); }
    else { minStr = std::to_string(min); }
    std::string hr12Str = std::to_string(hr12);
    return hr12Str + ":" + minStr + pStr;
}

struct TimeBlock {
    std::string label;
    int start;
    int end;

    TimeBlock(std::string _label, int _start, int _end)
        : label(_label), start(_start), end(_end) {
        // std::cout << "creating TimeBlock from " << start << " to " << end << "\n";
    }
};

// Define the HardCodedLocation struct
struct HardCodedLocation {
    std::string name;
    double latitude;
    double longitude;
};

struct Location {
    std::string name;
    // note that 0.0 is a valid value for these if a corresponding HardCodedLocation wasn't found
    double latitude;
    double longitude;
    std::string strHours;
    bool favorite;
    bool open;
    std::vector<TimeBlock> hours;

    Location(const std::string& _name, double _latitude, double _longitude, const std::vector<TimeBlock>& _hours)
        : name(_name), latitude(_latitude), longitude(_longitude), hours(_hours), favorite(false), open(false) {
        // create a nice string representation of the data
        strHours = "";
        for (TimeBlock& tb : hours) {
            strHours += tb.label + ": " + intToTimeStr(tb.start) + " to " + intToTimeStr(tb.end) + "\n";
        }
    }

    // check if this location is open by comparing the current time with the scheduled hours
    // (it only makes sense to call this if the hours are for the current date)
    void checkIfOpen() {
        int nowInt;
        if (!D_TIME) {
            time_t t = time(NULL);
            struct tm* tmPtr = localtime(&t);
            nowInt = tmPtr->tm_min + tmPtr->tm_hour*60;
        }
        else {
            nowInt = timeStrToInt(D_TIME_VAL);
        }

        for (TimeBlock& timeBlock : hours) {
            if (timeBlock.start <= nowInt && nowInt <= timeBlock.end) {
                open = true;
                break;
            }
        }
    }
};

// hrsStr may have multiple blocks and look like this:
// "[spaces]Lunch[spaces]11:00 AM - 2:00 PM[spaces]Dinner[spaces]4:00 PM - 8:00 PM[spaces]"
// OR just one:
// "[spaces]11:00 AM - 2:00 PM[spaces]"
std::vector<TimeBlock> parseHrsStr(std::string hrsStr) {
    std::vector<TimeBlock> timeBlocks;

    // trim hrsStr
    int beginIdx = 0;
    while (std::isspace(hrsStr[beginIdx])) { beginIdx++; }
    int endIdx = hrsStr.size() - 4;
    while (std::isspace(hrsStr[endIdx])) { endIdx--; }
    hrsStr = hrsStr.substr(beginIdx, endIdx - beginIdx + 1);

    // now it looks like: "Lunch[spaces]11:00 AM - 2:00 PM[spaces]Dinner[spaces]4:00 PM - 8:00 PM"
    // or: "11:00 AM - 2:00 PM"
    // std::cout << "`" << hrsStr << "`\n";

    // HACK: should use std::string::find(str, pos) instead
    // a view into a part of hrsStr, starting with the whole thing and shrinking from the left
    std::string hrsStrView = hrsStr; int pos = 0;

    // HACK: if the first char is a number, we have form #2
    if (std::isdigit(hrsStr[0])) {
        pos = hrsStrView.find(" ");
        std::string startTimeStr = hrsStrView.substr(0, pos);
        hrsStrView = hrsStrView.substr(pos + 1, hrsStrView.size() - 1);
        pos = hrsStrView.find(" ");
        std::string startTimePStr = hrsStrView.substr(0, pos);
        hrsStrView = hrsStrView.substr(pos + 1, hrsStrView.size() - 1);
        pos = hrsStrView.find(" ");
        hrsStrView = hrsStrView.substr(pos + 1, hrsStrView.size() - 1);
        pos = hrsStrView.find(" ");
        std::string endTimeStr = hrsStrView.substr(0, pos);
        hrsStrView = hrsStrView.substr(pos + 1, hrsStrView.size() - 1);
        std::string endTimePStr = hrsStrView;
        std::string startStr = startTimeStr + " " + startTimePStr;
        std::string endStr = endTimeStr + " " + endTimePStr;
        timeBlocks.push_back(TimeBlock{"Hours", timeStrToInt(startStr), timeStrToInt(endStr)});
        return timeBlocks;
    }

    while (true) {
        pos = hrsStrView.find(" "); // the first of many
        std::string label = hrsStrView.substr(0, pos);
        // std::cout << label << "\n";

        while (std::isspace(hrsStrView[pos])) { pos++; }
        hrsStrView = hrsStrView.substr(pos, hrsStrView.size() - 1);
        // std::cout << "`" << hrsStrView << "`\n";

        pos = hrsStrView.find(" ");
        std::string startTimeStr = hrsStrView.substr(0, pos);
        // std::cout << startTimeStr << "\n";

        hrsStrView = hrsStrView.substr(pos + 1, hrsStrView.size() - 1);
        // std::cout << "`" << hrsStrView << "`\n";

        pos = hrsStrView.find(" ");
        std::string startTimePStr = hrsStrView.substr(0, pos);
        // std::cout << startTimePStr << "\n";

        hrsStrView = hrsStrView.substr(pos + 1, hrsStrView.size() - 1);
        // std::cout << "`" << hrsStrView << "`\n";

        pos = hrsStrView.find(" ");

        hrsStrView = hrsStrView.substr(pos + 1, hrsStrView.size() - 1);
        // std::cout << "`" << hrsStrView << "`\n";

        pos = hrsStrView.find(" ");
        std::string endTimeStr = hrsStrView.substr(0, pos);
        // std::cout << endTimeStr << "\n";

        hrsStrView = hrsStrView.substr(pos + 1, hrsStrView.size() - 1);
        // std::cout << "`" << hrsStrView << "`\n";

        std::string endTimePStr;
        pos = hrsStrView.find(" ");
        bool done = false;
        if (pos == -1) {
            endTimePStr = hrsStrView;
            done = true;
        }

        if (!done) { endTimePStr = hrsStrView.substr(0, pos); }
        // std::cout << "endTimePStr: " << endTimePStr << "\n";

        std::string startStr = startTimeStr + " " + startTimePStr;
        std::string endStr = endTimeStr + " " + endTimePStr;
        timeBlocks.push_back(TimeBlock{label, timeStrToInt(startStr), timeStrToInt(endStr)});

        if (done) { break; }

        hrsStrView = hrsStrView.substr(pos + 1, hrsStrView.size() - 1);
        // std::cout << "`" << hrsStrView << "`\n";
        pos = 0;

        // there's a lot of spaces now so we skip over them
        while (std::isspace(hrsStrView[pos])) { pos++; }
        hrsStrView = hrsStrView.substr(pos, hrsStrView.size() - 1);
        // std::cout << "`" << hrsStrView << "`\n";
    }

    return timeBlocks;
}

// NOTE: `xmlNode->children` might as well have been named `xmlNode->firstChild`, because that's what it is
// it can be treated as a linked list of children using `xmlNode->children->next`

// searches for a child (or sibling) element of node with the provided string "name"
void _getElementsByTagName(xmlNode* node, std::string name, std::vector<xmlNode*>& results) {
    xmlNode* currNode = NULL;
    for (currNode = node; currNode; currNode = currNode->next) {
        if (currNode->type == XML_ELEMENT_NODE && std::string((const char*)currNode->name) == name) {
            results.push_back(currNode);
        }
        _getElementsByTagName(currNode->children, name, results);
    }
}

// searches for a child element of node with the provided string "name"
// results vector must be allocated by the caller
void getElementsByTagName(xmlNode* node, std::string name, std::vector<xmlNode*>& results) {
    _getElementsByTagName(node->children, name, results);
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
    // TODO: error checking for malformed HTML document from server
    xmlDoc* doc = htmlReadMemory(html.c_str(), html.size() + 1, NULL, NULL, HTML_PARSE_NOERROR);
    if (doc == NULL) {
        std::cerr << "Could not parse HTML." << std::endl;
        return locations;
    }

    xmlNode* root = xmlDocGetRootElement(doc);
    std::vector<xmlNode*> results;
    getElementsByTagName(root, "tr", results);
    // std::cout << results.size() << " results:\n";
    for (xmlNode* node : results) {
        // std::cout << "found " << node->name << " with content: \n";
        std::vector<xmlNode*> tdResults;
        getElementsByTagName(node, "td", tdResults);
        // there are always two result nodes, unless we're on a header row
        if (tdResults.size() == 2) {
            std::string locName;
            std::string hrsStr;
            for (int i = 0; i < 2; i++) {
                xmlNode* tdElem = tdResults[i];
                if (i == 0) {
                    // the first column is the name of the location, but it's nested in an <a> element
                    // there's actually multiple children of the <td>, including raw text nodes
                    // the <a> is the second child
                    xmlNode* aElem = tdElem->children->next;
                    xmlChar* key = xmlNodeListGetString(doc, aElem->children, 1);
                    locName = (const char*)key;
                    xmlFree(key);
                }
                else {
                    // the second column is the hours
                    xmlChar* key = xmlNodeListGetString(doc, tdElem->children, 1);
                    hrsStr = (const char*)key;
                    xmlFree(key);
                }
            }

            std::vector<TimeBlock> timeBlocks = parseHrsStr(hrsStr);
            Location l{locName, 0.0, 0.0, timeBlocks};
            // initialize the 'open' flag based on the current time
            l.checkIfOpen();
            locations.push_back(l);
        }
        // std::cout << "\n\n";
    }

    xmlFreeDoc(doc);
    xmlCleanupParser();

    return locations;
}

// returns 0 on fail, 1 on success
int searchByName(const std::vector<HardCodedLocation>& hcls, std::string name, HardCodedLocation* out) {
    for (auto& hcl : hcls) {
        if (hcl.name == name) {
            *out = hcl;
            return 1;
        }
    }
    return 0;
}

// chop off n chars from end of s
void rchop(std::string& s, int n) {
    s.erase(s.size() - n, n);
}

// THIS IS ONLY USED IN THE ANDROID VERSION (for passing C++ data to Java)
//
// output looks like:
// [location]||||[location]||||...
//
// [location] looks like:
// Baja Grill|||38.943203153879246|||-92.3267064269865|||[strHours]|||0|||1|||[timeBlocks]
//
// [timeBlocks] looks like:
// Lunch|100|200||Dinner|300|400||Late-night|500|600||...
std::string serializeLocations(const std::vector<Location>& locations) {
    std::string out;
    
    // extract each Location

    for (const Location& l : locations) {
        std::string serializedLoc;

        // extract each Location parameter

        // for (int i = 0; i < 7; i++) {
        //     
        //     serializedLoc += "|||";
        // }
        {
            serializedLoc += l.name + "|||";
            serializedLoc += std::to_string(l.latitude) + "|||";
            serializedLoc += std::to_string(l.longitude) + "|||";
            serializedLoc += l.strHours + "|||";
            serializedLoc += std::to_string(l.favorite) + "|||";
            serializedLoc += std::to_string(l.open) + "|||";

            // extract each TimeBlock
            for (const TimeBlock& tb : l.hours) {
                // extract each TimeBlock parameter
                {
                    serializedLoc += tb.label + "|";
                    serializedLoc += std::to_string(tb.start) + "|";
                    serializedLoc += std::to_string(tb.end);
                    // at the end, don't add a final "|"
                }

                serializedLoc += "||";
            }
            rchop(serializedLoc, 2);

        }
        // rchop(out, 3); // chop off trailing delim

        out += serializedLoc + "||||";
    }
    rchop(out, 4); // chop off trailing delim

    return out;
}

int main() {
    // Hardcode GPS coordinates for Mizzou dining locations
    std::vector<HardCodedLocation> hardcodedLocations = {
        {"Baja Grill", 38.943203153879246, -92.3267064269865},
        {"Bookmark Café", 38.94444846006983, -92.32643268762787},
        {"Do Mundo's", 38.94253788826768, -92.32686662995677},
        {"Emporium Café", 38.94107459217775, -92.32304902570891},
        {"infusion", 38.9431030190833, -92.32550479820374},
        {"Legacy Grill", 38.93918339225931, -92.33160118208086},
        {"Mort's", 38.9430322404151, -92.32697566673839},
        {"Plaza 900 Dining", 38.94103367843988, -92.322668187628},
        {"Potential Energy Café", 38.94623123277513, -92.32956715694311},
        {"Pizza & MO", 38.94180412624759, -92.32309229325335},
        {"Wings & MO", 38.94180412624759, -92.32309229325335},
        {"Sabai", 38.94212344437987, -92.32450920297028},
        {"Starbucks - Memorial Union", 38.94544954634613, -92.32511273180572},
        {"Starbucks - Southwest", 38.93916939101163, -92.33207097791104},
        {"Subway - Southwest", 38.93916939101163, -92.33207097791104},
        {"Sunshine Sushi", 38.9425837776428, -92.32676330297024},
        {"The MARK on 5th Street", 38.94533889956199, -92.33233735694316},
        {"Truffles", 38.93916939101163, -92.33206024907547},
        {"Wheatstone Bistro", 38.94548111216589, -92.3250477606413},
        {"Panda Express", 38.94278414251326, -92.32674715879237},
        {"The Restaurants at Southwest", 38.93912766582345, -92.33210316441776},
        {"Mizzou Market Central", 38.94254132210812, -92.32717908392979},
        {"Mizzou Market - Southwest", 38.93921968905726, -92.33280040112133},
    };
    std::string date = "2023-12-04";  // Replace with the desired date

    std::vector<Location> locations = GetScheduleData(date, D_MODE);

    // std::vector<TimeBlock> testTimeBlocks = parseHrsStr("10:00 AM - 3:00 PM");
    // Location testLoc{"Test", 0.0, 0.0, testTimeBlocks};
    // testLoc.checkIfOpen();
    // locations.push_back(testLoc);

    // Match locations with corresponding hardcoded coordinates

    // for loop without `&` is similar to function in that it makes a copy of everything by default
    // for (Location l : locations) {
    for (Location& l : locations) {
        HardCodedLocation hcl;
        int ret = searchByName(hardcodedLocations, l.name, &hcl);
        if (ret != 0) {
            l.latitude = hcl.latitude;
            l.longitude = hcl.longitude;
        }
    }

    // for (size_t i = 0; i < std::min(locations.size(), hardcodedLocations.size()); ++i) {
    //     locations[i].latitude = hardcodedLocations[i].latitude;
    //     locations[i].longitude = hardcodedLocations[i].longitude;
    // }

    std::cout << "Successfully parsed\n";
    std::cout << "List of locations:\n";
    std::cout << "====================\n";
    // Print information
    if (locations.empty()) {
        std::cout << "Locations vector is empty" << std::endl;
    }
    else {
        for (const auto& location : locations) {
            std::cout << "Name: " << location.name << std::endl;
            std::cout << location.strHours;
            std::cout << "Open: " << (location.open ? "Yes" : "No") << std::endl;
            std::cout << "GPS Coordinates: " << location.latitude << ", " << location.longitude << std::endl;
            std::cout << "====================\n";
        }
    }

    // std::cout << serializeLocations(locations) << "\n";

    return 0;
}
