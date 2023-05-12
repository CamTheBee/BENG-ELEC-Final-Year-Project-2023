/* mbed Microcontroller Library
 * Copyright (c) 2006-2020 ARM Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* MBED_DEPRECATED */
#warning "These services are deprecated and will be removed. Please see services.md for details about replacement services."

/*
#ifndef MBED_BLE_HEART_RATE_SERVICE_H__
#define MBED_BLE_HEART_RATE_SERVICE_H__
*/
#ifndef BLOOD_GLUCOSE_SERVICE_H__
#define BLOOD_GLUCOSE_SERVICE_H__

#include "ble/BLE.h"
#include "ble/Gap.h"
#include "ble/GattServer.h"

#if BLE_FEATURE_GATT_SERVER

/**
 * BLE Heart Rate Service.
 *
 * @par purpose
 *
 * Fitness applications use the heart rate service to expose the heart
 * beat per minute measured by a heart rate sensor.
 *
 * Clients can read the intended location of the sensor and the last heart rate
 * value measured. Additionally, clients can subscribe to server initiated
 * updates of the heart rate value measured by the sensor. The service delivers
 * these updates to the subscribed client in a notification packet.
 *
 * The subscription mechanism is useful to save power; it avoids unecessary data
 * traffic between the client and the server, which may be induced by polling the
 * value of the heart rate measurement characteristic.
 *
 * @par usage
 *
 * When this class is instantiated, it adds a heart rate service in the GattServer.
 * The service contains the location of the sensor and the initial value measured
 * by the sensor.
 *
 * Application code can invoke updateHeartRate() when a new heart rate measurement
 * is acquired; this function updates the value of the heart rate measurement
 * characteristic and notifies the new value to subscribed clients.
 *
 * @note You can find specification of the heart rate service here:
 * https://www.bluetooth.com/specifications/gatt
 *
 * @attention The service does not expose information related to the sensor
 * contact, the accumulated energy expanded or the interbeat intervals.
 *
 * @attention The heart rate profile limits the number of instantiations of the
 * heart rate services to one.
 */
class BloodGlucoseService {

public:
    /**
     * Construct and initialize a heart rate service.
     *
     * The construction process adds a GATT heart rate service in @p _ble
     * GattServer, sets the value of the heart rate measurement characteristic
     * to @p hrmCounter and the value of the body sensor location characteristic
     * to @p location.
     *
     * @param[in] _ble BLE device that hosts the heart rate service.
     * @param[in] hrmCounter Heart beats per minute measured by the heart rate
     * sensor.
     * @param[in] location Intended location of the heart rate sensor.
     */
    BloodGlucoseService(BLE &_ble, uint16_t bgmCounter) :
        ble(_ble),
        valueBytes(bgmCounter),
        bgmRate(
            GattCharacteristic::UUID_GLUCOSE_MEASUREMENT_CHAR,
            valueBytes.getPointer(),
            valueBytes.getNumValueBytes(),
            BloodGlucoseValueBytes::MAX_VALUE_BYTES,
            GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY
        )
    {
        setupService();
    }

    /**
     * Update the heart rate that the service exposes.
     *
     * The server sends a notification of the new value to clients that have
     * subscribed to updates of the heart rate measurement characteristic; clients
     * reading the heart rate measurement characteristic after the update obtain
     * the updated value.
     *
     * @param[in] hrmCounter Heart rate measured in BPM.
     *
     * @attention This function must be called in the execution context of the
     * BLE stack.
     */
    void updateBloodGlucose(uint16_t bgmCounter) {
        valueBytes.updateBloodGlucose(bgmCounter);
        ble.gattServer().write(
            bgmRate.getValueHandle(),
            valueBytes.getPointer(),
            valueBytes.getNumValueBytes()
        );
    }

protected:
    /**
     * Construct and add to the GattServer the heart rate service.
     */
    void setupService() {
        GattCharacteristic *charTable[] = {
            &bgmRate,
        };
        GattService bgmService(
            GattService::UUID_GLUCOSE_SERVICE,
            charTable,
            sizeof(charTable) / sizeof(charTable[0])
        );

        ble.gattServer().addService(bgmService);
    }

protected:
    /*
     * Heart rate measurement value.
     */
    struct BloodGlucoseValueBytes {
        /* 1 byte for the Flags, and up to two bytes for heart rate value. */
        static const unsigned MAX_VALUE_BYTES = 5;
        static const unsigned FLAGS_BYTE_INDEX = 0;

        static const unsigned VALUE_FORMAT_BITNUM = 0;
        static const uint8_t  VALUE_FORMAT_FLAG = (1 << VALUE_FORMAT_BITNUM);

        BloodGlucoseValueBytes(uint16_t bgmCounter) : valueBytes()
        {
            updateBloodGlucose(bgmCounter);
        }

        void updateBloodGlucose(uint16_t bgmCounter)
        {
            if (bgmCounter <= 65534) {
                valueBytes[FLAGS_BYTE_INDEX] &= ~VALUE_FORMAT_FLAG;
                /*
                 1010
                +1001
                 ----
                 1000 = 8 
                 &= Compares the 2 values using AND and then assigns the output to the rest.
                */
                valueBytes[FLAGS_BYTE_INDEX + 1] = bgmCounter;
            } else {
                valueBytes[FLAGS_BYTE_INDEX] |= VALUE_FORMAT_FLAG;
                valueBytes[FLAGS_BYTE_INDEX + 1] = (uint8_t)(bgmCounter & 0xFFFF);
                valueBytes[FLAGS_BYTE_INDEX + 2] = (uint8_t)(bgmCounter >> 8);
                valueBytes[FLAGS_BYTE_INDEX + 3] = (uint8_t)(bgmCounter >> 16);
                valueBytes[FLAGS_BYTE_INDEX + 4] = (uint8_t)(bgmCounter >> 24);
            }
        }

        uint8_t *getPointer()
        {
            return valueBytes;
        }

        const uint8_t *getPointer() const
        {
            return valueBytes;
        }

        unsigned getNumValueBytes() const
        {
            if (valueBytes[FLAGS_BYTE_INDEX] & VALUE_FORMAT_FLAG) {
                return 1 + sizeof(uint16_t);
            } else {
                return 1 + sizeof(uint8_t);
            }
        }

    private:
        uint8_t valueBytes[MAX_VALUE_BYTES];
    };

protected:
    BLE &ble;
    BloodGlucoseValueBytes valueBytes;
    GattCharacteristic bgmRate;
};

#endif // BLE_FEATURE_GATT_SERVER

#endif /* #ifndef BLOOD_GLUCOSE_SERVICE_H__*/
