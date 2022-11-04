#include "mbed.h"
#include "Photoresistor.h"
#include "Grove_LCD_RGB_Backlight.h"
#include <cstdint>
#include <cstdio>
#include <ratio>

#define DEADLINE 500

Photoresistor photoresistor(A0);
AnalogIn potenciometer(A1);
PwmOut led(D3);
Grove_LCD_RGB_Backlight screen(D14, D15);
PwmOut buzzer(D5);
InterruptIn button(D4);


float lux_percent = 0.0;
float pot_percent = 0.0;
float lux_compensated_percent = 0.0;
float lux_sum = 0.0;
int n_lux_reads = 0;
uint64_t start;
uint64_t timer;
int n_interrupts = 0;
bool calculant_mitjana = false;


void read_compensate_lux()
{
    uint64_t start1 = Kernel::get_ms_count();
    lux_percent = photoresistor.read_percent();
    if (lux_percent < 0.0) {
        buzzer.write(0.25);
        lux_percent = 0.0;
    }
    pot_percent = potenciometer.read();
    if (pot_percent < 0.0) {
        buzzer.write(0.25);
        pot_percent = 0.0;
    }

    if (lux_percent <= pot_percent)
        lux_compensated_percent = pot_percent - lux_percent;
    else
        lux_compensated_percent = 0.0;

    led.write(lux_compensated_percent);

    printf("Lux: %f%%, Pot: %f%%, Com: %f%%\n", lux_percent * 100, pot_percent * 100, lux_compensated_percent * 100);

    uint64_t end1 = Kernel::get_ms_count();
    printf("read_compensate_lux tarda %llu ms\n", end1 - start1);
}


void display()
{
    uint64_t start2 = Kernel::get_ms_count();
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
    uint64_t end2 = Kernel::get_ms_count();
    printf("display tarda %llu ms\n", end2 - start2);
}


void calculate_lux_mean()
{
    lux_sum += lux_percent;
    n_lux_reads++;
    if ((Kernel::get_ms_count() - timer) >= 10000) {
        printf("Mitjana dels ultims 10 segons: %f", lux_percent/n_lux_reads);
        calculant_mitjana = false;
        button.enable_irq();
    }
}


void start_lux_mean(){
    button.disable_irq();
    lux_sum = 0.0;
    n_lux_reads = 0;
    timer = Kernel::get_ms_count();
    calculate_lux_mean();
    calculant_mitjana = true;
}


void button_interrupt()
{
    // Si estem en el budget executar
    if ((start - Kernel::get_ms_count()) >= 100) {  //100 seria temps de read_compensate_lux + display
        start_lux_mean();
    // Sino encuar
    } else {
        n_interrupts++;
    }
}


bool comprovar_sobrecarrega(){
    if (DEADLINE < (start - Kernel::get_ms_count())) {
        buzzer.write(0.25);
        return true;
    }
    return false;
}


int main()
{
    buzzer.period(0.01);
    screen.setRGB(58, 52, 235);
    button.rise(&button_interrupt);

    while (true) {
        start = Kernel::get_ms_count();

        read_compensate_lux();
        if (!comprovar_sobrecarrega()){

            display();
            if (!comprovar_sobrecarrega()){

                if (n_interrupts > 0){
                    start_lux_mean();
                } else if (calculant_mitjana) {
                    calculate_lux_mean();
                }
                if (!comprovar_sobrecarrega()){
                    uint64_t time_remaining = DEADLINE - (start - Kernel::get_ms_count());
                    ThisThread::sleep_for(time_remaining);
                }
            }
        } else {
            ThisThread::sleep_for(500ms);   //Si sobrecarrega pitar 500ms
        }
        buzzer.write(0.0);
    }
}
