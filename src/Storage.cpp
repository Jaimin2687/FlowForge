#include "Storage.h"
#include "utils/JSONParser.h"
using namespace std;
void Storage::saveState(const nlohmann::json& state, const string& path) {
    JSONParser::writeFile(path, state);
}
nlohmann::json Storage::loadState(const string& path) {
    return JSONParser::parseFile(path);
}
