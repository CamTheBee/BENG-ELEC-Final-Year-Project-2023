/*
#include "mbed.h" 

// Create a DigitalOut “object” called greenLED. Pass constant LED1 as a “parameter”
DigitalOut greenLED(LED1);

//These are "commented out" and so are not part of the program. You can uncomment them by removing the // characters
//Your task is to make a sequence alternating between Green+Red and just Blue 
//DigitalOut blueLED(LED2);
//DigitalOut redLED(LED3);

//The main function - all executable C / C++ applications have a main function. This is our entry point in the software
int main() 
{
    // ALL the repeating code is contained in a  “while loop”
    while(true) 
    { 
        //The code between the { curly braces } is the code that is repeated forever

        // Turn onboard LED ON  
        greenLED = 1; 

        // Wait 0.2 second (1 million microseconds)
        wait_us(100000); 

        // Turn LED OFF
        greenLED = 0;

        // Wait 0.2 second
        wait_us(100000); 
    }
}
*/
/*
 * Copyright (c) 2018-2020 Arm Limited and affiliates.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Copyright (c) 2017-2020 Arm Limited and affiliates.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Copyright (c) 2018-2020 Arm Limited and affiliates.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Copyright (c) 2017-2020 Arm Limited and affiliates.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <cstdint>
#include <cstdio>

#include "mbed.h"

int main()
{
    DigitalOut pwmControl(PA_8, 1);
    DigitalOut iLED(PA_0, 1);
    AnalogIn acOutput(PC_1);
    AnalogIn dcOutput(PC_0);

    while (true) {
        pwmControl = 1;
        wait_us(100);
        pwmControl = 0;
        wait_us(100);
    }
    
}