#include <thread>

// Maximum bath water debit measured in liters/secomd
#define MAX_BATH_DEBIT .25
// Maximum shower water debit measured in liters/secomd
#define MAX_SHOWER_DEBIT .20
// Drain speed of the water when the stopper is lifted, measured in liters/secomd
#define DRAIN_SPEED .20

// State of shower/bath
typedef struct PipeState {
    // Specifies whether the pipe is on/off.
    bool isOn;
    // Specifiers the temperature of the water flowing through te pipe,
    // If the pipe is off, the expected value should be 0.
    float temperature;
    // Specifies the debit (how much water is flowing through the pipe).
    float debit;
} PipeState;

// Water quality details that are received from the sensor and stored inside the class.
typedef struct WaterQuality
{
    float pH; //6.5 - 8.5
    float chlorides; //250 - 400 mg/l
    float iron; //0.1 - 0.3 mg/l
    float calcium; //100 - 180 mg/l
    float color; //15 - 30
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
    const float bathtubValume = 300;
    // Current Volume of the bathtub in liters. It is calculated by the intervalCheck function
    float bathtubCurrentVolume = 0;
    // Bool variable representing the state of the bathtub stopper.
    // If it is unplugged (false), the water will drain. 
    bool isOnWaterStopper = true;
    
    // Varible to store the thread that is running the intervalCheck function
    // It is needed by the destructor to join at lifecycle end.
    std::thread checkThread;
    // Variable to tell the intervalCheck function to stop its recursive calls and return.
    bool _running = true;

    // Singleton instance
    static SmartBath* instance;
    // Private constructor
    SmartBath() {
        // Initialize states
        showerState = { .isOn = false, .temperature = 0, .debit = 0, };
        bathState = { .isOn = false, .temperature = 0, .debit = 0, };
        // Start the thread running intervalCheck and move its reference to the class member
        checkThread = std::move(std::thread([=]() { intervalCheck(this); return 1; }));
    }

    // Destructor of the SmartBath class
    ~SmartBath() {
        // Set the running state to false, so intervalCheck function will return
        this->_running = false;
        // Wait for the thread to join
        checkThread.join();
    }

    // Threaded function that checks every second if the water quality is good and the bathtub didn't fill up.
    static int intervalCheck(SmartBath* bath) {
        // Sleep one second
        sleep(1);
        // Get the current water debit from each pipe
        float currentDebit = bath->showerState.debit + bath->bathState.debit;

        float volume = bath->bathtubCurrentVolume + currentDebit;
        std::cout << "Current volume: " << volume << " liters\n";
        // If the stopper is not plugged, then substract the water that has drained
        if(!bath->isOnWaterStopper) {
            volume -= DRAIN_SPEED;
        }
        // If the bathtub is filling up turn off the pipes
        if(volume >= bath->bathtubValume) {
            bath->bathtubCurrentVolume = 300;
            bath->showerState = { .isOn = false, .temperature = 0, .debit = 0, };
            bath->bathState = { .isOn = false, .temperature = 0, .debit = 0, };
        } else {
            // else set the new volume
            bath->bathtubCurrentVolume = volume;
        }
        if(bath->_running)
            return intervalCheck(bath);
        return 0;

        // TODO: Check water quality
    }

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
            delete instance; // Calls the destructor
        }
    }

    // Method to set water quality. Returns true if set was successful, false otherwise.
    bool setWaterQuality(WaterQuality waterQuality) {
        // TODO: Stop water flow when water quality is bad
        this->waterQuality = waterQuality;
        return true;
    }

    PipeState getBathState() {
        return bathState;
    }

    PipeState getShowerState() {
        return showerState;
    }

    bool setBathState(PipeState state) {
        // TODO: data validation
        bathState = state;
        return true;
    }

    bool setShowerState(PipeState state) {
        // TODO: data validation
        showerState = state;
        return true;
    }
};

SmartBath* SmartBath::instance = nullptr;


