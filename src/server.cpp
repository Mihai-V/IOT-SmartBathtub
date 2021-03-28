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
    }

private:
    // JSON MimeType used in request responses
    Pistache::Http::Mime::MediaType JSON_MIME = MIME(Application, Json);

    void setupRoutes() {
        using namespace Rest;
        // Defining various endpoints
        Routes::Get(router, "/bath/state", Routes::bind(&BathEndpoint::getBathState, this));
        //Routes::Post(router, "/settings/:settingName/:value", Routes::bind(&BathEndpoint::setSetting, this));
        //Routes::Get(router, "/settings/:settingName/", Routes::bind(&BathEndpoint::getSetting, this));
    }

    
    void getBathState(const Rest::Request& request, Http::ResponseWriter response) {
        Guard guard(bathLock);

        auto state = bath->getBathState();
        auto m1 = MIME(Application, Json);
        // Response to be sent
        string stateResponse = "{\"isOn\": " + to_string(state.isOn);
        if(state.isOn) {
            stateResponse += ", \"temperature\": " + to_string(state.temperature);
            stateResponse += ", \"debit\": " + to_string(state.debit);
        }
        stateResponse += "} ";
        response.send(Http::Code::Ok, stateResponse, JSON_MIME);
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
    Port port(9080);

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