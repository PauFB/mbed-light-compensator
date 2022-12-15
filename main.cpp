#include "mbed.h"
#include "Photoresistor.h"
#include "Grove_LCD_RGB_Backlight.h"
#include <cstdint>
#include <cstdio>
#include <ratio>

#define PERIOD_MS 500
#define DEADLINE_READ_COMPENSATE_LUX_MS 52
#define DEADLINE_DISPLAY 12

// read_compensate_lux: 52 ms
// display: 12 ms

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
uint64_t start_read_compensate_lux_ms, start_display_ms;
uint64_t start_button_interrupt_ms;
int n_interrupts = 0;
bool calculant_mitjana = false;


void read_compensate_lux()
{
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
}


void display()
{
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
}


void calculate_lux_mean()
{
    lux_sum += lux_percent;
    n_lux_reads++;
    if ((Kernel::get_ms_count() - start_button_interrupt_ms) >= 10000) {
        printf("Mitjana dels ultims 10 segons: %f\n", lux_sum / n_lux_reads);
        screen.clear();
        screen.locate(0, 0);
        char str[] = "Mitjana (10 seg)";
        screen.print(str);
        screen.locate(0, 1);
        sprintf(str, "%.6f", lux_sum / n_lux_reads);
        screen.print(str);
        screen.locate(15, 1);
        char unit_str[] = "%";
        screen.print(unit_str);

        ThisThread::sleep_for(2000ms);

        calculant_mitjana = false;
        button.enable_irq();
    }
}


void start_lux_mean(){
    button.disable_irq();
    lux_sum = 0.0;
    n_lux_reads = 0;
    start_button_interrupt_ms = Kernel::get_ms_count();
    calculant_mitjana = true;
    calculate_lux_mean();
}


void button_interrupt()
{
    // Si estem en el budget executar
    if ((Kernel::get_ms_count() - start_read_compensate_lux_ms) >= 65) {  // 100 seria temps de read_compensate_lux + display
        start_lux_mean();
    // Si no encuar
    } else {
        n_interrupts++;
    }
}

bool comprovar_read_compensate_lux() {
    if (DEADLINE_READ_COMPENSATE_LUX_MS < (Kernel::get_ms_count() - start_read_compensate_lux_ms)) {
        buzzer.write(0.25);
        ThisThread::sleep_for(200ms);   // Si sobrecarrega pitar 200ms
        return true;
    }
    return false;
}

bool comprovar_display() {
    if (DEADLINE_DISPLAY < (Kernel::get_ms_count() - start_display_ms)) {
        buzzer.write(0.25);
        ThisThread::sleep_for(200ms);   // Si sobrecarrega pitar 200ms
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
        start_read_compensate_lux_ms = Kernel::get_ms_count();
        read_compensate_lux();
        printf("read_compensate_lux tarda %llu ms\n", Kernel::get_ms_count() - start_read_compensate_lux_ms);
        if (!comprovar_read_compensate_lux()) {
            start_display_ms = Kernel::get_ms_count();
            display();
            printf("display tarda %llu ms\n", Kernel::get_ms_count() - start_display_ms);
            if (!comprovar_display()) {
                if (n_interrupts > 0){
                    start_lux_mean();
                    n_interrupts--;
                } else if (calculant_mitjana) {
                    calculate_lux_mean();
                }
                if ((Kernel::get_ms_count() - start_read_compensate_lux_ms) <= PERIOD_MS) {
                    uint64_t time_remaining = PERIOD_MS - (Kernel::get_ms_count() - start_read_compensate_lux_ms);
                    ThisThread::sleep_for(time_remaining);
                }
            }
        }
        buzzer.write(0.0);
    }
}
