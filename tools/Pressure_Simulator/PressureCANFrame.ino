struct can_frame pressure_frame;


void init_pressure_frame(uint32_t can_id){
	pressure_frame.can_id = can_id;
	pressure_frame.can_dlc = 8;
	pressure_frame.data[0] = 0x0;
	pressure_frame.data[1] = 0x0;
	pressure_frame.data[2] = 0x0;
	pressure_frame.data[3] = 0x0;
	pressure_frame.data[4] = 0x0;
	pressure_frame.data[5] = 0x0;
	pressure_frame.data[6] = 0x0;
	pressure_frame.data[7] = 0x0;
}


void add_pressure(uint32_t pressure, uint8_t pos){
	// Max 4 pressures per frame (two bytes each).
	// Pressures are stored with the least significant byte first, and the more significant byte second.
	// Note that 'pos' is the pressure index (i.e. 0-3), not the start byte!
	if (pos > 3) return;
	Serial.println();
	Serial.print(F("Got pressure ")); Serial.print(pressure); Serial.print(F(" for pos ")); Serial.println(pos);
	
	// Offset the pressure so it fits into two bytes
	uint16_t under_pressure = 0; // Values below CAN_FRAME_PRESSURE_OFFSET are capped at 0
	if (pressure > CAN_FRAME_PRESSURE_OFFSET){
		Serial.println("pressure > CAN_FRAME_PRESSURE_OFFSET");
		pressure -= CAN_FRAME_PRESSURE_OFFSET;
		Serial.print("After subtracting the offset: "); Serial.println(pressure);
		if (pressure <= 65535){
			under_pressure = (uint16_t)pressure;
			Serial.print("Pressure <= 65535. Casting to 16 bit int... Result: "); Serial.print(under_pressure);
		}
		else{
			under_pressure = 65535;  // Values above CAN_FRAME_PRESSURE_OFFSET + 65535 are capped at 65535
			Serial.println("Pressure > 65535, setting max");
		}
	}

	// Load it into the CAN frame
	pressure_frame.data[pos*2] = under_pressure & 0xFF;
	pressure_frame.data[(pos*2)+1] = (under_pressure >> 8) & 0xFF;
}


void send_pressure_frame(MCP2515 *can_module){
	can_module->sendMessage(&pressure_frame);
}
