/* luna_button.h */
#ifndef LUNA_BUTTON_H
#define LUNA_BUTTON_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#ifndef LUNA_ASSERT
#include <assert.h>
#define LUNA_ASSERT                             assert
#endif

#ifndef LUNA_TICK_TYPE
#define LUNA_TICK_TYPE			        uint32_t
#endif

#ifndef LUNA_GET_TICK
#include <time.h>
static inline LUNA_TICK_TYPE luna_get_tick(void)
{
        return (LUNA_TICK_TYPE)(clock() * 1000 / CLOCKS_PER_SEC);
}
#define LUNA_GET_TICK                           luna_get_tick
#endif

#ifndef LUNA_EXPIRED
#define luna_expired(type, diff)	        ((diff) > (((type)-1) >> 1))
#define LUNA_EXPIRED			        luna_expired
#endif

#ifndef LUNA_LESS_THAN
#define LUNA_LESS_THAN(type, a, b)	        LUNA_EXPIRED(type, (a)-(b))
#endif

#ifndef BUTTON_DEBOUNCE_INTERVAL
#define BUTTON_DEBOUNCE_INTERVAL                (20)
#endif

#ifndef BUTTON_LONG_PRESSED_INTERVAL
#define BUTTON_LONG_PRESSED_INTERVAL            (600)
#endif

#ifndef BUTTON_LONG_PRESSED_REPEAT_INTERVAL
#define BUTTON_LONG_PRESSED_REPEAT_INTERVAL     (100)
#endif

#ifndef BUTTON_CLICK_INTERVAL
#define BUTTON_CLICK_INTERVAL		        (250)
#endif

typedef enum {
        BUTTON_RELEASE = 0,
        BUTTON_PRE_PRESSED,
        BUTTON_PRESSED,
        BUTTON_PRE_RELEASE,
} button_state_t;

struct button;

struct button_interface {
        bool (*is_press)(void);
};

struct button_callback {
        void (*mult_click)(void *arg, uint32_t count);
        void (*press)(void *arg);
        void (*release)(void *arg);
        void (*long_press)(void *arg);
};

struct button_config {
        uint32_t debounce_interval;
        uint32_t long_press_interval;
        uint32_t long_press_repeat_interval;
        uint32_t multiply_click_interval;
};

struct button {
        button_state_t                state;
        struct button_interface       interface;
	const struct button_callback *callback;
        void                         *arg;
        struct button_config          config;
        LUNA_TICK_TYPE                tick;

        uint32_t                      long_press_en : 1;
	uint32_t                      long_press    : 1;
	uint32_t                      repeat        :30;
};

void luna_button_init(struct button *button, bool (*is_press)(void));
void luna_button_bind(struct button *button, const struct button_callback *callback, void *arg);
void luna_button_poll(struct button *button);

void luna_button_enable_long_press(struct button *button, bool enable);

void luna_button_set_debounce_interval(struct button *button, uint32_t ms);
void luna_button_set_long_press_interval(struct button *button, uint32_t ms);
void luna_button_set_long_press_repeat_interval(struct button *button, uint32_t ms);
void luna_button_set_click_interval(struct button *button, uint32_t ms);

#endif

#ifdef LUNA_BUTTON_IMPLEMENTATION

void luna_button_init(struct button *button, bool (*is_press)(void))
{
        LUNA_ASSERT(button);

	button->state              = BUTTON_RELEASE;
	button->tick               = LUNA_GET_TICK();
	button->interface.is_press = is_press;
        button->callback           = NULL;

        button->long_press_en      = 0;
        button->long_press         = 0;
	button->repeat             = 0;

        button->config             = (struct button_config){
                .debounce_interval          = BUTTON_DEBOUNCE_INTERVAL,
                .long_press_interval        = BUTTON_LONG_PRESSED_INTERVAL,
                .long_press_repeat_interval = BUTTON_LONG_PRESSED_REPEAT_INTERVAL,
                .multiply_click_interval    = BUTTON_CLICK_INTERVAL
        };
}

void luna_button_bind(struct button *button, const struct button_callback *callback, void *arg)
{
        LUNA_ASSERT(button);
        LUNA_ASSERT(callback);
	button->callback = callback;
        button->arg      = arg;
}

void luna_button_poll(struct button *button)
{
        LUNA_ASSERT(button);
        if (!button->interface.is_press || !button->callback) {
		return;
	}

        bool press       = button->interface.is_press();
        LUNA_TICK_TYPE now = LUNA_GET_TICK();

        switch (button->state) {
                case BUTTON_RELEASE:
			if (button->repeat > 0) {
				if (LUNA_LESS_THAN(LUNA_TICK_TYPE, button->config.multiply_click_interval, now - button->tick)) {
					if (button->callback->mult_click) {
						button->callback->mult_click(button->arg, button->repeat);
					}
					button->repeat = 0;
				}
			}
                        if (press) {
                                button->state = BUTTON_PRE_PRESSED;
                                button->tick  = now;
                                button->long_press = 0;
                        }
		break;
                case BUTTON_PRE_PRESSED:
                        if (press) {
                                if (LUNA_LESS_THAN(LUNA_TICK_TYPE, button->config.debounce_interval, now - button->tick)) {
                                        button->state = BUTTON_PRESSED;
                                        button->tick  = now;
					++button->repeat;
					if (button->callback->press) {
						button->callback->press(button->arg);
					}
                                }
                        } else {
                                button->state = BUTTON_RELEASE;
                        }
		break;
                case BUTTON_PRESSED:
                        if (press) {
                                if (button->long_press_en) {
                                        if (!button->long_press) {
                                                if (LUNA_LESS_THAN(LUNA_TICK_TYPE, button->config.long_press_interval, now - button->tick)) {
                                                        button->tick  = now;
                                                        if (button->callback->long_press) {
                                                                button->callback->long_press(button->arg);
                                                        }
                                                        button->long_press = 1;
                                                }
                                        } else {
                                                if (LUNA_LESS_THAN(LUNA_TICK_TYPE, button->config.long_press_repeat_interval, now - button->tick)) {
                                                        button->tick = now;
                                                        if (button->callback && button->callback->long_press) {
                                                                button->callback->long_press(button->arg);
                                                        }
                                                }
                                        }
                                }
                        } else {
                                button->state = BUTTON_PRE_RELEASE;
                                button->tick  = now;
                        }
		break;
                case BUTTON_PRE_RELEASE:
                        if (press) {
                                button->state = BUTTON_PRESSED;
                                button->tick  = now;
                        } else {
                                if (LUNA_LESS_THAN(LUNA_TICK_TYPE, button->config.debounce_interval, now - button->tick)) {
					button->state = BUTTON_RELEASE;
                                        button->tick  = now;
					if (button->callback->release) {
						button->callback->release(button->arg);
					}
                                }
                        }
		break;
                default:
                        button->state      = BUTTON_RELEASE;
			button->tick       = now;
			button->repeat     = 0;
                        button->long_press = 0;
		break;
        }
}

void luna_button_enable_long_press(struct button *button, bool enable)
{
        LUNA_ASSERT(button);
        button->long_press_en = enable;
}

void luna_button_set_debounce_interval(struct button *button, uint32_t ms)
{
        LUNA_ASSERT(button);
        button->config.debounce_interval = ms;
}

void luna_button_set_long_press_interval(struct button *button, uint32_t ms)
{
        LUNA_ASSERT(button);
        button->config.long_press_interval = ms;
}

void luna_button_set_long_press_repeat_interval(struct button *button, uint32_t ms)
{
        LUNA_ASSERT(button);
        button->config.long_press_repeat_interval = ms;
}

void luna_button_set_click_interval(struct button *button, uint32_t ms)
{
        LUNA_ASSERT(button);
        button->config.multiply_click_interval = ms;
}

#endif

