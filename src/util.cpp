#pragma once
#include <vector>
#include <string>
using namespace std;

vector<string> splitString(string s, string& delimiter) {
    vector<string> result;
    size_t pos = 0;
    std::string token;
    while ((pos = s.find(delimiter)) != string::npos) {
        token = s.substr(0, pos);
        result.push_back(token);
        s.erase(0, pos + delimiter.length());
    }
    result.push_back(s);
    return result;
}

vector<double> convertStringVector(vector<string>& stringVector) {
    vector<double> result;
    for(auto it = stringVector.begin(); it < stringVector.end(); ++it) {
        result.push_back(stod(*it));
    }
    return result;
}