/**********************************************************************************************************************
 * Includes
 *********************************************************************************************************************/
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "FreeRTOSConfig.h"
#include "cmsis_os2.h"
#include "gpio_driver.h"
#include "debug_api.h"
#include "heap_api.h"
#include "led_api.h"
#include "led_app.h"
/**********************************************************************************************************************
 * Private definitions and macros
 *********************************************************************************************************************/
#define LED_BLINK_TIMER_NAME "LedBlinkTimer"
#define LED_BLINK_TIMER_MUTEX_NAME "LedBlinkMutex"
#define MUTEX_TIMEOUT_MS 10
/**********************************************************************************************************************
 * Private typedef
 *********************************************************************************************************************/
typedef enum eLedState {
    eLedState_First = 0,
    eLedState_On = eLedState_First,
    eLedState_Off,
    eLedState_Last
} eLedState_t;

typedef struct sLedBlinkArgs {
    eLed_t led;
    size_t num_of_calls;
    uint32_t ticks;
    osMutexId_t mutex_id;
    osTimerId_t timer_id;
    void (*callback_func)(eLed_t led);
} sLedBlinkArgs_t;

typedef struct sLedProperties {
    eGPIODriver_t gpio_pin;
    bool is_inverted;
} sLedProperties_t;

typedef struct sTimerProperties {
    sLedBlinkArgs_t blinker_timer_args;
} sTimerProperties_t;
/**********************************************************************************************************************
 * Private constants
 *********************************************************************************************************************/
CREATE_MODULE_TAG (LED_API);
static const sLedProperties_t g_static_led_lut[eLed_Last] = {
    [eLed_Status] = {.gpio_pin = eGPIODriver_StatusLedPin, .is_inverted = false},
    [eLed_GpsFix] = {.gpio_pin = eGPIODriver_LedGpsFixPin, .is_inverted = false}
};
const static osTimerAttr_t g_led_blink_task_attr = {
    .name = LED_BLINK_TIMER_NAME
};
const static osMutexAttr_t g_led_blink_mutex_attr = {
    .name = LED_BLINK_TIMER_MUTEX_NAME
};
/**********************************************************************************************************************
 * Private variables
 *********************************************************************************************************************/
static sTimerProperties_t g_dynamic_led [eLed_Last] = {0};
/**********************************************************************************************************************
 * Exported variables and references
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Prototypes of private functions
 *********************************************************************************************************************/
static void LED_API_LedBlinkTimerTask (void *args);
/**********************************************************************************************************************
 * Definitions of private functions
 *********************************************************************************************************************/
static void LED_API_LedBlinkTimerTask (void *args) {
    if (args == NULL) {
        DEBUG_INFO("No LED blinker timer task arguments!\r\n");
        return;
    }

    sLedBlinkArgs_t *timer_args = (sLedBlinkArgs_t *) args;
    if (osMutexAcquire(timer_args->mutex_id, MUTEX_TIMEOUT_MS) != osOK) {
        DEBUG_ERROR("Failed to acquire a mutex!\r\n");
        return;
    }

    LED_API_LedToggle(timer_args->led);
    timer_args->num_of_calls--;
    bool return_val = true;

    if (timer_args->num_of_calls <= 0) {
        if (timer_args->callback_func != NULL) {
            timer_args->callback_func(timer_args->led);
        }

        return_val = LED_API_LedOff(timer_args->led);

        if (return_val == false) {
            DEBUG_WARN("Failed to turn off the LED!\r\n");
        }
        else {
            return_val = false;
        }
    }

    if ((return_val == true) && (osTimerStart(timer_args->timer_id, timer_args->ticks) != osOK)){
        DEBUG_ERROR("Failed to restart the led blink timer!\r\n");
        return;
    }

    if (osMutexRelease(timer_args->mutex_id) != osOK) {
        DEBUG_ERROR("Failed to release a mutex!\r\n");
        return;
    }
}
/**********************************************************************************************************************
 * Definitions of exported functions
 *********************************************************************************************************************/
bool LED_API_LedInit (void) {
    for (eLed_t led = eLed_First; led < eLed_Last; led++) {
        if (GPIO_Driver_Init() == false) {
            return false;
        }

        g_dynamic_led[led].blinker_timer_args.mutex_id = osMutexNew(&g_led_blink_mutex_attr);

        if (g_dynamic_led[led].blinker_timer_args.mutex_id == NULL) {
            DEBUG_ERROR("Failed to create a mutex!\r\n");
            return false;
        }

        if (g_dynamic_led[led].blinker_timer_args.timer_id == NULL) {
            g_dynamic_led[led].blinker_timer_args.timer_id = osTimerNew(&LED_API_LedBlinkTimerTask, 
                                                                        osTimerOnce,
                                                                        &g_dynamic_led[led].blinker_timer_args,
                                                                        &g_led_blink_task_attr);
        }
        else if (g_dynamic_led[led].blinker_timer_args.timer_id == NULL) {
            DEBUG_ERROR("Failed to crate a led blink timer task!\r\n");
            return false;
        }

        g_dynamic_led[led].blinker_timer_args.ticks = 0;
        g_dynamic_led[led].blinker_timer_args.num_of_calls = 0;
    }

    return true;
}

bool LED_API_LedOn (eLed_t led) {
    if ((led < eLed_First) || (led >= eLed_Last)) {
        DEBUG_INFO("LED not found!\r\n");
        return false;
    }

    eLedState_t led_state = (g_static_led_lut[led].is_inverted == false) ? eGPIO_PinState_High : eGPIO_PinState_Low;

    if (GPIO_Driver_Write(g_static_led_lut[led].gpio_pin, led_state) == false) {
        DEBUG_ERROR("Failed to turn on the LED!\r\n");
        return false;
    }

    return true;
}

bool LED_API_LedOff (eLed_t led) {
    if ((led < eLed_First) || (led >= eLed_Last)) {
        DEBUG_INFO("LED not found!\r\n");
        return false;
    }

    eLedState_t led_state = (g_static_led_lut[led].is_inverted == false) ? eGPIO_PinState_Low : eGPIO_PinState_High;

    if (GPIO_Driver_Write(g_static_led_lut[led].gpio_pin, led_state) == false) {
        DEBUG_ERROR("Failed to turn on the LED!\r\n");
        return false;
    }

    return true;
}

bool LED_API_LedToggle (eLed_t led) {
    if ((led < eLed_First) || (led >= eLed_Last)) {
        DEBUG_INFO("LED not found!\r\n");
        return false;
    }

    if (GPIO_Driver_Toggle(g_static_led_lut[led].gpio_pin) == false) {
        return false;
    }
    
    return true;
}

bool LED_API_LedBlink (eLed_t led, size_t blink_freq, size_t blinks_num, void (*callback_func)(eLed_t led)) {
    if ((led < eLed_First) || (led >= eLed_Last) || (blink_freq == 0) || (blinks_num == 0)) {
        DEBUG_INFO("Command arguments are not valid!\r\n");
        return false;
    }

    if (osMutexAcquire(g_dynamic_led[led].blinker_timer_args.mutex_id, MUTEX_TIMEOUT_MS) != osOK) {
        DEBUG_ERROR("Failed to acquire a mutex!\r\n");
        return false;
    }

    g_dynamic_led[led].blinker_timer_args.led = led;
    g_dynamic_led[led].blinker_timer_args.ticks = (osKernelGetTickFreq() / blink_freq) / 2;
    g_dynamic_led[led].blinker_timer_args.num_of_calls = 2 * blinks_num;
    g_dynamic_led[led].blinker_timer_args.callback_func = callback_func;

    if (g_dynamic_led[led].blinker_timer_args.timer_id != NULL) {
        if (osTimerStart(g_dynamic_led[led].blinker_timer_args.timer_id, 
                         g_dynamic_led[led].blinker_timer_args.ticks) != osOK){
            DEBUG_ERROR("Failed to start the led blink timer!\r\n");
            return false;
        }
    }

    if (osMutexRelease(g_dynamic_led[led].blinker_timer_args.mutex_id) != osOK) {
        DEBUG_ERROR("Failed to release mutex!\r\n");
        return false;
    }

    return true;
}
