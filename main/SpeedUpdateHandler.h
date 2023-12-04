
#ifndef SPEEDUPDATEHANDLER_H
#define SPEEDUPDATEHANDLER_H

#include <mutex>
#include "state.h"
#include <memory>
#include <freertos/FreeRTOS.h>
#include <soc/adc_channel.h>
#include <driver/adc.h>
#include <esp_adc_cal.h>
#include <freertos/task.h>

class SpeedUpdateHandler {
    public:
        SpeedUpdateHandler(adc1_channel_t speedPin, std::shared_ptr<StateMachine> aStateMachine, uint32_t maxDriverFreq);
        uint32_t GetNormalSpeed();
        uint32_t GetRapidSpeed();
        static void UpdateTask(void* params);
        void UpdateSpeeds();

    private:
        uint32_t myMaxDriverFreq;
        TaskHandle_t updateTaskHandle;
        std::shared_ptr<StateMachine> myStateMachine;
        adc1_channel_t speedPin;
        uint32_t setSpeedADC = 0;
        std::mutex setSpeedMutex;
        uint32_t newSpeedADC = 0;
        std::mutex newSpeedMutex;
        uint32_t rapidSpeed = 0;
        /*******Averaging filter stuff********/
        #define WINDOW_SIZE 8
        uint32_t INDEX = 0;
        uint32_t VALUE = 0;
        uint32_t SUM = 0;
        uint32_t READINGS[WINDOW_SIZE];
        uint32_t AVERAGED = 0;
        /*************************************/

        uint32_t mapAdcToSpeed (uint16_t value, uint16_t inMin, uint16_t inMax, uint16_t outMin, uint16_t outMax) {
            return (value - inMin) * (outMax - outMin) / (inMax - inMin) + outMin;
        };
};

#endif // SPEEDUPDATEHANDLER_H
