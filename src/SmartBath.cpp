
// State of shower/bath
typedef struct PipeState {
    // Specifies whether the pipe is on/off.
    bool isOn;
    // Specifiers the temperature of the water flowing through te pipe,
    // If the pipe is off, the expected value should be 0.
    float temperature;
    // Specifies the debit (how much water is flowing through the pipe).
    float debit;
} s;

// Water quality details that are received from the sensor and stored inside the class.
// TODO: research on water quality parameters and their optimal values
typedef struct WaterQuality
{
    // pH of water
    float pH;
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


    // Singleton instance
    static SmartBath* instance;
    // Private constructor
    SmartBath() {
        // Initialize states
        showerState = { .isOn = false, .temperature = 0, .debit = 0, };
        bathState = { .isOn = false, .temperature = 0, .debit = 0, };
    }

public:
    // Static method for getting the singleton instance.
    static SmartBath *getInstance() {
        if(instance == nullptr) {
            instance = new SmartBath();
        }
        return instance;
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
};

SmartBath* SmartBath::instance = nullptr;


