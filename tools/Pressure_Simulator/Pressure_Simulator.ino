/*
Sends pressure frames at 10 hz

=== Frame details ===
ID:     0x513
DLC:    0x8 (bytes)
Byte 0: Sensor 0, least significant byte
Byte 1: Sensor 0, most significant byte
Byte 2: Sensor 1, least significant byte
Byte 3: Sensor 1, most significant byte
Byte 4: Sensor 2, least significant byte
Byte 5: Sensor 2, most significant byte
Byte 6: Sensor 3, least significant byte
Byte 7: Sensor 3, most significant byte

All readings are in Pascals, with an offset of 70000.

e.g. if you get the following data for a sensor:
Least significant byte: 0x30
Most significant byte:  0x75
1) Reassemble them into a 16-bit value: 0x7530
2) Convert to decimal: 0x7530 = 30000
3) Add the offset to get the pressure: 30000 + 70000 = 100000

=== Expected readings (after applying the offset) ===
Sensor 0:  70000 (absolute minimum)
Sensor 1: 135535 (absolute maximum)
Sensor 2: 100000 (1 Bar exactly)
Sensor 3: Sawtooth wave (rising 1000 Pa every second)
*/

#include <SPI.h>
#include <mcp2515.h>  // https://github.com/autowp/arduino-mcp2515 (or install from the library manager)


#define CAN_MODULE_CS 10
#define UNO_SPI_MOSI  11
#define UNO_SPI_MISO  12
#define UNO_SPI_SCK   13
#define MEGA_SPI_MOSI 50
#define MEGA_SPI_MISO 51
#define MEGA_SPI_SCK  52

// This is subtracted from the pressure so we can pack it into two bytes.
const uint32_t CAN_FRAME_PRESSURE_OFFSET = 70000; 

// Used to suppress spurious output from connected but unused pressure sensors.
// These pins a pulled HIGH at boot
const uint8_t UNUSED_SPI_CS_PINS[] = {5, 6, 7, 8 , 9};

const CAN_SPEED CAN_BUS_SPEED = CAN_1000KBPS;
const CAN_CLOCK CAN_BUS_CLOCK = MCP_20MHZ;

const uint32_t CAN_MEASUREMENT_SPACING_MS = 100;

MCP2515 can_module(CAN_MODULE_CS);


void setup() {
	//  Set up the serial port
	while (!Serial);  // wait for native usb if necessary
	Serial.begin(115200);

	Serial.println();
	Serial.println(F("Pressure simulator starting..."));

	// Disable unused pressure sensors
	for (uint8_t i=0; i<sizeof(UNUSED_SPI_CS_PINS); i++){
		Serial.print(F("Disabling SPI device on pin ")); Serial.println(UNUSED_SPI_CS_PINS[i]);
		pinMode(UNUSED_SPI_CS_PINS[i], INPUT_PULLUP);
	}
	
	// Set up MCP2515 module
	can_module.reset();
	MCP2515::ERROR set_can_bitrate_result = can_module.setBitrate(CAN_BUS_SPEED, CAN_BUS_CLOCK);
	can_module.setNormalMode();
	
	if (set_can_bitrate_result == MCP2515::ERROR_OK) {
		Serial.println(F("Sucessfully set CAN bitrate"));
	}
	else if (set_can_bitrate_result != MCP2515::ERROR_OK) {
		Serial.println(F("Failed to set CAN bitrate. Is the MCP2515 connected?"));  // TODO: retry until successful
	}
	Serial.println(F("Finished setup"));
}


void loop() {
	static uint32_t last_transmission_time = 0;
	uint32_t now = millis();
	if ((now % CAN_MEASUREMENT_SPACING_MS == 0 && now != last_transmission_time) || now - last_transmission_time >= CAN_MEASUREMENT_SPACING_MS) {
		last_transmission_time = now - (now % CAN_MEASUREMENT_SPACING_MS);
		
		init_pressure_frame(0x513);
		add_pressure(CAN_FRAME_PRESSURE_OFFSET, 0);                  // Min
		add_pressure(CAN_FRAME_PRESSURE_OFFSET + 65535, 1);          // Max
		add_pressure(100000, 2);                                     // 1 bar
		add_pressure(CAN_FRAME_PRESSURE_OFFSET + (now % 65535), 3);  // Sawtooth
		send_pressure_frame(&can_module);
	}
}
