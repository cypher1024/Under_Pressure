This project includes code from https://github.com/adafruit/Adafruit_BMP280_Library

Its licence can be found in the .cpp and .h files inside the src/Modified_Adfruit_BMP280_Library directory (or in their GitHub repo).

# Under Pressure

## An Arduino project to read pressures from Bosch BMP280 pressure sensors and send them out as CAN frames (via a cheap MCP2515 module)

**Contents:**
- BMP280_to_CAN - this is probably what you want
- tools/Pressure_Simulator - useful for CAN setup/debugging. It simulates four pressure sensors. All you need is an Arduino and MCP2515
- tools/Test_CAN_Reader - also useful for debugging. It listens for pressure frames, (IDs 0x513 and 0x514), decodes them, and prints them to serial

**Getting started:**
- Clone this repo or download it as a .zip and extract it
- Use the Arduino Library Manager to install the Adafruit_BMP280 library, (we don't use it directly, but we need the prerequisites it installs)
- This project defaults to a 1 Mbps CAN baud rate, which the cheap MCP2515 modules' 8 MHz cystals don't support. You can either swap a 20 MHz crystal and 15 pF capacitors onto the MCP2515 module, or reduce CAN_BUS_SPEED below 1 Mbps and set CAN_BUS_CLOCK to 8 MHz 
- Compile the firmware and flash it to your arduino of choice
- Connect the BMP280s and MCP2515 using the pin assignments in the code

Datasheet for the MCP2515: http://ww1.microchip.com/downloads/en/DeviceDoc/MCP2515-Stand-Alone-CAN-Controller-with-SPI-20001801J.pdf
Datasheet for the BMP280: https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bmp280-ds001.pdf
