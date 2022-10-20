#include "mbed.h"
#include "CustomIntensityLED.h"
//#include <cmath>

#define INPUT_VOLTAGE 3.3

CustomIntensityLED::CustomIntensityLED(PinName led_pin): _led(led_pin) {
    _led = DigitalOut(led_pin);
};

// float intensity in % [0.0, 1.0]
void CustomIntensityLED::setLEDIntensity(float intensity) {
    return;
}
