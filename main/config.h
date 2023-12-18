#ifndef CONFIG_H
#define CONFIG_H

#include <driver/adc.h>
#ifndef __IDF_VER__4_X_X
//#include <driver/adc_types_legacy.h>
#else
#include <driver/adc.h>
#endif

//#include <ssd1306.h>

#define DEBUG_ABORTS 1

#define dirPinStepper 4
#define enablePinStepper 5
#define stepPinStepper 6
#define LEFTPIN GPIO_NUM_35
#define RIGHTPIN GPIO_NUM_38
#define RAPIDPIN GPIO_NUM_36
#define ENCODER_A_PIN GPIO_NUM_17
#define ENCODER_B_PIN GPIO_NUM_18
#define ENCODER_BUTTON_PIN GPIO_NUM_8
#define PIEZO_PIN GPIO_NUM_2
#define ACCELERATION 20000 //steps/s/s
#define DECELERATION 200000

#define I2C_MASTER_SCL_IO 47	  /*!< gpio number for I2C master clock */
#define I2C_MASTER_SDA_IO 48	  /*!< gpio number for I2C master data  */
#define I2C_MASTER_NUM I2C_NUM_1  /*!< I2C port number for master dev */
#define I2C_MASTER_FREQ_HZ 100000 /*!< I2C master clock frequency */

const adc1_channel_t speedPin = ADC1_CHANNEL_6;  //front knob pot, GPIO7 on the S3
const double MAX_DRIVER_STEPS_PER_SECOND = 13000; // 20kHz max pulse freq in hz at 25/70 duty cycle, 13kHz at 50/50. FastAccelStepper is doing 50/50@13 :(
const int stepsPerRev = 200;
const double mmPerRev = 0.25 * 25.4 * 1.5 * 4; // 4 tpi lead screw, with 2 reductions
const double mmPerStep = mmPerRev / stepsPerRev;
const double stepsPerMm = stepsPerRev / mmPerRev;
const double MAX_SPEED = MAX_DRIVER_STEPS_PER_SECOND / stepsPerMm;


//#define USE_DENDO_STEPPER 1
    //(13,000 - 0) / 20,000
    const double FULL_SPEED_ACCELERATION_LINEAR_TIME = 1000*(MAX_DRIVER_STEPS_PER_SECOND / ACCELERATION);
    const double FULL_SPEED_DECELERATION_LINEAR_TIME = 1000*(MAX_DRIVER_STEPS_PER_SECOND / DECELERATION);

    // #define ESP_LOGI(tag, format, ...) printf(format, ##__VA_ARGS__)
    // #define ESP_LOGE(tag, format, ...) printf(format, ##__VA_ARGS__)

#ifdef USE_DENDO_STEPPER

#elif USE_FASTACCELSTEPPER
const int acceleration = 20000;
const int deceleration = 20000;
#endif



#endif // SHARED_H
