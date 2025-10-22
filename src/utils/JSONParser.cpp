#include "JSONParser.h"
#include <fstream>
using namespace std;
nlohmann::json JSONParser::parseFile(const string& path) {
    ifstream file(path);
    nlohmann::json j;
    if (!file.is_open()) {
        // return empty object rather than attempting to parse
        return nlohmann::json::object();
    }
    try {
        file >> j;
    } catch (const std::exception &e) {
        // return empty object on parse error to avoid exceptions bubbling up
        return nlohmann::json::object();
    }
    return j;
}
void JSONParser::writeFile(const string& path, const nlohmann::json& j) {
    ofstream file(path);
    file << j.dump(4);
}
