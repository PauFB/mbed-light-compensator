#include "mbed.h"
#include "LightCompensator.h"
#include <cmath>

#define INPUT_VOLTAGE 3.3
#define MAX_LUX_VALUE 728

LightCompensator::LightCompensator(PinName photoresistor_pin, PinName led_pin): _photoresistor(photoresistor_pin), _led(led_pin) {
    _photoresistor = AnalogIn(photoresistor_pin);
    _led = AnalogOut(led_pin);
};

float LightCompensator::read_lux() {
    float vOut = INPUT_VOLTAGE * _photoresistor.read();
    return ((INPUT_VOLTAGE * 500 * vOut) - 500) / 10;
}

void LightCompensator::ledOn() {
    _led.write(1.0);
}

void LightCompensator::ledOff() {
    _led.write(0.0);
}
