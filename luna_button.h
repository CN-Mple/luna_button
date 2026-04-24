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

typedef enum {
        BUTTON_CTRL_LONG_PRESS = 1 << 0,
} button_ctrl_t;


struct button;

struct button_interface {
        bool (*is_press)(void);
};

struct button_callback {
        void (*mult_click)(void *user_arg, uint32_t count);
        void (*press)(void *user_arg);
        void (*release)(void *user_arg);
        void (*long_press)(void *user_arg);
};

struct button_config {
        uint32_t debounce_interval;
        uint32_t long_press_interval;
        uint32_t long_repeat_interval;
        uint32_t multiply_click_interval;
};

struct button {
        button_state_t                state;
        struct button_interface       interface;
	const struct button_callback *callback;
        void                         *user_arg;
        struct button_config          config;
        LUNA_TICK_TYPE                tick;

        uint32_t                      long_press_enable : 1;
	uint32_t                      is_long_pressed   : 1;
	uint32_t                      multi_click_count :30;
};

void luna_button_init(struct button *button, bool (*is_press)(void), const struct button_config *config);
void luna_button_bind(struct button *button, const struct button_callback *callback, void *user_arg);
void luna_button_ctrl(struct button *button, button_ctrl_t ctrl, bool enable);
void luna_button_poll(struct button *button);

#endif

#ifdef LUNA_BUTTON_IMPLEMENTATION

#define BUTTON_DEFAULT_CONFIG()                                                 \
        (struct button_config){                                                 \
                .debounce_interval       = BUTTON_DEBOUNCE_INTERVAL,            \
                .long_press_interval     = BUTTON_LONG_PRESSED_INTERVAL,        \
                .long_repeat_interval    = BUTTON_LONG_PRESSED_REPEAT_INTERVAL, \
                .multiply_click_interval = BUTTON_CLICK_INTERVAL                \
        }

#define BUTTON_CUSTOM_CONFIG(debounce, long_press, long_repeat, multi_click)    \
        (struct button_config){                                                 \
                .debounce_interval       = debounce,                            \
                .long_press_interval     = long_press,                          \
                .long_repeat_interval    = long_repeat,                         \
                .multiply_click_interval = multi_click                          \
        }

void luna_button_init(struct button *button, bool (*is_press)(void), const struct button_config *config)
{
        LUNA_ASSERT(button);

	button->state              = BUTTON_RELEASE;
	button->tick               = LUNA_GET_TICK();
	button->interface.is_press = is_press;
        button->callback           = NULL;

        button->long_press_enable  = 0;
        button->is_long_pressed    = 0;
	button->multi_click_count  = 0;

        button->config             = *config;
}

void luna_button_bind(struct button *button, const struct button_callback *callback, void *user_arg)
{
        LUNA_ASSERT(button);
        LUNA_ASSERT(callback);
	
	button->callback = callback;
        button->user_arg      = user_arg;
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
			if (button->multi_click_count > 0) {
				if (LUNA_LESS_THAN(LUNA_TICK_TYPE, button->config.multiply_click_interval, now - button->tick)) {
					if (button->callback->mult_click) {
						button->callback->mult_click(button->user_arg, button->multi_click_count);
					}
					button->multi_click_count = 0;
				}
			}
                        if (press) {
                                button->state = BUTTON_PRE_PRESSED;
                                button->tick  = now;
                                button->is_long_pressed = 0;
                        }
		break;
                case BUTTON_PRE_PRESSED:
                        if (press) {
                                if (LUNA_LESS_THAN(LUNA_TICK_TYPE, button->config.debounce_interval, now - button->tick)) {
                                        button->state = BUTTON_PRESSED;
                                        button->tick  = now;
					++button->multi_click_count;
					if (button->callback->press) {
						button->callback->press(button->user_arg);
					}
                                }
                        } else {
                                button->state = BUTTON_RELEASE;
                        }
		break;
                case BUTTON_PRESSED:
                        if (press) {
                                if (button->long_press_enable) {
                                        if (!button->is_long_pressed) {
                                                if (LUNA_LESS_THAN(LUNA_TICK_TYPE, button->config.long_press_interval, now - button->tick)) {
                                                        button->tick  = now;
                                                        if (button->callback->long_press) {
                                                                button->callback->long_press(button->user_arg);
                                                        }
                                                        button->is_long_pressed = 1;
                                                }
                                        } else {
                                                if (LUNA_LESS_THAN(LUNA_TICK_TYPE, button->config.long_repeat_interval, now - button->tick)) {
                                                        button->tick = now;
                                                        if (button->callback->long_press) {
                                                                button->callback->long_press(button->user_arg);
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
						button->callback->release(button->user_arg);
					}
                                }
                        }
		break;
                default:
                        button->state             = BUTTON_RELEASE;
			button->tick              = now;
			button->multi_click_count = 0;
                        button->is_long_pressed   = 0;
		break;
        }
}

void luna_button_ctrl(struct button *button, button_ctrl_t ctrl, bool enable)
{
        LUNA_ASSERT(button);
	
        switch (ctrl) {
        case BUTTON_CTRL_LONG_PRESS:
                button->long_press_enable = enable ? 1 : 0;
                if (!enable) {
                        button->is_long_pressed = 0;
                }
        break;
        default:
                LUNA_ASSERT(0);
        break;
        }
}

#endif

