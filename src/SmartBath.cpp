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
    mqttThread.join();
}

int SmartBath::intervalCheck(SmartBath** instance_ptr) {
    // Sleep one second
    sleep(1);
    SmartBath* bath = *instance_ptr;
    // Check if lifecycle did end, and if so return
    if(bath == nullptr) {
        return 0;
    }
    // Lock the mutex
    bath->blockingMutex.lock();
    // Get the current water debit from each pipe
    double currentDebit = bath->showerState.debit + bath->bathState.debit;

    double volume = bath->bathtubCurrentVolume + currentDebit;
    // If the stopper is not plugged, then substract the water that has drained
    if(!bath->isOnWaterStopper) {
        volume -= DRAIN_SPEED;
        if(volume < 0) {
            volume = 0;
        }
    }
    // If the bathtub is filling up turn off the pipes
    if(volume >= bath->bathtubValume && (bath->bathState.isOn || bath->showerState.isOn)) {
        bath->bathtubCurrentVolume = bath->bathtubValume;
        // Turn off pipes
        PipeState state = { .isOn = false, .temperature = 0, .debit = 0 };
        bath->_setShowerState(state);
        bath->_setBathState(state);
    } else {
        // else set the new volume
        bath->bathtubCurrentVolume = volume;
    }

    // Inform volume value over MQTT
    bath->sendMessage("display", "currentVolume/" + to_string(volume));

    if(bath->isSetWaterQuality && !SmartBath::checkWaterQuality(bath->waterQuality)) {
        PipeState state = { .isOn = false, .temperature = 0, .debit = 0 };
        bath->_setShowerState(state);
        bath->_setBathState(state);
    }

    // If target is reached turn off pipes
    if(bath->isFillTargetSet && bath->fillTarget <= bath->bathtubCurrentVolume) {
        PipeState state = { .isOn = false, .temperature = 0, .debit = 0 };
        bath->_setShowerState(state);
        bath->_setBathState(state);
        bath->isFillTargetSet = false;
        // Notify with MQTT
        auto msg = mqtt::make_message("display", "targetReached");
        bath->mqtt_client->publish(msg);
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
        sendStopCommand(instanceAddress);
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

void SmartBath::_setBathState(PipeState state, bool lockMutex) {
    string msg = "pipe/bath/";
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
        msg += "on/" + to_string(state.debit) + "/" + to_string(state.temperature);
    } else { // State off
        if(state.temperature != 0 || state.debit != 0) {
            throw std::runtime_error("INVALID_DATA");
        }
        isFillTargetSet = false; // Cancel filling target
        msg += "off";
    }
    
    if(lockMutex) {
        blockingMutex.lock();
    }
    // Change value if validation is successful
    bathState = state;
    if(lockMutex) {
        blockingMutex.unlock();
    }
    sendMessage("display", msg);
}

void SmartBath::setBathState(PipeState state) {
    _setBathState(state, true);
}

void SmartBath::_setShowerState(PipeState state, bool lockMutex) {
    string msg = "pipe/shower/";
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
        msg += "on/" + to_string(state.debit) + "/" + to_string(state.temperature);
    } else { // State off
        if(state.temperature != 0 || state.debit != 0) {
            throw std::runtime_error("INVALID_DATA");
        }
        msg += "off";
    }

    if(lockMutex) {
        blockingMutex.lock();
    }
    showerState = state;
    if(lockMutex) {
        blockingMutex.unlock();
    }
    sendMessage("display", msg);
}

void SmartBath::setShowerState(PipeState state) {
    _setShowerState(state, true);
}

void SmartBath::sendMessage(string topic, string message) {
    auto msg = mqtt::make_message(topic, message);
    mqtt_client->publish(msg);
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

double SmartBath::getDefaultTemperature() {
    return defaultTemperature;
}

double SmartBath::getBathtubCurrentVolume() {
    return bathtubCurrentVolume;
}

int SmartBath::listenForDevices(SmartBath** instance_ptr) {
    const vector<string> TOPICS { "temperature", "waterQuality", "salt", "display", "command" };
    const vector<int> QOS { 0, 0, 0, 0, 1 };

    SmartBath* bath = *instance_ptr;

    mqtt::client cli(SERVER_ADDRESS, CLIENT_ID);
    bath->mqtt_client = &cli;

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
            cout << "[Received] " << msg->get_topic() << ": " << msg->to_string() << endl;
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
                } catch(...) { }
            } else if(msg->get_topic() == string("display")) {
                try {
                    string qualityString = msg->to_string();
                    string delimiter("/");
                    auto splitted = splitString(qualityString, delimiter);
                    if(splitted[0] == string("setPipe")) {
                        PipeState state;
                        if(splitted[2] == string("on")) {
                            double debit = stod(splitted[3]);
                            double temperature = bath->defaultTemperature;
                            if(splitted.size() > 4) {
                                temperature = stod(splitted[4]);
                            }
                            state = { .isOn = true, .temperature = temperature, .debit = debit };
                        } else if(splitted[2] == string("off")) {
                            state = { .isOn = false, .temperature = 0, .debit = 0 };
                        } else {
                            throw;
                        }
                        if(splitted[1] == string("bath")) {
                            bath->setBathState(state);
                        } else if(splitted[1] == string("shower")) {
                            bath->setShowerState(state);
                        }
                    }
                } catch(...) { }
            } else if(msg->get_topic() == string("command")) {
                if(msg->to_string() == string("stop")) {
                    break;
                }
            }
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

void SmartBath::sendStopCommand(SmartBath* bath) {
    auto msg = mqtt::make_message("command", "stop");
    bath->mqtt_client->publish(msg);
}

bool SmartBath::checkWaterQuality(WaterQuality waterQuality) {
    return (6.5 <= waterQuality.pH && waterQuality.pH <= 8.5)
        && (250.0 <= waterQuality.chlorides && waterQuality.chlorides <= 400.0)
        && (0.1 <= waterQuality.iron && waterQuality.iron <= 0.3)
        && (100.0 <= waterQuality.calcium && waterQuality.calcium <= 180.0)
        && (15.0 <= waterQuality.color && waterQuality.color <= 30.0);
}

void SmartBath::prepareBath(double weight, double temperature) {
    if(isFillTargetSet) {
        throw std::runtime_error("BATH_ALREADY_IN_PREPARATION");
    }
    double _fillTarget = weight * (1 / HUMAN_BODY_DENSITY);
    if(_fillTarget > bathtubValume) {
        throw std::runtime_error("You're too fat man...");
    }
    if(_fillTarget <= bathtubCurrentVolume) {
        throw std::runtime_error("Already filled.");
    }
    blockingMutex.lock();
    isFillTargetSet = true;
    fillTarget = _fillTarget;
    bathState = { .isOn = true, .temperature = temperature, .debit = MAX_BATH_DEBIT };
    blockingMutex.unlock();
}

void SmartBath::prepareBath(double weight) {
    prepareBath(weight, defaultTemperature);
}
