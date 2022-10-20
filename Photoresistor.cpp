#include "mbed.h"
#include "Photoresistor.h"
#include <cmath>

Photoresistor::Photoresistor(PinName photoresistor_pin): _photoresistor(photoresistor_pin) {
    _photoresistor = AnalogIn(photoresistor_pin);
};

float Photoresistor::read_lux() {
    float vOut = INPUT_VOLTAGE * _photoresistor.read();
    return ((INPUT_VOLTAGE * 500 * vOut) - 500) / 10;
}

float Photoresistor::read_percent() {
    return read_lux() / MAX_LUX_VALUE;
}
