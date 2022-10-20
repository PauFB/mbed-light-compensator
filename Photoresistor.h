#include "mbed.h"

#define INPUT_VOLTAGE 5
#define MAX_LUX_VALUE 1200

class Photoresistor {
    public:
        Photoresistor(PinName photoresistor_pin);
        float read_lux();
        float read_percent();
    private:
        AnalogIn _photoresistor;
};
