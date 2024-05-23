
#ifndef __MX_SOC_GPIO_H_
 #define __MX_SOC_GPIO_H_

#include <stdint.h>
#include <stdbool.h>


enum pin_mode_e
{
    PIN_MODE_OUT,
    PIN_MODE_OUT_OPEN_DRAIN,
    PIN_MODE_OUT_OPEN_DRAIN_PULLUP,
    PIN_MODE_IN,
    PIN_MODE_IN_FILTER,
    PIN_MODE_IN_PULLUP,
};

enum pin_state_e
{
  PIN_ST_DOWN = 0,
  PIN_ST_UP
};


struct pin_drv;

typedef void (*pin_irq_callback)(bool state);   // Called from interrupt context

void pin_open(struct pin_drv *pin, int8_t mode, int8_t initial_state);
void pin_close(struct pin_drv *pin);
void pin_set_irq_callback(struct pin_drv *pin, pin_irq_callback callback);
void pin_enable_irq(struct pin_drv *pin);
void pin_disable_irq(struct pin_drv *pin);
void pin_up(struct pin_drv *pin);
void pin_down(struct pin_drv *pin);
bool pin_state(struct pin_drv *pin);



#define LED_10MS    1
#define LED_1S      100


struct led_drv;

void led_open(struct led_drv *led);
void led_close(struct led_drv *led);
bool led_is_idle(struct led_drv *led);
struct pin_drv* led_pin(struct led_drv *led);

void led_on(struct led_drv *led);
void led_off(struct led_drv *led);
void led_toggle(struct led_drv *led);


#define LED_PERMANENT_BLINK     0
#define LED_SINGLE_BLINK        2
#define LED_DOUBLE_BLINK        4
#define LED_TRIPLE_BLINK        6
#define LED_QUAD_BLINK          8

void led_blink(struct led_drv *led, uint32_t cycles, uint32_t time_on, uint32_t time_off);
bool led_is_blinking(struct led_drv *led);

void led_flash(struct led_drv *led, uint32_t timeout);
bool led_is_flashing(struct led_drv *led);




struct btn_drv;

enum btn_event_e
{
    BTN_PRESS,
    BTN_RELEASE,
    BTN_HOLD,
    BTN_CLICK,
    BTN_DOUBLE_CLICK,
    BTN_TRIPLE_CLICK,
    BTN_QUAD_CLICK,
};

typedef void (*btn_irq_callback)(int btn_event, uint32_t duration);     // Called from interrupt context

void btn_open(struct btn_drv *btn, btn_irq_callback callback);
void btn_close(struct btn_drv *btn);
bool btn_is_idle(struct btn_drv *btn);
bool btn_is_pressed(struct btn_drv *btn);
void btn_speedup_first_click(struct btn_drv *btn);
struct pin_drv* btn_pin(struct btn_drv *btn);

void btn_enable_irq(struct btn_drv *btn);
void btn_disable_irq(struct btn_drv *btn);




struct buzzer_drv;

void buzzer_open(struct buzzer_drv *bzr);
void buzzer_close(struct buzzer_drv *bzr);
void buzzer_on(struct buzzer_drv *bzr);
void buzzer_off(struct buzzer_drv *bzr);

void buzzer_set_frequency(struct buzzer_drv *bzr, uint16_t frequency);
void buzzer_set_duty_cycle(struct buzzer_drv *bzr, uint8_t duty);



void gpio_timer_handler(uint32_t ticks);


#endif /* __MX_SOC_GPIO_H_ */
