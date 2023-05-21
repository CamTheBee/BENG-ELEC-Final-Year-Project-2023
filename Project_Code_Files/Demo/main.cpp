
//Demo Code for the showcase

#include "mbed.h"

DigitalOut pwmControl(PA_8, 1);
DigitalOut iLED(PA_0, 1);
AnalogIn acOutput(PC_1);
AnalogIn dcOutput(PC_0);

const chrono::milliseconds sampleRate = 1000ms; //Sets the sample rate for the programme

//List of EventQueues
EventQueue mainQueue; //EventQueue for main to prevent it from ending
EventQueue pwmQueue; //PWM EventQueue - Allows for dual-supply on the Op-Amps
EventQueue pdReadQueue; //EventQueue for the Photodiode Reading Thread
EventQueue printQueue;

//List of threads
Thread pwm; //Thread dedicated for suppling the pwm to the dual-rails
Thread pdRead; //Thread dedicated to reading the photodiode data
Thread prints; //Thread dedicated for printing to the terminal

//List of functions for threads - sets up the threads EventQueues
void pwmTask () {
    pwmQueue.dispatch_forever();
}

void pdReadTask () {
    pdReadQueue.dispatch_forever();
}

void printTask () {
    printQueue.dispatch_forever();
}

void pwmSwitch() {
    pwmControl=!pwmControl;
}

void pdReading() {
    //Reading the DC and AC photodiode values.
    double acComponent = acOutput.read_u16();
    double dcComponent = dcOutput.read_u16();
    acComponent = (3.3/4096)*(acComponent/16);
    dcComponent = (3.3/4096)*(dcComponent/16);

    printQueue.call(printf, "AC Component Value: %f\n", acComponent);
    printQueue.call(printf, "DC Component Value: %f\n\n", dcComponent);
}

int main() {
    
    pwm.start(pwmTask); //Starts pwm thread for dual-rail supply
    pwm.set_priority(osPriorityRealtime); //Set pwm thread to highest priority as it is required to be active to enable for the circuit to work correctly

    //Reset of threads started with normal priority
    pdRead.start(pdReadTask); 
    prints.start(printTask);

    pwmQueue.call_every(1ms, callback(pwmSwitch)); //Calls the pwm thread every 1ms to ensure the pwm switches at a rate of 1kHz as designed for the circuitry 
    pdReadQueue.call_every(sampleRate, callback(pdReading)); //Calls the Photodiode reading thread every 10ms to ensure a sampling rate of 100Hz.

    mainQueue.dispatch_forever(); //Sets the main thread to dispatch forever so it sleeps until it is given a task

}
    
