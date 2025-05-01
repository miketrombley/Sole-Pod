#ifndef VOLTAGE_READER_H
#define VOLTAGE_READER_H

#include <Arduino.h>

// Define constants
#define VOLTAGE_PIN 9                // Pin to read the voltage
#define NUM_SAMPLES 50               // Number of samples for averaging
#define STALL_VOLTAGE_THRESHOLD 0.15 // Threshold for detecting motor stall

// Function prototypes
void initVoltageReader();
float readAverageVoltage();
bool isStallDetected();

#endif // VOLTAGE_READER_H