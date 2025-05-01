#include "VoltageReader.h"

void initVoltageReader() {
    // Initialize GPIO pin as an input
    pinMode(VOLTAGE_PIN, INPUT);
    Serial.println("Voltage Reading Initialized!");
}

float readAverageVoltage() {
    int totalADCValue = 0; // Sum of ADC values

    // Take multiple samples and average them
    for (int i = 0; i < NUM_SAMPLES; i++) {
        int adcValue = analogRead(VOLTAGE_PIN); // Read ADC value
        totalADCValue += adcValue;              // Accumulate ADC value
        delay(1);                               // Small delay for stability
    }

    // Calculate average ADC value
    float averageADCValue = totalADCValue / (float)NUM_SAMPLES;

    // Convert to voltage (assuming 3.3V reference and 12-bit ADC)
    float averageVoltage = (averageADCValue / 4095.0) * 3.3;

    return averageVoltage;
}

bool isStallDetected() {
    float voltage = readAverageVoltage();
    
    // Log voltage for debugging
    //Serial.print("Average Voltage: ");
    //Serial.println(voltage);
    
    // Check if voltage exceeds the stall threshold
    return (voltage > STALL_VOLTAGE_THRESHOLD);
}