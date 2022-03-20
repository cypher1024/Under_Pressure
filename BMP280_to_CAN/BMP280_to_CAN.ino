/*
 * ====== NOTES =========
 * 
 * 
 * BMP280 SPI pinout
 *  SCL=SCK
 *  SDA=MOSI
 *  CSB=CS
 *  SDO=MISO
 *  
 * Disabling temperature measurements will allow faster pressure sampling
 * 
 * Might need to disable the IIR filter to get accurate pressure readings (it tries to filter out things like slamming a car door)
 * 
 * With pressure oversampling at X2 and temp oversampling at x1, the IIR filter off, and timing at 1ms (0.5ms in the datasheet), output data rate is 125Hz
 * 
 * Read time for a single unit (with the old float method) seems to be <= 2ms, and reading 5 seems to be <= 4ms
 * 
 * 
 * 
 * ========== TODO ==============
 * 
 * Triggerable calibration routine that saves to flash?
 * Write a library to handle multiple sensors?
 * Retry connection to sensors if a connection attempt fails, but proceed with the sketch if at least one succeeds. Might need to spoof a value for dead sensors.
 * Maybe keep trying to connect to any missing sensors?
 * Keep trying to connect to CAN module if the first attempt fails. 
 * Status LED output (need sequences for bootup, error, and maybe flash on each set of frames sent). Can't use built-in LED because it's used for SPI clock
 * 
 */

// SPI Library is used by the MCP2515
#include <SPI.h>

// Adafruit_BMP280_Library (with integer pressure function added, included with this project)
// Note that it also requires Adafruit_Sensor and Adafruit_BusIO. The easiest thing is to 
// just install the unmodified Adafruit_BMP280 library via the Arduino library manager,
// and it will take care of these prerequisites.
#include "src/Modified_Adafruit_BMP280_Library/Adafruit_BMP280.h"

// https://github.com/autowp/arduino-mcp2515
#include <mcp2515.h>          


// Pin assignments (note that the SPI MOSI/MISO/SCK are just provided for reference)
Adafruit_BMP280 pressure_sensors[] = {
	Adafruit_BMP280(5),
	Adafruit_BMP280(6),
	Adafruit_BMP280(7),
	Adafruit_BMP280(8),
	Adafruit_BMP280(9)
};
#define CAN_MODULE_CS 10
#define UNO_SPI_MOSI  11
#define UNO_SPI_MISO  12
#define UNO_SPI_SCK   13
#define MEGA_SPI_MOSI 50
#define MEGA_SPI_MISO 51
#define MEGA_SPI_SCK  52

const CAN_SPEED CAN_BUS_SPEED = CAN_1000KBPS;
const CAN_CLOCK CAN_BUS_CLOCK = MCP_20MHZ;

const uint32_t CAN_MEASUREMENT_SPACING_MS = 100;  // Time between sets of CAN frames
const uint32_t FIRST_PRESSURE_FRAME_ID = 0x513;   // ID for the first 4 sensors. The next 4 will be sent as 0x514

size_t sensor_count = sizeof(pressure_sensors) / sizeof(pressure_sensors[0]);

MCP2515 can_module(CAN_MODULE_CS);


void setup() {
    //  Set up the serial port
	while (!Serial);  // wait for native usb if necessary
	Serial.begin(115200);

	Serial.println();
	Serial.println(F("CAN Bus Pressure Sensors starting..."));
	bool status;
	for (int i=0; i<sensor_count; i++){
		status = pressure_sensors[i].begin();
		if (!status) {
			Serial.print(F("Could not find a valid BMP280 sensor for sensor ")); Serial.println(i);
			Serial.print("SensorID was: 0x"); Serial.println(pressure_sensors[i].sensorID(), 16);
			Serial.print("        ID of 0xFF probably means a bad address, a BMP 180 or BMP 085\n");
			Serial.print("   ID of 0x56-0x58 represents a BMP 280,\n");
			Serial.print("        ID of 0x60 represents a BME 280.\n");
			Serial.println("        ID of 0x61 represents a BME 680.\n");
			Serial.println(F("Halting..."));
			while (1) delay(10);
		}

		// Sensor settings
		pressure_sensors[i].setSampling(
			Adafruit_BMP280::MODE_NORMAL,    /* Operating Mode. Normal mode constantly samples */
			Adafruit_BMP280::SAMPLING_NONE,  /* Temp. oversampling. SAMPLING_NONE/X1/X2/X4/X8/X16 */
			Adafruit_BMP280::SAMPLING_X8,    /* Pressure oversampling SAMPLING_NONE/X1/X2/X4/X8/X16 */
			Adafruit_BMP280::FILTER_OFF,     /* Filtering. FILTER_OFF/X2/X4/X8/X16 */
			Adafruit_BMP280::STANDBY_MS_1    /* Standby time. 1/63/125/250/500/1000/2000/4000 */
		); 	
	}

	// Set up MCP2515 module
    can_module.reset();
    MCP2515::ERROR set_can_bitrate_result = can_module.setBitrate(CAN_BUS_SPEED, CAN_BUS_CLOCK);
    can_module.setNormalMode();
	
    if (set_can_bitrate_result == MCP2515::ERROR_OK) {
        Serial.println(F("Sucessfully set CAN bitrate"));
    }
    else if (set_can_bitrate_result != MCP2515::ERROR_OK){
        Serial.println(F("Failed to set CAN bitrate. Is the MCP2515 connected?"));  // TODO: retry until successful
    }
    Serial.println(F("Finished setup"));
}


void loop() {
	static uint32_t last_transmission_time = 0;

	// To mitigate drift, we use two checks to decide when to send frames.
	// The first check tries to send every time millis() is evenly divisible by CAN_MEASUREMENT_SPACING_MS.
	// So if CAN_MEASUREMENT_SPACING_MS is 100, we would try to send at the 100 mark, then the 200 mark, etc.
	// As a backup, we will also send if CAN_MEASUREMENT_SPACING_MS has elapsed since the last transmission.
	// This is to avoid skipped transmissions if we can't hit the CAN_MEASUREMENT_SPACING_MS mark.
	// If this happens, we set last_transmission_time to the time we were trying to hit.
	uint32_t now = millis();
	if ((now % CAN_MEASUREMENT_SPACING_MS == 0 && now != last_transmission_time) || now - last_transmission_time >= CAN_MEASUREMENT_SPACING_MS){
		last_transmission_time = now - (now % CAN_MEASUREMENT_SPACING_MS);
		
		// Initialise the first frame
		init_pressure_frame(FIRST_PRESSURE_FRAME_ID);
		
		for (int i=0; i<sensor_count; i++){
			// Add each pressure reading
			add_pressure(pressure_sensors[i].readPressureInt(), i % 4);
			// If the frame is full, (4 readings), send it and start a new one with the next ID
			if (i % 4 == 3){
				send_pressure_frame(&can_module);
				init_pressure_frame(FIRST_PRESSURE_FRAME_ID + ((i + 1) / 4));
			}
		}
		
		// If there's data waiting in the last frame, send it
		if (sensor_count % 4 != 0){
			send_pressure_frame(&can_module);		
		}
	}
}
