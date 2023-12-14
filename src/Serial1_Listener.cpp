#include "Serial1_Listener.h"

char buffer[256];   // Where we will hold the serial data

Serial1_Listener *Serial1_Listener::_instance;

// [static]
Serial1_Listener &Serial1_Listener::instance() {
    if (!_instance) {
        _instance = new Serial1_Listener();
    }
    return *_instance;
}

Serial1_Listener::Serial1_Listener() {
}

Serial1_Listener::~Serial1_Listener() {
}

void Serial1_Listener::setup() {
    Serial1.begin(115200);                              // Open serial port to communicate with Serial1 device
    Log.info("Starting up the Serial1_Listener");
}

void Serial1_Listener::loop() {                        
}

bool Serial1_Listener::getResponse(char *response, int responseSize) {       // This function will return the response from the Serial1 device
                                 
    unsigned long readStart = millis();   // Safely wait here until there is data available to read - takes the device a beat to respond.
    while(millis() - readStart < 4000){}; // Give plenty of time for the device to output.

    while(Serial1.available()) {                                             // Check if there is data available to read                 
        int numBytes = Serial1.readBytesUntil('\n', buffer, sizeof(buffer) - 1);
        if (numBytes > 0) {
            buffer[numBytes] = 0;
            strncpy(response, buffer, responseSize);                         // Copy the response to the response buffer
            buffer[0] = 0;                                                   // Clear the buffer
            // Clear any remaining data in the Serial1 output buffer
            while (Serial1.available()) {
                delay(10);
                Serial1.read();
            }
            return true;                                                     // Return true to indicate that there was a response
        }
        else {
            Log.info("Serial1_Listener received no bytes");
            return false;
        }
    }
    return false;
}

