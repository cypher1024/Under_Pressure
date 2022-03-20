#include <SPI.h>
#include <mcp2515.h>

struct can_frame canMsg;
MCP2515 mcp2515(10);


const uint32_t CAN_FRAME_PRESSURE_OFFSET = 70000;
// TODO: Rate limit


void setup() {
	Serial.begin(115200);
	Serial.println(F("Test CAN Reader starting..."));
	
	mcp2515.reset();
	MCP2515::ERROR set_can_bitrate_result = mcp2515.setBitrate(CAN_1000KBPS, MCP_20MHZ);
	mcp2515.setNormalMode();
	
	if (set_can_bitrate_result == MCP2515::ERROR_OK) {
		Serial.println(F("Sucessfully set CAN bitrate"));
	}
	else if (set_can_bitrate_result != MCP2515::ERROR_OK){
		Serial.println(F("Failed to set CAN bitrate. Is the MCP2515 connected?"));
	}
	Serial.println(F("Finished setup"));
}

void loop() {
	if (mcp2515.readMessage(&canMsg) == MCP2515::ERROR_OK) {
		Serial.print(F("Received - ID: ")); Serial.print(canMsg.can_id, HEX);
		Serial.print(F(", DLC: "));  Serial.print(canMsg.can_dlc, HEX); // print DLC
		Serial.print(F(", Data: "));
		for (int i = 0; i<canMsg.can_dlc; i++){
			Serial.print(F("0x"));
			if (canMsg.data[0] <= 0xF){
				Serial.print(F("0"));
			}
			Serial.print(canMsg.data[i], HEX); Serial.print(F(" "));
		}
		Serial.println();
		
		// If the ID is one of our pressure frames, convert and print the pressure
		if (canMsg.can_id == 0x513 || canMsg.can_id == 0x514){
			for (int i=0; i<4; i++){
				uint32_t pressure = canMsg.data[2*i];
				pressure |= ((uint32_t)canMsg.data[2*i+1]) << 8;
				pressure += CAN_FRAME_PRESSURE_OFFSET;
				Serial.print("Pressure "); Serial.print(i); Serial.print(": "); Serial.print(pressure, DEC); 
				if (i < 3){
					Serial.print(F(", "));
				}
			}
		}
		Serial.println();
		Serial.println();
	}
}
