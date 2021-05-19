#pragma once
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
    bool isSetWaterQuality = false;
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
    // Stores the thread that runs the listenForDevices function
    std::thread mqttThread;
    // Mutex to avoid concurrent reading/writing
    std::mutex blockingMutex;

    // Singleton instance
    static SmartBath* instance;
    // Private constructor
    SmartBath();

    // Destructor of the SmartBath class
    ~SmartBath();

    // Threaded function that checks every second if the water quality is good and the bathtub didn't fill up.
    static int intervalCheck(SmartBath** instance_ptr);
    static int listenForDevices(SmartBath** instance_ptr);
    static void sendStopCommand();
    static bool checkWaterQuality(WaterQuality waterQuality);

public:
    // Static method for getting the singleton instance.
    static SmartBath* getInstance();
    // Static method to destroy the instance.
    static void destroyInstance();

    // Method to set water quality. Returns true if set was successful, false otherwise.
    bool setWaterQuality(WaterQuality waterQuality);

    PipeState getBathState();

    PipeState getShowerState();

    // Set Bath State
    // Throws runtime_error
    void setBathState(PipeState state);

    // Set Shower State
    // Throws runtime_error
    void setShowerState(PipeState state);

    void setDefaultTemperature(double temperature);
};