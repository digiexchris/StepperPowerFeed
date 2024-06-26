#pragma once

#include "state.h"
#include "EventTypes.h"
#include <esp_event.h>
#include <memory>
#include "Screen.h"
#include <led_strip.h>
#include "Event.h"
#include "Settings.h"
#include "state.h"

class Screen;

class UI : public EventPublisher, EventHandler
{
  public:

	UIState myUIState;
	
	UI(gpio_num_t sdaPin, 
	   gpio_num_t sclPin, 
	   i2c_port_t i2cPort, 
	   uint i2cClkFreq, 
	   gpio_num_t anEncAPin, 
	   gpio_num_t aEncBPin, 
	   gpio_num_t aButtonPin,
	   int32_t aSavedNormalSpeed,
	   int32_t aSavedRapidSpeed,
	   SpeedUnit aSavedSpeedUnits);
	void Update();
	static void UpdateTask(void *pvParameters);
	void Start();
	static void ProcessEventCallback(void *aUi, esp_event_base_t base, int32_t id, void *payload);

	void ProcessEvent(Event event, EventData* eventData);

  private:
	static std::shared_ptr<UI> myRef;
	void ToggleUnitsButton();
	void ToggleUnits();
	static void CheckAndSaveSettingsCallback(void *param);
	static void ToggleUnitsButtonTask(void *params);
	led_strip_handle_t configureLed(gpio_num_t anLedPin);
	std::unique_ptr<Screen> myScreen;
	int32_t myNormalSpeed;
	int32_t myRapidSpeed;
	SpeedUnit mySpeedUnits;
	bool myIsRapid;
	led_strip_handle_t* myLedHandle;
	gpio_num_t myButtonPin;
};