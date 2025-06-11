# Sole Pod Control System

An ESP32-based firmware for controlling a smart pod system with motorized door and tray, LED lighting, and wireless connectivity.

## 🚀 Features

### Core Functionality
- **Motorized Door & Tray Control**: Automated opening/closing sequence with precise position control
- **Smart LED Lighting**: Full-color NeoPixel LED with adjustable brightness and color
- **Hall Sensor Feedback**: Precise position detection for door and tray states
- **Safety Systems**: Motor stall detection with automatic shutdown
- **Persistent Settings**: All configurations saved to flash memory

### Connectivity
- **Bluetooth Low Energy (BLE)**: Local device control via mobile app
- **WiFi Integration**: Network connectivity with credential management
- **AWS IoT Core**: Cloud connectivity for remote monitoring and control
- **MQTT Communication**: Real-time state updates and commands

### User Interface
- **Physical Buttons**: Manual door and LED control
- **Child Lock**: Safety feature to prevent accidental operation
- **Status Indicators**: Real-time feedback on device state

## 📋 Hardware Requirements

### Compatible Hardware
- **Control Board**: v1.4
- **LED Board**: v1.2  
- **Hall Sensor Board**: v1.0
- **Microcontroller**: ESP32

### Pin Configuration
```cpp
// Motor Control
#define DOOR_MOTOR 46          // Door motor control
#define DOOR_DIRECTION 3       // Door motor direction
#define TRAY_MOTOR 10          // Tray motor control  
#define TRAY_DIRECTION 11      // Tray motor direction

// Sensors
#define SW_DOOR_CLOSED 45      // Door closed sensor
#define SW_DOOR_OPENED 48      // Door opened sensor
#define SW_TRAY_CLOSED 35      // Tray closed sensor
#define SW_TRAY_OPENED 36      // Tray opened sensor

// User Interface
#define DOOR_BTN 47            // Door control button
#define LED_BTN 21             // LED control button
#define LED_DATA_PIN 14        // NeoPixel data pin

// Safety
#define VOLTAGE_PIN 9          // Motor current monitoring
```

## 🏗️ Architecture

### System States
The pod operates in 5 distinct states:
1. **CLOSED** (0): Door closed, tray closed
2. **DOOR_MIDWAY** (1): Door transitioning, tray closed  
3. **DOOR_OPEN** (2): Door open, tray closed
4. **TRAY_MIDWAY** (3): Door open, tray transitioning
5. **OPEN** (4): Door open, tray open

### Operation Sequence
**Opening**: CLOSED → DOOR_MIDWAY → DOOR_OPEN → TRAY_MIDWAY → OPEN
**Closing**: OPEN → TRAY_MIDWAY → DOOR_OPEN → DOOR_MIDWAY → CLOSED

## 📱 BLE Interface

### Service UUID
`7d840001-11eb-4c13-89f2-246b6e0b0000`

### Characteristics
| Function | UUID | Type | Range | Description |
|----------|------|------|-------|-------------|
| Door Status | `7d840002-...0001` | R/W | 0-1 | 0=Closed, 1=Open |
| Door Position | `7d840003-...0002` | R/W | 50,100 | Partial/Full open |
| LED Status | `7d840004-...0003` | R/W | 0-1 | 0=Off, 1=On |
| LED Brightness | `7d840005-...0004` | R/W | 0-100 | Brightness percentage |
| LED Color | `7d840006-...0005` | R/W | Hex | 6-digit hex color code |
| WiFi Credentials | `7d840007-...0006` | W | String | Format: `SSIDENDNETWORKPASSWORDENDPASSWORD` |
| WiFi Status | `7d840008-...0007` | R/N | String | Connection status |

## ☁️ AWS IoT Integration

### Device Shadow Topics
- **Publish**: `$aws/things/pod_1/shadow/update`
- **Subscribe**: `$aws/things/pod_1/shadow/update/accepted`

### Shadow Document Structure
```json
{
  "state": {
    "reported": {
      "is_open": true,
      "nightlight": false,
      "color": "FF0000",
      "nightlight_brightness": 75
    },
    "desired": {
      "is_open": false,
      "nightlight": true,
      "color": "0000FF",
      "nightlight_brightness": 50
    }
  }
}
```

## 🔧 Configuration

### AWS Configuration
Update `aws_config.h` with your AWS IoT settings:
```cpp
#define AWS_IOT_ENDPOINT "your-endpoint-ats.iot.region.amazonaws.com"
#define DEVICE_NAME "your_device_name"
```

Replace the certificate and key constants with your device-specific credentials.

### WiFi Configuration
Default credentials can be set in `main.cpp`:
```cpp
const char* defaultSSID = "your_network";
const char* defaultPassword = "your_password";
```

## 🛡️ Safety Features

### Motor Stall Detection
- **Threshold**: 0.2V (configurable in `VoltageReader.h`)
- **Sampling**: 50-sample average for noise reduction
- **Response**: Immediate motor shutdown and system lockout
- **Recovery**: Requires power cycle after stall detection

### Child Lock
- Prevents accidental button operation
- Configurable via software flag
- Applies to both door and LED controls

## 📊 Debug Output

Enable debug mode in `main.cpp`:
```cpp
#define DEBUG_MODE true
```

### Debug Information Includes:
- Current pod state and target
- Sensor readings (door/tray positions)
- LED status (state, brightness, color)
- WiFi connection status and signal strength
- Motor voltage readings
- Safety system status

## 🔄 State Persistence

The system automatically saves and restores:
- LED color and brightness settings
- Door position preference (50% or 100% open)
- LED on/off state
- Door open/closed status

Settings are stored in ESP32 flash memory using the Preferences library.

## 🚦 Status Codes

### Safety Status
- `0`: OK - Normal operation
- `1`: Motor stall detected
- `2`: Obstacle detected  
- `3`: Overcurrent condition
- `4`: System error

### WiFi Status
- "Connected" - Successfully connected
- "Disconnected" - Not connected
- "Connection failed" - Failed to connect
- "SSID not available" - Network not found

## 🛠️ Development

### Dependencies
- Arduino ESP32 Core
- WiFiClientSecure
- PubSubClient (MQTT)
- ArduinoJson
- Adafruit NeoPixel
- ESP32 BLE Arduino
- Preferences

### Building
1. Install ESP32 board package in Arduino IDE
2. Install required libraries
3. Configure AWS certificates in `aws_config.h`
4. Upload to ESP32 device

### Testing
- Use serial monitor at 115200 baud for debug output
- Test BLE connectivity with compatible mobile app
- Verify AWS IoT connection in AWS Console
- Test all physical controls and sensors

## 📝 License

TBD

## 🤝 Contributing

[Add contribution guidelines if applicable]

## 📞 Support

[Add support contact information]

---

**Hardware Compatibility**: Control Board v1.4, LED Board v1.2, Hall Sensor Board v1.0