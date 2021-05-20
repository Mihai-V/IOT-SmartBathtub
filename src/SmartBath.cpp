#include "SmartBath.hpp"
#include "util.cpp"
using namespace std;


SmartBath* SmartBath::instance = nullptr;

SmartBath::SmartBath() {
    // Initialize states
    showerState = { .isOn = false, .temperature = 0, .debit = 0, };
    bathState = { .isOn = false, .temperature = 0, .debit = 0, };
    // Start the thread running intervalCheck and move its reference to the class member
    // intervalCheck function will be given the address of the instance pointer
    checkThread = std::move(std::thread(intervalCheck, &instance));
    mqttThread = std::move(std::thread(listenForDevices, &instance));
}

SmartBath::~SmartBath() {
    // Wait for the thread to join
    checkThread.join();
    sendStopCommand();
    mqttThread.join();
}

int SmartBath::intervalCheck(SmartBath** instance_ptr) {
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
    std::cout << "Current volume: " << volume << " liters\n";
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

    if(bath->isSetWaterQuality && !SmartBath::checkWaterQuality(bath->waterQuality)) {
        // Turn off pipes
        bath->showerState = { .isOn = false, .temperature = 0, .debit = 0, };
        bath->bathState = { .isOn = false, .temperature = 0, .debit = 0, };
    }

    // Unlock the mutex
    bath->blockingMutex.unlock();

    // Call the function again
    return intervalCheck(instance_ptr);
}

SmartBath* SmartBath::getInstance() {
    if(instance == nullptr) {
        instance = new SmartBath();
    }
    return instance;
}

void SmartBath::destroyInstance() {
    if(instance) {
        // Copy of the instance pointer. We need to set the instance pointer to null before calling the destructor,
        // so that intervalCheck will stop its execution and we can join.
        SmartBath* instanceAddress = instance;
        instance = nullptr;
        delete instanceAddress; // Calls the destructor
    }
}

bool SmartBath::setWaterQuality(WaterQuality waterQuality) {
    blockingMutex.lock();
    this->waterQuality = waterQuality;
    this->isSetWaterQuality = true;
    blockingMutex.unlock();
    return true;
}

PipeState SmartBath::getBathState() {
    return bathState;
}

PipeState SmartBath::getShowerState() {
    return showerState;
}

void SmartBath::setBathState(PipeState state) {
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

void SmartBath::setShowerState(PipeState state) {
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

void SmartBath::setDefaultTemperature(double temperature) {
    if(temperature < MIN_WATER_TEMPERATURE) {
        temperature = MIN_WATER_TEMPERATURE;
    } else if(temperature > MAX_WATER_TEMPERATURE) {
        temperature = MAX_WATER_TEMPERATURE;
    }
    blockingMutex.lock();
    defaultTemperature = temperature;
    blockingMutex.unlock();
}

int SmartBath::listenForDevices(SmartBath** instance_ptr) {
    const vector<string> TOPICS { "temperature", "waterQuality", "salt", "display", "command" };
    const vector<int> QOS { 0, 0, 0, 0, 1 };

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
                bath->setDefaultTemperature(stod(msg->to_string()));
            } else if(msg->get_topic() == string("waterQuality")) {
                try {
                    string qualityString = msg->to_string();
                    string delimiter(",");
                    auto splitted = splitString(qualityString, delimiter);
                    auto result = convertStringVector(splitted);
                    WaterQuality waterQuality = {
                        .pH = result[0],
                        .chlorides = result[1],
                        .iron = result[2],
                        .calcium = result[3],
                        .color = result[4]
                    };
                    bath->setWaterQuality(waterQuality);
                } catch(char* err) { }

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

bool SmartBath::checkWaterQuality(WaterQuality waterQuality) {
    return (6.5 <= waterQuality.pH && waterQuality.pH <= 8.5)
        && (250.0 <= waterQuality.chlorides && waterQuality.chlorides <= 400.0)
        && (0.1 <= waterQuality.iron && waterQuality.iron <= 0.3)
        && (100.0 <= waterQuality.calcium && waterQuality.calcium <= 180.0)
        && (15.0 <= waterQuality.color && waterQuality.color <= 30.0);
}

