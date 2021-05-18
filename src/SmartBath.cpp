#include <thread>
#include "mqtt/client.h"
using namespace std;

// Maximum bath water debit measured in liters/secomd
#define MAX_BATH_DEBIT .25
// Maximum shower water debit measured in liters/secomd
#define MAX_SHOWER_DEBIT .2
// Drain speed of the water when the stopper is lifted, measured in liters/secomd
#define DRAIN_SPEED .2
// Minimum tempature of the water flowing through the pipe
#define MIN_WATER_TEMPERATURE 5
// Maximum tempature of the water flowing through the pipe
#define MAX_WATER_TEMPERATURE 50

const string SERVER_ADDRESS	{ "tcp://localhost:1883" };
const string CLIENT_ID		{ "smartbath" };

// State of shower/bath
typedef struct PipeState {
    // Specifies whether the pipe is on/off.
    bool isOn;
    // Specifiers the temperature of the water flowing through te pipe,
    // If the pipe is off, the expected value should be 0.
    double temperature;
    // Specifies the debit (how much water is flowing through the pipe).
    double debit;
} PipeState;

// Water quality details that are received from the sensor and stored inside the class.
typedef struct WaterQuality
{
    double pH; //6.5 - 8.5
    double chlorides; //250 - 400 mg/l
    double iron; //0.1 - 0.3 mg/l
    double calcium; //100 - 180 mg/l
    double color; //15 - 30
} WaterQuality;

// Singleton class 
class SmartBath {
private:
    // Water quality information
    WaterQuality waterQuality;
    // Struct variable storing the actual state of the shower
    PipeState showerState;
    // Struct variable storing the actual state of the bath
    PipeState bathState;
    // Volume of bathtub in liters
    const double bathtubValume = 300;
    // Current Volume of the bathtub in liters. It is calculated by the intervalCheck function
    double bathtubCurrentVolume = 0;
    // Bool variable representing the state of the bathtub stopper.
    // If it is unplugged (false), the water will drain. 
    bool isOnWaterStopper = true;

    double defaultTemperature = 20;
    
    // Varible to store the thread that is running the intervalCheck function
    // It is needed by the destructor to join at lifecycle end.
    std::thread checkThread;
    std::thread roomTemperatureThread;
    // Mutex to avoid concurrent reading/writing
    std::mutex blockingMutex;

    // Singleton instance
    static SmartBath* instance;
    // Private constructor
    SmartBath() {
        // Initialize states
        showerState = { .isOn = false, .temperature = 0, .debit = 0, };
        bathState = { .isOn = false, .temperature = 0, .debit = 0, };
        // Start the thread running intervalCheck and move its reference to the class member
        // intervalCheck function will be given the address of the instance pointer
        checkThread = std::move(std::thread(intervalCheck, &instance));
        roomTemperatureThread = std::move(std::thread(listenForDevices, &instance));
    }

    // Destructor of the SmartBath class
    ~SmartBath() {
        // Wait for the thread to join
        checkThread.join();
        sendStopCommand();
        roomTemperatureThread.join();
    }

    // Threaded function that checks every second if the water quality is good and the bathtub didn't fill up.
    static int intervalCheck(SmartBath** instance_ptr) {
        SmartBath* bath = *instance_ptr;
        // Check if lifecycle did end, and if so return
        if(bath == nullptr) {
            return 0;
        }
        // Sleep one second
        sleep(1);
        // Lock the mutex
        bath->blockingMutex.lock();
        // Get the current water debit from each pipe
        double currentDebit = bath->showerState.debit + bath->bathState.debit;

        double volume = bath->bathtubCurrentVolume + currentDebit;
        //std::cout << "Current volume: " << volume << " liters\n";
        // If the stopper is not plugged, then substract the water that has drained
        if(!bath->isOnWaterStopper) {
            volume -= DRAIN_SPEED;
            if(volume < 0) {
                volume = 0;
            }
        }
        // If the bathtub is filling up turn off the pipes
        if(volume >= bath->bathtubValume) {
            bath->bathtubCurrentVolume = bath->bathtubValume;
            // Turn off pipes
            bath->showerState = { .isOn = false, .temperature = 0, .debit = 0, };
            bath->bathState = { .isOn = false, .temperature = 0, .debit = 0, };
        } else {
            // else set the new volume
            bath->bathtubCurrentVolume = volume;
        }

        // TODO: Check water quality


        // Unlock the mutex
        bath->blockingMutex.unlock();

        // Call the function again
        return intervalCheck(instance_ptr);
    }
    static int listenForDevices(SmartBath** instance_ptr);
    static void sendStopCommand();

public:
    // Static method for getting the singleton instance.
    static SmartBath *getInstance() {
        if(instance == nullptr) {
            instance = new SmartBath();
        }
        return instance;
    }
    // Static method to destroy the instance.
    static void destroyInstance() {
        if(instance) {
            // Copy of the instance pointer. We need to set the instance pointer to null before calling the destructor,
            // so that intervalCheck will stop its execution and we can join.
            SmartBath* instanceAddress = instance;
            instance = nullptr;
            delete instanceAddress; // Calls the destructor
        }
    }

    // Method to set water quality. Returns true if set was successful, false otherwise.
    bool setWaterQuality(WaterQuality waterQuality) {
        blockingMutex.lock();
        this->waterQuality = waterQuality;
        blockingMutex.unlock();
        return true;
    }

    PipeState getBathState() {
        return bathState;
    }

    PipeState getShowerState() {
        return showerState;
    }

    // Set Bath State
    // Throws runtime_error
    void setBathState(PipeState state) {
        // Data validation
        if(state.isOn) { // State on
            // Exceeded debit
            if(state.debit > MAX_BATH_DEBIT) {
                throw std::runtime_error("MAXIMUM_DEBIT_EXCEEDED");
            }
            // Temerature NOT in interval
            if(!(MIN_WATER_TEMPERATURE <= state.temperature && state.temperature <= MAX_WATER_TEMPERATURE)) {
                throw std::runtime_error("TEMPERATURE_NOT_IN_RANGE");
            }
        } else { // State off
            if(state.temperature != 0 || state.debit != 0) {
                throw std::runtime_error("INVALID_DATA");
            }
        }
        
        // Change value if validation is successful
        blockingMutex.lock();
        bathState = state;
        blockingMutex.unlock();
    }

    // Set Shower State
    // Throws runtime_error
    void setShowerState(PipeState state) {
        // Data validation
        if(state.isOn) { // State on
            // Exceeded debit
            if(state.debit > MAX_SHOWER_DEBIT) {
                throw std::runtime_error("MAXIMUM_DEBIT_EXCEEDED");
            }
            // Temerature NOT in interval
            if(!(MIN_WATER_TEMPERATURE <= state.temperature && state.temperature <= MAX_WATER_TEMPERATURE)) {
                throw std::runtime_error("TEMPERATURE_NOT_IN_RANGE");
            }
        } else { // State off
            if(state.temperature != 0 || state.debit != 0) {
                throw std::runtime_error("INVALID_DATA");
            }
        }

        blockingMutex.lock();
        showerState = state;
        blockingMutex.unlock();
    }
};

SmartBath* SmartBath::instance = nullptr;

int SmartBath::listenForDevices(SmartBath** instance_ptr) {
    const vector<string> TOPICS { "temperature", "salt", "display", "command" };
    const vector<int> QOS { 0, 0, 0, 1 };

    SmartBath* bath = *instance_ptr;

    mqtt::client cli(SERVER_ADDRESS, CLIENT_ID);

	auto connOpts = mqtt::connect_options_builder()
		.clean_session(false)
		.finalize();

	try {
		cout << "Connecting to the MQTT server... " << flush;
		mqtt::connect_response rsp = cli.connect(connOpts);
		cout << "OK\n";

		if (!rsp.is_session_present()) {
			std::cout << "Subscribing to topics... " << std::flush;
			cli.subscribe(TOPICS, QOS);
			std::cout << "OK\n";
		}


		while (true) {
            auto msg = cli.consume_message();
            if(!msg) break;
            if(msg->get_topic() == string("temperature")) {
                bath->defaultTemperature = stod(msg->to_string());
            } else if(msg->get_topic() == string("command")) {
                if(msg->to_string() == string("stop")) {
                    break;
                }
            }
            cout << "[Received] " << msg->get_topic() << ": " << msg->to_string() << endl;
		}

		if (cli.is_connected()) {
			cli.disconnect();
		}
        return 0;
	} catch (const mqtt::exception& exc) {
		cerr << "\n  " << exc << endl;
		return 1;
	}
}

void SmartBath::sendStopCommand() {
    mqtt::client client(SERVER_ADDRESS, CLIENT_ID);

    mqtt::connect_options options;
    options.set_keep_alive_interval(20);
    options.set_clean_session(true);
    client.connect(options);
    const std::string TOPIC = "command";
    const std::string PAYLOAD = "stop";
    auto msg = mqtt::make_message(TOPIC, PAYLOAD);

    // Publish it to the server
    client.publish(msg);

    // Disconnect
    client.disconnect();
}


