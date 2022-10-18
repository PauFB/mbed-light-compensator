#include "mbed.h"

class LightCompensator {
    public:
        LightCompensator(PinName photoresistor_pin, PinName led_pin);
        float read_lux();
        void ledOn();
        void ledOff();
    private:
        AnalogIn _photoresistor;
        AnalogOut _led;
};
