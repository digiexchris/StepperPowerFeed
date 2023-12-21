#pragma once

#include "state.h"
#include <driver/i2c.h>
#include <u8g2.h>
#include <memory>

extern "C"
{
#include <u8g2_esp32_hal.h>
};

class Screen
{

  public:
	enum class SpeedUnit
	{
		MMPM,
		IPM
	} mySpeedUnit;

	Screen(gpio_num_t sdaPin, gpio_num_t sclPin, i2c_port_t i2cPort, uint32_t i2cClkFreq = 100000);
	void SetSpeed(uint32_t aSpeed);
	void SetState(UIState aState);
	void SetUnit(SpeedUnit aUnit);
	
	
	void SetSpeedState(SpeedState aSpeedState);
	void Start();

  private:
	uint16_t mySpeed;
	uint16_t myPrevSpeed;
	UIState myState;
	UIState myPrevState;
	SpeedState mySpeedState;
	SpeedState myPrevSpeedState;
	SpeedUnit myPrevSpeedUnit;
	u8g2_t u8g2;
	u8g2_esp32_hal_t u8g2_esp32_hal;
	
	static Screen* myRef;
	
	static void UpdateTask(void *pvParameters);
	void Update();
	void DrawSpeed();
	void DrawSpeedUnit();
	void DrawState();
	void DrawUnit();

	float SpeedPerMinute();
};