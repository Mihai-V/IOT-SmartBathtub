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

string pipeStateToJson(PipeState state) {
    // Response to be sent
    string stateResponse = "{\"isOn\": " + to_string(state.isOn);
    if(state.isOn) {
        stateResponse += ", \"temperature\": " + to_string(state.temperature);
        stateResponse += ", \"debit\": " + to_string(state.debit);
    }
    stateResponse += "} ";
    return stateResponse;
}

string profileToJson(UserProfile profile) {
    string stateResponse = "{\"weight\": " + to_string(profile.weight);
    stateResponse += ", \"preferredBathTemperature\": " + to_string(profile.preferredBathTemperature);
    stateResponse += ", \"preferredShowerTemperature\": " + to_string(profile.preferredShowerTemperature);
    stateResponse += "} ";
    return stateResponse;
}