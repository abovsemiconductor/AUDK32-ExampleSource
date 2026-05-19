# AUDK32 Example Applications

Example applications and reference projects for the ABOV A31xxxx/A33xxxx/A34xxxx SDK.

This package provides peripheral examples, demonstration applications, and template projects for supported A31xxxx devices.  
The examples are intended to help users quickly evaluate peripherals and accelerate application development.

## Features

- Peripheral usage examples
- Demonstration applications
- Ready-to-build reference projects
- Template project for user applications
- Bare-metal examples
- Device-specific example structure

## Getting Started

### 1. Select a project

Choose one of the examples:

```text
Bare/Peripheral/
Demo/
AirClock/
GpioLedBlinking/
TmplUserApp/
```

### 2. Open the build project

Project files are available in:

```text
Example/Build/
```

Supported environments:

- ARM/Keil
- IAR Embedded Workbench
- Eclipse

### 3. Build and download

Build the selected project and download it to the target device.

### 4. Verify operation

Examples may use:

- UART output
- LEDs
- External peripherals
- Display devices
- Debugger output

Refer to each example source for implementation details.

## Notes

Some examples require:

- External hardware modules
- Sensors
- EEPROM devices
- OLED display modules
- Flash memory devices

Check each example directory for dependency information.
