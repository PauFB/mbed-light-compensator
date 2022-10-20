#include "mbed.h"

class CustomIntensityLED {
    public:
        CustomIntensityLED(PinName led_pin);
        void setLEDIntensity(float intensity);
    private:
        DigitalOut _led;
};
