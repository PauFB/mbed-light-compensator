#include "mbed.h"
#include "LightCompensator.h"

int main()
{
    LightCompensator lightcompensator(A0, A3);
    while (true) {
        printf("%f\n", lightcompensator.read_lux());
        lightcompensator.ledOn();
        ThisThread::sleep_for(1s);
        lightcompensator.ledOff();
        ThisThread::sleep_for(1s);
    }
}
