#include "SDBlockDevice.h"
#include "FATFileSystem.h"
#include "PushSwitch.hpp"
#include <chrono>
#include "mbed.h"
#include <chrono>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <math.h>
#include <cmath>
#include <iterator>

using namespace std;

/*
Author:
Cameron Stephens
Description:
- This is the main file for the Blood Glucose measuring via PPG signals.
- Most functions are contained in this main file rather than separate .hpp and .cpp files as the bulk of the code is sampling and writing to an SD Card.
- RTOS is used for accurate sampling via threads and a MailBox.
- CRC has been implemented to ensure data integrity throughout the code.
- An SD Card is required to run this code!

Disclaimer:
- Some of this code was extracted from code written by me (Cameron Stephens) and James Upfield for the ELEC351 Coursework but implemented for this project.
- A PushSwitch.hpp file is used to implement a thread safe PushSwitch Class. This was written by Nicholas Outram.

Error Forcing:
A list of if statments have been tight to different GPIOs to mock errors. To demo these errors, buttons must be attached the the GPIO pins listed below.
The blue user button is an exception as this button is always attached to the 401RE board.

- PA_5 - Pressing errorButton1 forces a fail putting a message onto the mailbox
- PA_6 - Pressing errorButton2 causes the creation of a CRC to fail - Depending on where the code is currently when the button is pressed
- PA_7 - Pressing errorButton3 forces the CRC check to fail - Depending on where the code is currently when the button is pressed
- PA_9 - Holding errorButton4 forces a failed read of the mailbox, eventually leading it to run out of memory
- Blue User Button - Pressing the Blue User Button forces a write fail to the SD Card - This must be pressed just before a write takes place to occur
*/

//Structure to store read samples.
struct pdData {
    unsigned long acRead;
    unsigned long dcRead; 
};

//Variables of the structure data type pdData.
struct pdData readData;
struct pdData extract;

int bufferFlag=0; //Int for buffer switching
int sampleCounter=0; //Int to count current sample
int sdDetection; //Int to validate SD Card
int sampleFlag=1; //Int to count how many sample periods have occurred.
int sampleStopFlag=1; //Int for user to input the amount of sample periods.

const chrono::milliseconds sampleRate = 5ms; //Sets the sample rate for the programme - Minimum Tested value is 5ms = 200Hz!
const int bufferSize = 10s/sampleRate;//Const Int for buffer size - Set by the dividing 10s over the sample rate - e.g. 10000/5=2000 samples for 10s
enum {buf = bufferSize}; //enum created to set the buffers to the bufferSize value

//Buffers for storing Photodiode data.
pdData buffer_1[buf];
pdData buffer_2[buf];

Timer tmr1; //Timer
const uint32_t TIMEOUT_MS = 5000; //Constant for watchdog timeout


//GPIO Connections//
PushSwitch userButton(PC_13); //Enables use of the blue user button on the Nucleo board using Nicholas Outram's PushSwitch Class.
DigitalIn userButtonError(PC_13); //Enables uses of the blue user button using the base Mbed button class.

//Integrated buttons for error testing
DigitalIn errorButton1(PA_5); //Forces a fail putting a message onto the mailbox
DigitalIn errorButton2(PA_6); ///Forces the CRC creation to fail
DigitalIn errorButton3(PA_7); //Forces a CRC check to fail
DigitalIn errorButton4(PA_9); //Forces a failed read of the mailbox leading it to eventually run out of memory


PwmOut pwmControl(PA_8); //Controls the pwm for the dual-rails

DigitalOut iLED(PA_0, 0); //Turns on the inferred LED for the SFH7060 package

//User information LEDs - Spare LEDs on the SFH7060 package are used to inform the user of the current process of the programme 
//These do not run whilst sampling is occurring so no corruption of data occurs
DigitalOut grnLED(PB_6, 1); //LED used for confirmation
DigitalOut redLED(PA_1, 0); //LED used for error alerts

//Pins to read in the photodiode data
AnalogIn acOutput(PC_1);
AnalogIn dcOutput(PC_0);

SDBlockDevice sd(PB_5, PB_4, PB_3, PC_7); //SD Card object 
/* Pin Assignment:
PB_5    MOSI (Master Out Slave In)
PB_4    MISO (Master In Slave Out)
PB_3    SCLK (Serial Clock)
PC_7    CS (Chip Select) */


//CRC//
MbedCRC<POLY_32BIT_ANSI, 32> ct; //CRC for data checks

//List of ints for the CRC throughout the code.
uint32_t crcRead;
uint32_t crcReadAC;
uint32_t crcReadDC;
uint32_t crcMessageAC;
uint32_t crcMessageDC;
uint32_t crcCon;
uint32_t crcBuffer;
uint32_t crcBufferAC;
uint32_t crcBufferDC;
uint32_t crcPayloadAC;
uint32_t crcPayloadDC;
uint32_t crcExtAC;
uint32_t crcExtDC;
uint32_t crcTempAC;
uint32_t crcTempDC;
uint32_t crcOutput;


//Threads//
Mail<pdData, 32> mail_box; //Mailbox for sending photodiode data between pdReading() and Consumer() Threads

Mutex sdLock; //Mutex Lock for writting to the SD Card

//List of EventQueues
EventQueue mainQueue; //EventQueue for main to prevent it from ending
//EventQueue pwmQueue; //PWM EventQueue - Allows for dual-supply on the Op-Amps
EventQueue pdReadQueue; //EventQueue for the Photodiode Reading Thread
EventQueue bufferQueue; //Buffer EventQueue - Used by the consumer Thread
EventQueue sdWriteQueue; //EventQueue for SD Card writting
EventQueue printQueue; //EventQueue for all printing
EventQueue errorQueue; //EventQueue used to call errors

//List of threads
//Thread pwm; //Thread dedicated for suppling the pwm to the dual-rails
Thread pdRead; //Thread dedicated to reading the photodiode data
Thread buffer; //Thread dedicated to buffering the photodiode data
Thread sdWrite; //Thread dedicated to writting the photodiode data to an SD Card
Thread prints; //Thread dedicated for printing to the terminal
Thread errors; //Thread dedicated for handling errors that could occur

//List of functions for threads - sets up the threads EventQueues
/*
void pwmTask () {
    pwmQueue.dispatch_forever();
}
*/
void pdReadTask () {
    pdReadQueue.dispatch_forever();
}

void bufferTask () {
    bufferQueue.dispatch_forever();
}

void sdWriteTask () {
    sdWriteQueue.dispatch_forever();
}

void printTask () {
    printQueue.dispatch_forever();
}

void errorTask () {
    errorQueue.dispatch_forever();
}


//Initialsing Functions before main
//void pwmSwitch(); //Used to control the PWM for the dual-rail
void pdReading(); //Photodiode Reading Function
void consumer(); //Buffering of Photodiode data Function
int writeSDCard(pdData sendData[bufferSize]); //Function for writing to the SD Card
int sdMemoryReset(); //Reset SD Card Memory Function
void errorHandler(int errorCode); //Error Handling Function


int main () 
{
    pwmControl.period_us(10);
    pwmControl.pulsewidth_us(5);
    //Introdcution Information for user printed to the terminal
    printf("Welcome to Blood Glucose Sampling using PPG signals!\n");
    printf("WARNING: The data produced can only be saved via a connected micro-SD Card.");
    printf(" Therefore, if an micro-SD Card is not connected, this program will not run!\n");
    printf("The file 'glucoseResults' on the micro-SD Card will be wiped before sampling begins.");
    printf(" Due to this, please ensure any wanted data is backed up before continuing\n");
    printf("Once an micro-SD Card has been connected, please press the blue button to continue.\n");
    
    userButton.waitForPress(); //Uses button class to wait for button input
    ThisThread::sleep_for(50ms); //Thread sent to sleep to prevent switch bounce

    //Checks if SD Card is connected. Memory wipes if it is connected. Returns an init error and system resets if not
    if ((sdDetection = sd.init())==0) {
        //SD Card has been initialised so memory wipe can begin
        printf("Micro-SD Card Detected! Memory wipe in progress...\n"); //Alerts user about SD Card and memory wipe
        sdMemoryReset(); //Calls memory wipe function
    }
    else {
        //Alerts user of missing SD Card error
        printf("\nMicro-SD Init failed: system reset in 5 seconds\n");
        printf("Please insert an Micro-SD Card to begin once system has restarted\n\n");

        //Configures LEDs to inform user of missing SD card
        iLED = 0;
        grnLED = 0;
        redLED = 1;
        //Backup restart if WatchDog Timer fails
        ThisThread::sleep_for(5s); //Waits 5 seconds before resetting
        system_reset(); //Resets program
        return -1;
    }
    
    //More user instructions to choose how many sample periods are required
    printf("A new test can begin.\n");
    printf("How many 10 second samples are required? Please type the amount in the terminal and press the enter key.\n");
    cin >> sampleStopFlag; //Used cin to red user input to the terminal.

    if (sampleStopFlag==0) {
        printf("ERROR: Zero is not a possible choice!\n");
        printf("System Reset in 5 seconds\n");
        //Configures LEDs to inform user of missing SD card
        iLED = 0;
        grnLED = 0;
        redLED = 1;
        //Backup restart if WatchDog Timer fails
        ThisThread::sleep_for(5s); //Waits 5 seconds before resetting
        system_reset(); //Resets program
    }

    printf("%i lots of 10 second samples have been choosen.\n", sampleStopFlag);
    printf("Please press the black reset button if this is incorrect!\n");
    printf("Or\n");
    //User final instruction to press the blue button to begin sampling
    printf("Please press the blue button to start sampling!\n");

    userButton.waitForPress(); //Uses button class to wait for button input
    ThisThread::sleep_for(50ms); //Thread sent to sleep to prevent switch bounce

    //Toggles off the green LED as sampling is occurring - Turns on the inferred LED to sample
    grnLED = 0;
    iLED = 1;

    //Sampling starting so threads can be started
    printf("Sampling starting in 5 seconds...\n");

    //Force sleep of the thread to allow the Photodiode to be fully ready
    ThisThread::sleep_for(5000ms);

    printf("Starting...\n");
    //Create watchdog to reset the program in the event of any critical errors
    Watchdog &watchdog = Watchdog::get_instance();
    watchdog.start(TIMEOUT_MS);

    //pwm.start(pwmTask); //Starts pwm thread for dual-rail supply
    //pwm.set_priority(osPriorityRealtime); //Set pwm thread to highest priority as it is required to be active to enable for the circuit to work correctly

    //Reset of threads started with normal priority
    pdRead.start(pdReadTask); 
    pdRead.set_priority(osPriorityRealtime); //Set pdRead thread to highest priority to minimise jitter while sampling.

    buffer.start(bufferTask);
    sdWrite.start(sdWriteTask);
    prints.start(printTask);
    errors.start(errorTask);

    //pwmQueue.call_every(1ms, callback(pwmSwitch)); //Calls the pwm thread every 1ms to ensure the pwm switches at a rate of 1kHz as designed for the circuitry 
    pdReadQueue.call_every(sampleRate, callback(pdReading)); //Calls the Photodiode reading thread every 10ms to ensure a sampling rate of 100Hz.

    mainQueue.dispatch_forever(); //Sets the main thread to dispatch forever so it sleeps until it is given a task

} //End of main

/*
//PWM function - simply switches the current pwm value everytime the function is called - every 1ms.
void pwmSwitch() {
    pwmControl=!pwmControl;
}
*/

//Function responsible for handling critical errors and halting program execution when a critical error occurs. 
//Critical errors are defined as any error which results in a hardware fault, deadlocks or loss of data.
void errorHandler (int errorCode) {

    //Terminate running threads so watchdog will reset program 
    //pwm.terminate();
    buffer.terminate();
    pdRead.terminate();
    sdWrite.terminate();
    prints.terminate();

    //Turn off infered LED and turns on red LED to alert user
    grnLED = 0;
    iLED = 0;
    redLED = 1;
    //Switch case to inform the user of why the error has occurred. Allowing error to be targetted
    switch(errorCode) { 
        case 0:
            error("CRITICAL ERROR: Mailbox ran out of memory\n"); 
            break;
        case 1:
            error("CRITICAL ERROR: Failed to put message on mailbox queue\n");
            break;
        case 2:
            error("CRITCAL ERROR: Consumer failed to get payload\n");
            break;
        case 3:
            error("CRITCAL ERROR: Buffer switch fail\n");
            break;
        case 4:
            error("CRITCAL ERROR: Buffer flag fail\n");
            break;
        case 5:
            error("CRITCAL ERROR: CRC creation error\n");
            break;
        case 6:
            error("CRITCAL ERROR: CRC data corruption error\n");
            break;
        case 7:
            error("CRITCAL ERROR: Micro-SD Card failed to Initialise\n");
            break;
        case 8:
            error("CRITCAL ERROR: Could not open file for write\n");
            break;
        case 9:
            error("CRITCAL ERROR: Writing to micro-SD Card has resulted in a deadlock\n");
            break;
        default: 
            error("CRITICAL ERROR: Unkown Error\n");
            break;
        break;
    }

    //Backup system reset incase watchdog does not reset the system. 
    ThisThread::sleep_for(5s); 
    system_reset(); 
}


//Reads the Photodiode values and passes the data to the consumer thread via a Mailbox.
void pdReading() {

    //Reading the DC and AC photodiode values.
    readData.acRead = acOutput.read_u16();
    readData.dcRead = dcOutput.read_u16();

    int crcReadACCheck = ct.compute((void *)readData.acRead, 32, &crcReadAC); //Computes a CRC for the read AC data

    //CRC Creation force error
    if (errorButton2.read()==1) {
        crcReadACCheck=1;
    }

    //Checks if the CRC was successful - If not, the error handler is called to inform the user
    if (crcReadACCheck==!0) {
        printQueue.call(printf, "Error with creating CRC for AC Read data!\n");
        errorQueue.call(errorHandler,5);
        return;
    }

    int crcReadDCCheck = ct.compute((void *)readData.dcRead, 32, &crcReadDC); //Computs a CRC for the read DC data

        //CRC Creation force error
    if (errorButton2.read()==1) {
        crcReadDCCheck=1;
    }

    //Checks if the CRC creation was successful - If not, the error handler is called to inform the user
    if (crcReadDCCheck==!0) {
        printQueue.call(printf, "Error with creating CRC for DC Read data!\n");
        errorQueue.call(errorHandler,5);
        return;
    }

    //Tries to put the photodiode data on the mailbox.
    pdData* message = mail_box.try_alloc_for(2s);


    //If passing data to the mailbox fails, the mailbox is out of memory and a critical error is called.
    if (message == NULL) {
        errorQueue.call(errorHandler,0);
        return;
    }

    //If attempt of message works, put the new photodiode data onto the message buffer.
    message->acRead = readData.acRead;
    message->dcRead = readData.dcRead;

    //Check the message is successfully put onto the mailbox.
    osStatus stat = mail_box.put(message);

    //Pressing errorButton1 forces a fail to put a message onto the mailbox 
    if(errorButton1.read()==1) stat=osError;

    //Call an error if not successful
    if (stat != osOK) {
        //ERROR
        printQueue.call(printf, "mail_box.put() Error code: %4Xh, Resource not available\r\n", stat);  
        mail_box.free(message);
        errorQueue.call(errorHandler, 1);
        return;
    }

    int crcMessageACCheck = ct.compute((void *)message->acRead, 32, &crcMessageAC); //Computes a CRC for the message AC data

    //CRC Creation force error
    if (errorButton2.read()==1) {
        crcMessageACCheck=1;
    }

    //Checks if the CRC creation was successful - If not, the error handler is called to inform the user
    if (crcMessageACCheck==!0) {
        printQueue.call(printf, "Error with creating CRC for DC message data!\n");
        errorQueue.call(errorHandler,5);
        return;
    }

    //CRC Check force error
    if (errorButton3.read()==1) {
        crcMessageAC=1;
    }

    //Compares the crc for the AC Read data and the Message AC data - If they differ, data corruption has occurred and an error is called
    if (crcMessageAC==!crcReadAC) {
        printQueue.call(printf, "Error: AC Message Data Corrupt!\n");
        errorQueue.call(errorHandler,6); 
    }

    int crcMessageDCCheck = ct.compute((void *)message->dcRead, 32, &crcMessageDC); //Computd a CRC for the message DC data

    //CRC Creation force error
    if (errorButton2.read()==1) {
        crcReadDCCheck=1;
    }

    //Checks if the CRC creation was successful - If not, the error handler is called to inform the user
    if (crcMessageDCCheck==!0) {
        printQueue.call(printf, "Error with creating CRC for the AC message data!\n");
        errorQueue.call(errorHandler,5);
        return;
    }

    //CRC Check force error
    if (errorButton3.read()==1) {
        crcMessageDC=1;
    }

    //Compares the crc for the DC Read data and the Message DC data - If they differ, data corruption has occurred and an error is called
    if (crcMessageDC==!crcReadDC) {
        printQueue.call(printf, "Error: DC Message Data Corrupt!\n");
        errorQueue.call(errorHandler, 6); 
    }

    //Calls the consumer thread to send the data to the buffer for SD Card writting
    bufferQueue.call(consumer);

    //Kick the watchdog timer to prevent a software reset
    Watchdog::get_instance().kick();

} //End of pdReading Thread


//Collects the data from the mailbox to be buffered so it can be written to an SD Card
void consumer() {

    //Creates a payload for the data
    pdData* payload;


    //Tries to get the payload from the mailbox
    payload = mail_box.try_get_for(2s);
    
    //Pressing and holding causes the mailbox to run out of memory
    if (payload && errorButton4.read()!=1) {

        int crcPayloadACCheck = ct.compute((void *)payload->acRead, 32, &crcPayloadAC); //Computed a CRC for the Payload AC data

        //CRC Creation force error
        if (errorButton2.read()==1) {
            crcPayloadACCheck=1;
        }

        //Checks if the CRC creation was successful - If not, the error handler is called to inform the user
        if (crcPayloadACCheck==!0) {
            mail_box.free(payload);
            printQueue.call(printf, "Error with creating CRC for the AC payload data!\n");
            errorQueue.call(errorHandler,5 );
            //return;
        }

        //CRC Check force error
        if (errorButton3.read()==1) {
            crcPayloadAC=1;
        }

        //Compares the crc for the AC Message data and the Payload AC data - If they differ, data corruption has occurred and an error is called
        if (crcPayloadAC==!crcMessageAC) {
            printQueue.call(printf, "Error: AC Payload Data Corrupt!\n");
            errorQueue.call(errorHandler, 6); 
        }

        int crcPayloadDCCheck = ct.compute((void *)payload->dcRead, 32, &crcPayloadDC); //Computed a CRC for the Payload DC data

        //CRC Creation force error
        if (errorButton2.read()==1) {
            crcPayloadDCCheck=1;
        }

        //Checks if the CRC creation was successful - If not, the error handler is called to inform the user
        if (crcPayloadDCCheck==!0) {
            mail_box.free(payload);
            printQueue.call(printf, "Error with creating CRC for the DC payload data!\n");        
            errorQueue.call(errorHandler,5);
            //return;
        }

        //CRC Check force error
        if (errorButton3.read()==1) {
            crcPayloadDC=1;
        }

        //Compares the crc for the DC Message data and the Payload DC data - If they differ, data corruption has occurred and an error is called
        if (crcPayloadDC==!crcMessageDC) {
            printQueue.call(printf, "Error: DC Payload Data Corrupt!\n");
            errorQueue.call(errorHandler, 6); 
        }
    
        //Collect the data from the payload
        extract.acRead = payload->acRead;
        extract.dcRead = payload->dcRead;

        int crcExtACCheck = ct.compute((void *)extract.acRead, 32, &crcExtAC); //Computed a CRC for the Extracted AC data

        //CRC Creation force error
        if (errorButton2.read()==1) {
            crcExtACCheck=1;
        }

        //Checks if the CRC creation was successful - If not, the error handler is called to inform the user
        if (crcExtACCheck==!0) {
            mail_box.free(payload);
            printQueue.call(printf, "Error with creating CRC for the AC Extract data!\n");    
            errorQueue.call(errorHandler,5);
            //return;
        }

        //CRC Check force error
        if (errorButton3.read()==1) {
            crcExtAC=1;
        }

        //Compares the crc for the AC Payload data and the Extracted AC data - If they differ, data corruption has occurred and an error is called
        if (crcExtAC==!crcPayloadACCheck) {
            printQueue.call(printf, "Error: AC Extract Data Corrupt!\n");
            errorQueue.call(errorHandler,6); 
        }

        int crcExtDCCheck = ct.compute((void *)extract.dcRead, 32, &crcExtDC); //Computed a CRC for the Extracted DC data

        //CRC Creation force error
        if (errorButton2.read()==1) {
            crcExtDCCheck=1;
        }

        //Checks if the CRC creation was successful - If not, the error handler is called to inform the user
        if (crcExtDCCheck==!0) {
            mail_box.free(payload);
            printQueue.call(printf, "Error with creating CRC for the DC Extract data!\n"); 
            errorQueue.call(errorHandler,5);
            //return;
        }

        //CRC Check force error
        if (errorButton3.read()==1) {
            crcExtDC=1;
        }

        //Compares the crc for the DC Payload data and the Extracted DC data - If they differ, data corruption has occurred and an error is called
        if (crcExtDC==!crcPayloadDCCheck) {
            printQueue.call(printf, "Error: DC Extract Data Corrupt!\n");
            errorQueue.call(errorHandler,6); 
        }

        //Free the mailbox payload as data has been extracted
        mail_box.free(payload);

        //Data now wrote into one or two buffers, depending on which is free - switch case used to choose
        switch (bufferFlag) {
            //Write data into the first buffer
            case 0:
                //Data writting into buffer 1
                buffer_1[sampleCounter].acRead = extract.acRead;
                buffer_1[sampleCounter].dcRead = extract.dcRead;
            
                //Computed a CRC for the Buffer AC data and checks if it was successful - If not, the error handler is called to inform the user
                if (int crcBufferACCheck = ct.compute((void *)buffer_1[sampleCounter].acRead, 32, &crcBufferAC)==!0) {
                    printQueue.call(printf, "Error with creating CRC for the AC Buffer data!\n"); 
                    errorQueue.call(errorHandler,5); 
                }

                //CRC Creation force error
                if (errorButton2.read()==1) {
                    printQueue.call(printf, "Error with creating CRC for the AC Buffer data!\n"); 
                    errorQueue.call(errorHandler,5);;
                }

                //CRC Check force error
                if (errorButton3.read()==1) {
                    crcBufferAC=1;
                }

                //Compares the crc for the AC Extracted data and the Buffer AC data - If they differ, data corruption has occurred and an error is called
                if (crcBufferAC==!crcExtAC) {
                    printQueue.call(printf, "Error: AC Buffer Data Corrupt!\n");
                    errorQueue.call(errorHandler,6);
                }
            
                //Computed a CRC for the Buffer DC data and checks if it was successful - If not, the error handler is called to inform the user
                if (int crcBufferDCCheck = ct.compute((void *)buffer_1[sampleCounter].dcRead, 32, &crcBufferDC)==!0) {
                    printQueue.call(printf, "Error with creating CRC for the DC Buffer data!\n"); 
                    errorQueue.call(errorHandler,5); 
                }

                //CRC Creation force error
                if (errorButton2.read()==1) {
                    printQueue.call(printf, "Error with creating CRC for the DC Buffer data!\n"); 
                    errorQueue.call(errorHandler,5);;
                }

                //CRC Check force error
                if (errorButton3.read()==1) {
                    crcBufferDC=1;
                }

                //Compares the crc for the DC Extracted data and the Buffer DC data - If they differ, data corruption has occurred and an error is called
                if (crcBufferDC==!crcTempDC) {
                    printQueue.call(printf, "Error: DC Buffer Data Corrupt!\n");
                    errorQueue.call(errorHandler,6);
                }

                break;
        
            //Write data into the second buffer
            case 1:
                //Data writting into buffer 2
                buffer_2[sampleCounter].acRead = extract.acRead;
                buffer_2[sampleCounter].dcRead = extract.dcRead;

                //Computed a CRC for the Buffer AC data and checks if it was successful - If not, the error handler is called to inform the user
                if (int crcBufferACCheck = ct.compute((void *)buffer_2[sampleCounter].acRead, 32, &crcBufferAC)==!0) {
                    printQueue.call(printf, "Error with creating CRC for the AC Buffer data!\n"); 
                    errorQueue.call(errorHandler,5); 
                }

                //CRC Creation force error
                if (errorButton2.read()==1) {
                    printQueue.call(printf, "Error with creating CRC for the AC Buffer data!\n"); 
                    errorQueue.call(errorHandler,5);;
                }

                //CRC Check force error
                if (errorButton3.read()==1) {
                    crcBufferAC=1;
                }

                //Compares the crc for the AC Extracted data and the Buffer AC data - If they differ, data corruption has occurred and an error is called
                if (crcBufferAC==!crcExtAC) {
                    printQueue.call(printf, "Error: AC Buffer Data Corrupt!\n");
                    errorQueue.call(errorHandler,6);
                }
            
                //Computed a CRC for the Buffer DC data and checks if it was successful - If not, the error handler is called to inform the user
                if (int crcBufferDCCheck = ct.compute((void *)buffer_2[sampleCounter].dcRead, 32, &crcBufferDC)==!0) {
                    printQueue.call(printf, "Error with creating CRC for the DC Buffer data!\n"); 
                    errorQueue.call(errorHandler,5); 
                }

                //CRC Creation force error
                if (errorButton2.read()==1) {
                    printQueue.call(printf, "Error with creating CRC for the DC Buffer data!\n"); 
                    errorQueue.call(errorHandler,5);;
                }

                //CRC Check force error
                if (errorButton3.read()==1) {
                    crcBufferDC=1;
                }

                //Compares the crc for the DC Extracted data and the Buffer DC data - If they differ, data corruption has occurred and an error is called
                if (crcBufferDC==!crcTempDC) {
                    printQueue.call(printf, "Error: DC Buffer Data Corrupt!\n");
                    errorQueue.call(errorHandler,6);
                }

                break;
        
            //Default case - if reached, error has occurred
            default:
                errorQueue.call(errorHandler,3);
                break;
        }
    }
    
    //Increment the sample counter
    sampleCounter++;

    //If statement once 10 seconds of sampling has been reached
    if (sampleCounter==bufferSize) {
        printQueue.call(printf, "Switching buffer...\n"); //Alerts user of the buffer being switched
        
        //Switch case using the buffer flag to determine which buffer contains the data
        switch (bufferFlag){
            //Case for buffer 1
            case 0:
                bufferFlag=1; //Buffer flag is updated
                printQueue.call(printf, "Starting data send on buffer 1...\n"); //Alerts user of data send to first buffer

                //Computed a CRC for the Buffer data and checks if it was successful - If not, the error handler is called to inform the user
                if (int crcBufferCheck = ct.compute((void *)buffer_1, 32, &crcBuffer)==!0) {
                    printQueue.call(printf, "Error with creating CRC for the Buffer data!\n"); 
                    errorQueue.call(errorHandler,5);
                    //return; 
                }
                
                //CRC Creation force error
                if (errorButton2.read()==1) {
                    printQueue.call(printf, "Error with creating CRC for the Buffer data!\n"); 
                    errorQueue.call(errorHandler,5);;
                }

                sdWriteQueue.call(writeSDCard, buffer_1); //Calls the sdWrite buffer with the buffered data
                break;
            
            //Case for buffer 2
            case 1:
                bufferFlag=0; //Buffer flag is updated
                printQueue.call(printf, "Starting data send on bufffer 2...\n"); //Alrts usert of data send to second buffer

                //Computed a CRC for the Buffer data and checks if it was successful - If not, the error handler is called to inform the user
                if (int crcBufferCheck = ct.compute((void *)buffer_2, 32, &crcBuffer)==!0) {
                    printQueue.call(printf, "Error with creating CRC for the Buffer data!\n"); 
                    errorQueue.call(errorHandler,5);
                    //return; 
                }

                //CRC Creation force Error
                if (errorButton2.read()==1) {
                    printQueue.call(printf, "Error with creating CRC for the Buffer data!\n"); 
                    errorQueue.call(errorHandler,5);;
                }

                sdWriteQueue.call(writeSDCard, buffer_2); //Calls the sdWrite buffer with the buffered data
                break;
            
            //Default case - if reached, error has occurred
            default:
                errorQueue.call(errorHandler,4);
                break;
        }

            sampleCounter=0; //Sample counter reset to zero

    }
    //Kick watchdog if no issues to prevent software reset
    Watchdog::get_instance().kick();
}


//Writes the buffered data to the SD Card
int writeSDCard(pdData sendData[bufferSize]) {  
    
    //Computed a CRC for the Output data and checks if it was successful - If not, the error handler is called to inform the user
    if (int crcOutputCheck = ct.compute((void *)sendData, 32, &crcOutput)==!0) {
        printQueue.call(printf, "Error with creating CRC for the Output data!\n"); 
        errorQueue.call(errorHandler,5);
    }

    //CRC Creation force Error
    if (errorButton2.read()==1) {
        printQueue.call(printf, "Error with creating CRC for the Buffer data!\n"); 
        errorQueue.call(errorHandler,5);;
    }

    //CRC Check force error
    if (errorButton3.read()==1) {
        crcOutput=1;
    }

    //Compares the crc for the Buffer data and the Output data - If they differ, data corruption has occurred and an error is called
    if (crcOutput==!crcBuffer) {
        printQueue.call(printf, "Error: Output Data Corrupted!\n");
        errorQueue.call(errorHandler,6);
    }

    printQueue.call(printf, "Checking micro-SD Card - Do not remove the micro-SD Card!\n"); //Informs user that SD Card initialsing occuring
    
    //Error checking the SD card initialsation - If failed, error handler is called
    if (int err = sd.init()==!0) {
        errorQueue.call(errorHandler,7);
        return -1;
    }
    
    FATFileSystem fs("sd", &sd); //Creats an instance allowing SD Card writting

    printQueue.call(printf, "micro-SD check successful, writing to micro-sd...\n"); //Alerts user of SD Card initialisation success

    FILE *fp = fopen("/sd/glucoseresults.txt","a+"); //Set format and attempt to open file in append mode
 
    if(userButtonError.read()==0) fp=NULL; //Holding the userButton forces an error with writing to the SD Card.

    //If unable to open file then a critical error has been encountered as data has not been written and will be lost. 
    if(fp == NULL) {   
        errorQueue.call(errorHandler,8);
        return -1;
    } 

    //If file is opened successfully then write data to sd card. 
    else {
        bool lockTaken = sdLock.trylock_for(200ms); //Lock taken to safeguard SD Card write - Should be safe as writting from opposite buffer to write - If not, error occurrs

        //Writing data to SD Card as lock has been aquired
        if (lockTaken == true) {
            for(int i=0; i<bufferSize; i++) {
                fprintf(fp, "%u,%u\n", sendData[i].acRead, sendData[i].dcRead); //Each result wrote to the SD Card
            }
            sdLock.unlock(); //Release lock as finsihed accessing the buffer
        }
        else {
            errorQueue.call(errorHandler,9); //If not able to acquire lock then deadlock has occured so reset system
            return -1;
        }

        //Closes fp to end writing to the SD Card
        fprintf(fp, "\n\n");
        fclose(fp); 

        //Alerts the user that the SD Card write has finished and the current data set has been saved to the SD Card
        printQueue.call(printf, "micro-SD Write done...\n");
        printQueue.call(printf, "Data set %i saved to the micro-SD card!\n\n", sampleFlag);

        //SD Card deinitialised
        sd.deinit();
        
        //Check to see if the the amount of samples choosen by the user has been reached. If yes, threads are terminated - ending the program.
        if (sampleFlag==sampleStopFlag) {
            //pwm.terminate();
            pdRead.terminate();
            buffer.terminate();
                
            //Turns off the inferred LED and turns on the Green LED, informing the user the system has finished
            iLED = 0;
            redLED = 0;
            grnLED = 1;

            //Alerts user of sampling being complete and the program is about to restart
            printQueue.call(printf,"Sampling Complete!\n");
            printQueue.call(printf,"Please remove the micro-SD card to review sampled data. The file is called 'glucoseresults'.\n");
            printQueue.call(printf,"System Restarting in 5 seconds!\n\n");

            //Backup reset of 5 seconds incase WatchDog Timer Fails
            ThisThread::sleep_for(5s);
            system_reset();
        }

        //If the amount of samples choosen by the user has not been reached, sample flag is incremented and the program still runs
        else {
            sampleFlag++;
        }
        return 0;
    }    
} //End of writeSDCard


//Function to reset the memory of the SD Card
int sdMemoryReset() {

    FATFileSystem fs("sd", &sd); //Creats an instance allowing SD Card writting

    //Open in write mode and write no characters to clear the file
    FILE *fp = fopen("/sd/glucoseresults.txt","w"); 
    fprintf(fp, "");
    fclose(fp);

    //Deinitialise the SD Card
    sd.deinit();

    //Alert user of memory clear success
    printf("Memory Cleared Successfully!\n");
    return 0;
} 
 



