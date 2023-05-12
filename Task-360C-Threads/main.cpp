#include "mbed.h"
#include <cstdio>
//#include "uop_msb.h"
//#include "PushSwitch.hpp"
//#include "FlashingLED.hpp"
//#include <cstdio>

//using namespace uop_msb;

void task1();
void task2();
signed int countUp = 0;
signed int countDown = 0;
Thread t1, t2;

int main() {
    printf("Start\n");
    //FlashingLED yellow(TRAF_YEL1_PIN);
    t1.start(task1);
    t2.start(task2);
    
    //t2.set_priority(osPriorityRealtime);  //Try this

    //Wait for t1 and t2 to end (which they never do)
    t1.join();
    t2.join();
}

// Version 1 - Partially uses a spinning technique
void task1() {
    while (true){
    countUp++;
    printf("Added 1!\n");
    if (countUp==10) {
        printf("Counted to 10!\n");
        countUp = 0;
    }
    ThisThread::sleep_for(1000ms);
    }
    /*
    DigitalOut red_led(TRAF_RED1_PIN);  
    DigitalIn sw1(BTN1_PIN);
    red_led = sw1;
    
    while(true) {
        while (sw1 == 0) {};            //BLOCKS via SPINNING
        ThisThread::sleep_for(50ms);    //Blocks in the WAITING state
        while (sw1 == 1) {};            //BLOCKS via SPINNING
        red_led = !red_led;             
        ThisThread::sleep_for(50ms);    //Blocks in the WAITING state
    } 
    */   
}

// Version 2 - uses a much more power-friendly ISR driven method
void task2() {
    while (true) {
    countDown--;
    if (countDown==-100) {
        printf("Counted to -100!\n");
        countDown = 0;
    }
    
    ThisThread::sleep_for(50ms);
    }
    /*
    DigitalOut green_led(TRAF_GRN1_PIN);
    PushSwitch button2(BTN2_PIN);

    printf("Waiting for switch B\n");
    green_led = button2;    //Blocks until there is a change
    printf("Unblocked - let's go\n");

    while(true) {
        button2.waitForPress();         //Blocks in the WAITING state 
        ThisThread::sleep_for(50ms);    //Blocks in the WAITING state
        button2.waitForRelease();       //Blocks in the WAITING state
        green_led = !green_led;         
        ThisThread::sleep_for(50ms);    //Blocks in the WAITING state
    }  
*/    
}


/*
Part 5 - When t1 has priority, the LED cycle does not work properly. The red LED stil works but when the A button is also pressed, the Yellow led comes on, does not flash.
The A button does nothing unless pressed at the same time as button B. This is because t1 takes all the CPU power and has a busy wait loop so it is sat doing nothing for most 
of the time.

Part 6 - When t2 has priority, there is no busy wait loop to sit in so it all still works fine.
*/