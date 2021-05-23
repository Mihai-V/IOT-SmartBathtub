#include <algorithm>

#include <pistache/net.h>
#include <pistache/http.h>
#include <pistache/peer.h>
#include <pistache/http_headers.h>
#include <pistache/cookie.h>
#include <pistache/router.h>
#include <pistache/endpoint.h>
#include <pistache/common.h>
#include <signal.h>
#include "SmartBath.cpp"
#include "util.cpp"
#include "env.hpp"

using namespace std;
using namespace Pistache;

class BathEndpoint {
public:
    explicit BathEndpoint(Address addr)
        : httpEndpoint(std::make_shared<Http::Endpoint>(addr))
    { }

    // Initialization of the server. Additional options can be provided here
    void init(size_t thr = 2) {
        auto opts = Http::Endpoint::options()
            .threads(static_cast<int>(thr));
        httpEndpoint->init(opts);
        // Server routes are loaded up
        setupRoutes();
    }

    // Server is started threaded.  
    void start() {
        httpEndpoint->setHandler(router.handler());
        httpEndpoint->serveThreaded();
    }

    // When signaled server shuts down
    void stop(){
        httpEndpoint->shutdown();
        // Destroy the SmartBath instance to free up memory
        SmartBath::destroyInstance();
    }

private:
    // JSON MimeType used in request responses
    Pistache::Http::Mime::MediaType JSON_MIME = MIME(Application, Json);

    void setupRoutes() {
        using namespace Rest;
        // Defining various endpoints
        Routes::Get(router, "/volume", Routes::bind(&BathEndpoint::getCurrentVolume, this));
        Routes::Get(router, "/:pipe/state", Routes::bind(&BathEndpoint::getPipeState, this));
        Routes::Post(router, "/:pipe/off", Routes::bind(&BathEndpoint::setPipeStateOff, this));
        Routes::Post(router, "/:pipe/on", Routes::bind(&BathEndpoint::setPipeStateOn, this));
        Routes::Post(router, "/:pipe/on/:debit", Routes::bind(&BathEndpoint::setPipeStateOn, this));
        Routes::Post(router, "/:pipe/on/:debit/:temperature", Routes::bind(&BathEndpoint::setPipeStateOn, this));
        Routes::Post(router, "/stopper/:on", Routes::bind(&BathEndpoint::toggleStopper, this));
        Routes::Post(router, "/profiles/add/:name/:weight/:bathTemp/:showerTemp", Routes::bind(&BathEndpoint::addProfile, this));
        Routes::Post(router, "/profiles/edit/:name/:weight/:bathTemp/:showerTemp", Routes::bind(&BathEndpoint::editProfile, this));
        Routes::Post(router, "/profiles/remove/:name", Routes::bind(&BathEndpoint::removeProfile, this));
        Routes::Post(router, "/profiles/set/:name", Routes::bind(&BathEndpoint::setProfile, this));
        Routes::Get(router, "/profiles/get/:name", Routes::bind(&BathEndpoint::getProfile, this));
        Routes::Get(router, "/profiles/get-set", Routes::bind(&BathEndpoint::getProfileSet, this));
        Routes::Post(router, "/prepare", Routes::bind(&BathEndpoint::prepareBathForProfile, this));
        Routes::Post(router, "/prepare/:weight", Routes::bind(&BathEndpoint::prepareBath, this));
        Routes::Post(router, "/prepare/:weight/:temperature", Routes::bind(&BathEndpoint::prepareBath, this));
    }



    // Get the pipe state
    void getPipeState(const Rest::Request& request, Http::ResponseWriter response) {
        Guard guard(bathLock);
        auto pipe = request.param(":pipe").as<std::string>();

        PipeState state;
        if(pipe == "bath") {
            state = bath->getBathState();
        }
        else if(pipe == "shower") {
            state = bath->getShowerState();
        }
        else {
            // Return error if pipe is not known
            response.send(Http::Code::Bad_Request, "{\"error\": \"UNKNOWN_PIPE\"}", JSON_MIME);
            return;
        }

        // Response to be sent
        string stateResponse = pipeStateToJson(state);
        response.send(Http::Code::Ok, stateResponse, JSON_MIME);
    }

    void setPipeStateOn(const Rest::Request& request, Http::ResponseWriter response) {
        Guard guard(bathLock);
        string pipe = request.param(":pipe").as<std::string>();
        if(!(pipe == "bath" || pipe == "shower")) {
            // Return error if pipe is not known
            response.send(Http::Code::Bad_Request, "{\"error\": \"UNKNOWN_PIPE\"}", JSON_MIME);
            return;
        }

        double temperature, debit;

        // Try get temperature
        try {
            temperature = request.param(":temperature").as<double>();
        } catch(std::runtime_error err) {
            auto errorWhat = err.what();
            if(strcmp(errorWhat, "Unknown parameter") == 0) { // If temperature is not set, set a default temperature
                temperature = bath->getDefaultTemperature();
            } else { // If there is another error, send Bad Request response
                response.send(Http::Code::Bad_Request, "{\"error\": \"BAD_TEMPERATURE_FORMAT\"}", JSON_MIME);
                return;
            }
        }

        // Try get debit
        try {
            debit = request.param(":debit").as<double>();
        } catch(std::runtime_error err) {
            auto errorWhat = err.what();
            if(strcmp(errorWhat, "Unknown parameter") == 0) { // If debit is not set, set a default debit
                debit = 0.2;
            } else { // If there is another error, send Bad Request response
                response.send(Http::Code::Bad_Request, "{\"error\": \"BAD_DEBIT_FORMAT\"}", JSON_MIME);
                return;
            }
        }

        // Everything is OK from now on
        PipeState state = { .isOn = true, .temperature = temperature, .debit = debit };
        try {
            if(pipe == "bath") {
                bath->setBathState(state);
            }
            else if(pipe == "shower") {
                bath->setShowerState(state);
            }
        } catch(runtime_error err) {
            auto errWhat = string(err.what());
            response.send(Http::Code::Bad_Request, "{\"error\": \"" + errWhat + "\"}", JSON_MIME);
            return;
        }

        string stateResponse = pipeStateToJson(state);
        response.send(Http::Code::Ok, stateResponse, JSON_MIME);
    }

    // Turn off the pipe
    void setPipeStateOff(const Rest::Request& request, Http::ResponseWriter response) {
        Guard guard(bathLock);
        string pipe = request.param(":pipe").as<std::string>();
        PipeState state = { .isOn = false, .temperature = 0, .debit = 0 };
        try {
            if(pipe == "bath") {
                bath->setBathState(state);
            }
            else if(pipe == "shower") {
                bath->setShowerState(state);
            }
        } catch(runtime_error err) {
            auto errWhat = string(err.what());
            response.send(Http::Code::Bad_Request, "{\"error\": \"" + errWhat + "\"}", JSON_MIME);
            return;
        }
        
        response.send(Http::Code::Ok);
    }

    void getCurrentVolume(const Rest::Request& request, Http::ResponseWriter response) {
        double volume = bath->getBathtubCurrentVolume();
        response.send(Http::Code::Ok, "{\"currentVolume\": " + to_string(volume) + "}", JSON_MIME);
    }

    void toggleStopper(const Rest::Request& request, Http::ResponseWriter response) {
        string on = request.param(":on").as<std::string>();
        bool onBool;
        if(on == "on") {
            onBool = true;
        } else if(on == "off") {
            onBool = false;
        } else {
            response.send(Http::Code::Bad_Request);
            return;
        }
        bath->toggleStopper(onBool);
        response.send(Http::Code::Ok, "{\"stopper\": " + to_string(onBool) + "}", JSON_MIME);
    }

    void addProfile(const Rest::Request& request, Http::ResponseWriter response) {
        string name;
        double weight, bathTemp, showerTemp;
        try {
            name = request.param(":name").as<std::string>();
            weight = stod(request.param(":weight").as<std::string>());
            bathTemp = stod(request.param(":bathTemp").as<std::string>());
            showerTemp = stod(request.param(":showerTemp").as<std::string>());
        } catch(...) {
            response.send(Http::Code::Bad_Request);
        }
        try {
            UserProfile profile = {
                .weight = weight,
                .preferredBathTemperature = bathTemp,
                .preferredShowerTemperature = showerTemp
            };
            bath->addProfile(name, profile);
            auto resp = profileToJson(profile);
            response.send(Http::Code::Ok, resp, JSON_MIME);
        } catch(runtime_error err) {
            auto errWhat = string(err.what());
            response.send(Http::Code::Bad_Request, "{\"error\": \"" + errWhat + "\"}", JSON_MIME);
        }
    }

    void editProfile(const Rest::Request& request, Http::ResponseWriter response) {
        string name;
        double weight, bathTemp, showerTemp;
        try {
            name = request.param(":name").as<std::string>();
            weight = stod(request.param(":weight").as<std::string>());
            bathTemp = stod(request.param(":bathTemp").as<std::string>());
            showerTemp = stod(request.param(":showerTemp").as<std::string>());
        } catch(...) {
            response.send(Http::Code::Bad_Request);
        }
        try {
            UserProfile profile = {
                .weight = weight,
                .preferredBathTemperature = bathTemp,
                .preferredShowerTemperature = showerTemp
            };
            bath->editProfile(name, profile);
            auto resp = profileToJson(profile);
            response.send(Http::Code::Ok, resp, JSON_MIME);
        } catch(runtime_error err) {
            auto errWhat = string(err.what());
            response.send(Http::Code::Bad_Request, "{\"error\": \"" + errWhat + "\"}", JSON_MIME);
        }
    }

    void removeProfile(const Rest::Request& request, Http::ResponseWriter response) {
        string name = request.param(":name").as<std::string>();
        try {
            bath->removeProfile(name);
            response.send(Http::Code::Ok, "{\"success\": true }", JSON_MIME);
        } catch(runtime_error err) {
            auto errWhat = string(err.what());
            response.send(Http::Code::Bad_Request, "{\"error\": \"" + errWhat + "\"}", JSON_MIME);
        }
    }

    void setProfile(const Rest::Request& request, Http::ResponseWriter response) {
        string name = request.param(":name").as<std::string>();
        try {
            bath->setProfile(name);
            response.send(Http::Code::Ok, "{\"success\": true }", JSON_MIME);
        } catch(runtime_error err) {
            auto errWhat = string(err.what());
            response.send(Http::Code::Bad_Request, "{\"error\": \"" + errWhat + "\"}", JSON_MIME);
        }
    }

    void getProfile(const Rest::Request& request, Http::ResponseWriter response) {
        string name = request.param(":name").as<std::string>();
        try {
            auto profile = bath->getProfile(name);
            response.send(Http::Code::Ok, profileToJson(profile), JSON_MIME);
        } catch(runtime_error err) {
            auto errWhat = string(err.what());
            response.send(Http::Code::Bad_Request, "{\"error\": \"" + errWhat + "\"}", JSON_MIME);
        }
    }

    void getProfileSet(const Rest::Request& request, Http::ResponseWriter response) {
        auto profile = bath->getProfileSet();
        if(profile == nullptr) {
            response.send(Http::Code::Ok, "null", JSON_MIME);
        } else {
            response.send(Http::Code::Ok, profileToJson(*profile), JSON_MIME);
        }
    }

    void prepareBathForProfile(const Rest::Request& request, Http::ResponseWriter response) {
        try {
            int seconds = bath->prepareBath();
            response.send(Http::Code::Ok, "{\"readyAfter\": " + to_string(seconds) + " }", JSON_MIME);
        } catch(runtime_error err) {
            auto errWhat = string(err.what());
            response.send(Http::Code::Bad_Request, "{\"error\": \"" + errWhat + "\"}", JSON_MIME);
        }
    }

    void prepareBath(const Rest::Request& request, Http::ResponseWriter response) {
        double weight = request.param(":weight").as<double>();
        double temperature;

        // Try get temperature
        try {
            temperature = request.param(":temperature").as<double>();
        } catch(std::runtime_error err) {
            auto errorWhat = err.what();
            if(strcmp(errorWhat, "Unknown parameter") == 0) { // If temperature is not set, set a default temperature
                temperature = bath->getDefaultTemperature();
            } else { // If there is another error, send Bad Request response
                response.send(Http::Code::Bad_Request, "{\"error\": \"BAD_TEMPERATURE_FORMAT\"}", JSON_MIME);
                return;
            }
        }

        try {
            int seconds = bath->prepareBath(weight, temperature);
            response.send(Http::Code::Ok, "{\"readyAfter\": " + to_string(seconds) + " }", JSON_MIME);
        } catch(runtime_error err) {
            auto errWhat = string(err.what());
            response.send(Http::Code::Bad_Request, "{\"error\": \"" + errWhat + "\"}", JSON_MIME);
        }
    }

    // Create the lock which prevents concurrent editing of the same variable
    using Lock = std::mutex;
    using Guard = std::lock_guard<Lock>;
    Lock bathLock;

    // Instance of the SmartBath model
    SmartBath* bath = SmartBath::getInstance();

    // Defining the httpEndpoint and a router.
    std::shared_ptr<Http::Endpoint> httpEndpoint;
    Rest::Router router;
};


int main(int argc, char *argv[]) {

    // This code is needed for gracefull shutdown of the server when no longer needed.
    sigset_t signals;
    if (sigemptyset(&signals) != 0
            || sigaddset(&signals, SIGTERM) != 0
            || sigaddset(&signals, SIGINT) != 0
            || sigaddset(&signals, SIGHUP) != 0
            || pthread_sigmask(SIG_BLOCK, &signals, nullptr) != 0) {
        perror("install signal handler failed");
        return 1;
    }

    // Set a port on which your server to communicate
    Port port(HTTP_ENDPOINT_PORT);

    // Number of threads used by the server
    int thr = 2;

    if (argc >= 2) {
        port = static_cast<uint16_t>(std::stol(argv[1]));

        if (argc == 3)
            thr = std::stoi(argv[2]);
    }

    Address addr(Ipv4::any(), port);

    cout << "Cores = " << hardware_concurrency() << endl;
    cout << "Using " << thr << " threads" << endl;

    // Instance of the class that defines what the server can do.
    BathEndpoint stats(addr);

    // Initialize and start the server
    stats.init(thr);
    stats.start();


    // Code that waits for the shutdown sinal for the server
    int signal = 0;
    int status = sigwait(&signals, &signal);
    if (status == 0)
    {
        std::cout << "received signal " << signal << std::endl;
    }
    else
    {
        std::cerr << "sigwait returns " << status << std::endl;
    }

    stats.stop();
}