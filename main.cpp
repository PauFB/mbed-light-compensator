#include "mbed.h"
#include "Photoresistor.h"
#include "Grove_LCD_RGB_Backlight.h"

int main()
{
    Photoresistor photoresistor(A0);
    AnalogIn potenciometer(A1);
    Grove_LCD_RGB_Backlight screen(D14, D15);
    PwmOut led(D3);
    PwmOut buzzer(D5);

    buzzer.period(0.01);

    while (true) {
        float lux_percent = photoresistor.read_percent();
        float pot_percent = potenciometer.read();
        printf("Lux: %f%%, Pot: %f%%\n", lux_percent * 100, pot_percent * 100);

        float lux_compensated_percent;
        if (lux_percent <= pot_percent)
            lux_compensated_percent = pot_percent - lux_percent;
        else
            lux_compensated_percent = 0.0;
        
        if (lux_percent < 0.0)
            buzzer.write(0.25);
        else
            buzzer.write(0.0);

        led.write(lux_compensated_percent);

        screen.setRGB(58, 52, 235);

        char lux_str[16] = "lux: ";
        char lux_percent_str[16];
        sprintf(lux_percent_str, "%.6f", lux_percent * 100);
        strcat(lux_str, lux_percent_str);

        char com_str[16] = "com: ";
        char com_percent_str[16];
        sprintf(com_percent_str, "%.6f", lux_compensated_percent * 100);
        strcat(com_str, com_percent_str);
        
        screen.clear();
        screen.locate(0, 0);
        screen.print(lux_str);
        screen.locate(15, 0);
        char unit_str[] = "%";
        screen.print(unit_str);

        screen.locate(0, 1);
        screen.print(com_str);
        screen.locate(15, 1);
        screen.print(unit_str);

        ThisThread::sleep_for(1s);
    }
}
