# Smart Home Monitor System for Arduino Uno

## Overview
This repository contains the source code for an Arduino Uno-based Smart Home Monitor System. The project demonstrates advanced concepts in embedded systems programming, object-oriented design, and serial communication using C++.

## Features

### Embedded Systems Programming
- **Finite State Machine (FSM):** The heart of the system is a deterministic Finite State Machine, ensuring efficient state transitions and interactions. This approach enhances the system's reliability and predictability in various scenarios.

### Object-Oriented Programming (OOP)
- **'Device' Class:** A centerpiece of our design, the 'Device' class encapsulates the properties and behaviors of smart home devices. Key OOP concepts like encapsulation, abstraction, method overloading, and data manipulation are extensively used.

### Serial Communication
- **Custom Protocol:** The system uses a bespoke protocol for reliable host-Arduino communication. This custom solution addresses synchronization and error handling, ensuring seamless data exchange between the smart home system and the controlling host.

## Hardware Requirements
- Arduino Uno
- Adafruit RGB LCD Shield
- Other smart home device components (sensors, actuators, etc.)

## Software Dependencies
- `Wire.h`
- `Adafruit_RGBLCDShield.h`
- `Adafruit_MCP23017.h`

## Usage
- The system starts in the 'Waiting' state and transitions through various states based on inputs and internal logic.
- Serial communication can be used to interact with the system, sending commands and receiving updates.


