# ICT
[Version française](LISEZMOI.md)

ICT is a tester for integrated circuits
## Presentation
ICT allows to identify and verify integrated circuits of the [4000](https://en.wikipedia.org/wiki/List_of_4000-series_integrated_circuits) and [7400](https://en.wikipedia.org/wiki/List_of_7400-series_integrated_circuits) series.

It also allos to test memory components.
- Dynamic memories 4116, 4164, 41256, 4416 and 44256.
- Static memories 2114, 6116 and 61256.
- Read-only memories 2764, 27128 and 27256.

It is possible to handle new components by adding definitions into the files of parameters.

ICT is inspired by [Smart IC Tester](https://www.instructables.com/id/Smart-IC-Tester/) project from Akshay Baweja.

## Installation
ICT is made of several items.
### Hardware
- An [Arduino Mega 2560](https://store.arduino.cc/arduino-mega-2560-rev3) card.
- A TFT touch screen with integrated micro SD card reader.
- The ICT version 2.0 extension card.
- A micro SD card.

As for the screen, there are many models with different controllers. It will therefore be necessary to adapt the management of the screen.
The screen used is perfectly managed by the MCUFRIEND_kbv library for display and the Adafruit TouchScreen library for touch.

The files **logic.ict**, **memory.ict** and **ict.ini** must be copied to the root of the micro SD card.
### Software
To develop the application, the Eclipse IDE, the Sloeber plugin and the official Arduino core are used:
- [Eclipse CDT](https://www.eclipse.org/cdt/) 2020-06
- [Sloeber](https://eclipse.baeyens.it/) 4.3.3
- Official core [Arduino AVR](https://github.com/arduino/ArduinoCore-avr) 1.8.3
> It is however possible to use the Arduino IDE to compile the application.

#### Libraries
The application uses several libraries:
- Adafruit BusIO 1.7.2
- Adafruit GFX Library 1.10.4
- Adafruit TouchScreen 1.1.1
- MCUFRIEND_kbv 2.9.9
- SdFat 1.1.4 ou 2.0.4
#### Modifications
It is necessary to modify the file ** SdFatConfig.h ** of the SdFat library to allow access to the micro SD card.

In version 1.1.4, you must set `ENABLE_SOFTWARE_SPI_CLASS` to 1:
```cpp
#define ENABLE_SOFTWARE_SPI_CLASS 1
```
In version 2.0.4, you must set `SPI_DRIVER_SELECT` to 2:
```cpp
#define SPI_DRIVER_SELECT 2
```
> In case of problem accessing the SD card, it is recommended to format the SD card with the [SD Card Formatter](https://www.sdcard.org/downloads/formatter/) tool.

If your screen uses an HX8347 component, it is necessary to uncomment a line of the **MCUFRIEND_kbv.cpp** file to activate support for this component.
```cpp
#define SUPPORT_8347D
```
In addition, depending on the screen used, the code may require adaptations to manage orientation and touch.
These portions of code are indicated by a comment.
```cpp
// TODO adapt code to your display
```
To improve touch accuracy, it is possible to increase the number of samples used in the **TouchScreen.cpp** file.
```cpp
#define NUMSAMPLES 4
```
## Usage
- The normal orientation of the tester in use is screen at the bottom, ZIF support at the top.
- Pin 1 of the component to be tested is positioned at the top, to the right of the ZIF support, towards the locking lever, as indicated by the pictogram, to the right of the ZIF support.
- Markings on the PCB at the upper edge of the ZIF support indicate the position of the component according to the number of pins.
- The push button to the right of the ZIF support is a user button that allows you to interrupt an operation.
- The push button at the top left below the screen is a reset button.

### Identification of logic circuits
- Insert the circuit to be identified in the ZIF support.
- Select the option **identify logic**.
- On the next screen, select the number of pins, 14 or 16 of the circuit.
- Component identification is performed.
### Logic circuit test
- Insert the circuit to be tested in the ZIF socket.
- Select the option **test logic**.
- On the next screen, enter the code of the circuit to be tested. For example ** 74139 ** for a component **74LS139**.
- Click on the **ENTER** button.
- The circuit test is carried out. To interrupt the operation, tap the screen.
### RAM component test
- Insert the component to be tested in the ZIF support.
- Select the option **RAM test**.
- On the next screen, enter the code of the component to be tested. For example **4164** or **tms4164** for a **TMS4164** component.
- Click on the **ENTER** button.
- Component test is performed.
> It is possible to enter alphanumeric RAM component codes.
### ROM component test
- Insert the component to be tested in the ZIF support.
- Select the option **ROM test**.
- On the next screen, enter the code of the component to be tested. For example **2764** or **am27c64** for an **AM27C64** component.
- Click on the **ENTER** button.
- On the next screen, select the options:
    - Break if the memory is not blank.
    - Dump content to the serial monitor.
    - Dump content to SD card.
- Click on the **CONTINUE** button.
- Component test is performed.
> It is possible to enter alphanumeric ROM component codes.
## Troubleshooting
In general, if the ICT extension does not work properly, it is necessary to make each element work step by step.
You have to start by using the screen on the Arduino Mega, without the ICT extension.
### Screen operation
If you have a white screen, the screen may not be handled by default.
On the serial monitor, the screen component identifier is displayed:
```batch
TFT initialized 0x9341
```
You can search for this identifier and check if it is possible to activate its support in the **MCUFRIEND_kbv** library.
Another solution is to use the **diagnose_TFT_support.ino** sketch in the examples of the **MCUFRIEND_kbv** library and to follow the instructions.
### Touch operation
The **TouchScreen_Calibr_native.ino** example sketch of the **MCUFRIEND_kbv** library is used to configure and calibrate the touch screen.
### SD operation
Problems accessing the SD card, apart from a correct configuration of the ** SdFat ** library, are linked to incompatible formatting of the card.

Once every element works, the last step is to integrate the extension. At this stage, the only point that can cause problems is improper insertion of the extension.### Fonctionnement de la SD
## Links
For help or to obtain the ICT extension, you can consult the thread [[Arduino] ICT testeur de circuits intégrés](https://forum.system-cfg.com/viewtopic.php?f=18&t=11417), from the System.cfg site forum.