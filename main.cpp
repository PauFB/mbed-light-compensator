#include "mbed.h"
#include "Photoresistor.h"
#include "Grove_LCD_RGB_Backlight.h"
#include <cstdint>
#include <cstdio>
#include <ratio>
#include <string>

#define PERIOD_MS 500
#define DEADLINE_READ_COMPENSATE_LUX_MS 1
#define DEADLINE_DISPLAY 12

#define ID_START_LUX_MEAN 0
#define ID_DISPLAY_MEAN 1

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
bool calculant_mitjana = false;

typedef struct {
    int    id;
} message_t;
MemoryPool<message_t, 16> mpool;
Queue<message_t, 16> aperiodic_tasks_queue;

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

    if (calculant_mitjana) {
        lux_sum += lux_percent;
        n_lux_reads++;
    }

    //printf("Lux: %f%%, Pot: %f%%, Com: %f%%\n", lux_percent * 100, pot_percent * 100, lux_compensated_percent * 100);
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

void display_mean()
{
    //printf("Mitjana dels ultims 10 segons: %f\n", (lux_sum / n_lux_reads) * 100);

    screen.clear();
    screen.locate(0, 0);
    char str[] = "Mitjana (10 seg)";
    screen.print(str);
    screen.locate(0, 1);
    sprintf(str, "%.6f", (lux_sum / n_lux_reads) * 100);
    screen.print(str);
    screen.locate(15, 1);
    char unit_str[] = "%";
    screen.print(unit_str);

    calculant_mitjana = false;

    ThisThread::sleep_for(2000ms);
    
    button.enable_irq();
}

void mean_interrupt()
{
    // Si estem en el budget executar
    if ((Kernel::get_ms_count() - start_read_compensate_lux_ms) > (DEADLINE_READ_COMPENSATE_LUX_MS + DEADLINE_DISPLAY)) {
        display_mean();
    // Si no encuar
    } else {
        message_t *message = mpool.alloc();
        message->id = ID_DISPLAY_MEAN;
        aperiodic_tasks_queue.put(message);
    }
}

void start_lux_mean(){
    button.disable_irq();
    lux_sum = 0.0;
    n_lux_reads = 0;
    calculant_mitjana = true;
    EventQueue queue;
    queue.call_in(10s, mean_interrupt);
    queue.dispatch();
}

void button_interrupt()
{
    // Si estem en el budget executar
    if ((Kernel::get_ms_count() - start_read_compensate_lux_ms) > (DEADLINE_READ_COMPENSATE_LUX_MS + DEADLINE_DISPLAY)) {
        Thread *newThread = new Thread(osPriorityBelowNormal, 1024, NULL, NULL);
        newThread->start(callback(start_lux_mean));
    // Si no encuar
    } else {
        message_t *message = mpool.alloc();
        message->id = ID_START_LUX_MEAN;
        aperiodic_tasks_queue.put(message);
    }
}

bool check_read_compensate_lux_deadline() {
    if ((Kernel::get_ms_count() - start_read_compensate_lux_ms) > DEADLINE_READ_COMPENSATE_LUX_MS) {
        buzzer.write(0.25);
        ThisThread::sleep_for(200ms);   // Si hi ha sobrecàrrega pitar durant 200 ms
        return true;
    }
    return false;
}

bool check_display_deadline() {
    if ((Kernel::get_ms_count() - start_display_ms) > DEADLINE_DISPLAY) {
        buzzer.write(0.25);
        ThisThread::sleep_for(200ms);   // Si hi ha sobrecàrrega pitar durant 200 ms
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
        //printf("read_compensate_lux tarda %llu ms\n", Kernel::get_ms_count() - start_read_compensate_lux_ms);
        if (!check_read_compensate_lux_deadline()) {
            start_display_ms = Kernel::get_ms_count();
            display();
            //printf("display tarda %llu ms\n", Kernel::get_ms_count() - start_display_ms);
            if (!check_display_deadline()) {
                if (!aperiodic_tasks_queue.empty()){
                    osEvent evt = aperiodic_tasks_queue.get();
                    if (evt.status == osEventMessage) {
                        message_t *message = (message_t *)evt.value.p;
                        if (message->id == ID_START_LUX_MEAN){
                            Thread *newThread = new Thread(osPriorityBelowNormal, 1024, NULL, NULL);
                            newThread->start(callback(start_lux_mean));
                        } else if (message->id == ID_DISPLAY_MEAN) {
                            display_mean();
                        }
                        mpool.free(message);
                    }
                }
                if ((Kernel::get_ms_count() - start_read_compensate_lux_ms) < PERIOD_MS) {
                    uint64_t time_remaining = PERIOD_MS - (Kernel::get_ms_count() - start_read_compensate_lux_ms);
                    ThisThread::sleep_for(time_remaining);
                }
            }
        }
        buzzer.write(0.0);
    }
}
