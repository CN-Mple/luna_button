/* luna_button.h */
#ifndef LUNA_BUTTON_H
#define LUNA_BUTTON_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#ifndef LUNA_TICK_TYPE
#define LUNA_TICK_TYPE			uint32_t
#endif

#ifndef LUNA_GET_TICK
#define LUNA_GET_TICK                   LUNA_GET_TICK
#include <time.h>
static inline LUNA_TICK_TYPE LUNA_GET_TICK(void)
{
        return (LUNA_TICK_TYPE)(clock() * 1000 / CLOCKS_PER_SEC);
}
#endif

#ifndef LUNA_EXPIRED
#define luna_expired(type, diff)	((diff) > (((type)-1) >> 1))
#define LUNA_EXPIRED			luna_expired
#endif

#ifndef LUNA_LESS_THAN
#define LUNA_LESS_THAN(type, a, b)	LUNA_EXPIRED(type, (a)-(b))
#endif

#ifndef BUTTON_DEBOUNCE_TICK
#define BUTTON_DEBOUNCE_TICK		(20)
#endif

#ifndef BUTTON_LONG_TICK
#define BUTTON_LONG_TICK		(3000)
#endif

#ifndef BUTTON_CLICK_INTERVAL
#define BUTTON_CLICK_INTERVAL		(500)
#endif

typedef enum {
        RELEASE = 0,
        PRE_PRESSED,
        PRESSED,
        PRE_RELEASE,
        LONG_PRESSED,
} button_state_t;

struct button_ops {
        bool (*is_pressed)(void);
};

struct button_handle_ops {
        bool (*pressed)(void *arg);
        bool (*release)(void *arg);
        bool (*clicks)(void *arg);
        bool (*long_pressed)(void *arg);
};

struct button {
        button_state_t           state;
        struct button_ops        ops;
	struct button_handle_ops handle;
        LUNA_TICK_TYPE           tick;
	uint32_t                 repeat;
};

void luna_button_init(struct button *button, bool (*is_pressed)(void));
void luna_button_register(struct button *button, struct button_handle_ops handle);
void luna_button_process(struct button *button);

#endif

#ifdef LUNA_BUTTON_IMPLEMENTATION

void luna_button_init(struct button *button, bool (*is_pressed)(void))
{
	if (!button) {
		return;
	}
	button->state          = RELEASE;
	button->tick           = LUNA_GET_TICK();
	button->repeat         = 0;
	button->ops.is_pressed = is_pressed;
}

void luna_button_register(struct button *button, struct button_handle_ops handle)
{
	button->handle = handle;
}

void luna_button_process(struct button *button)
{
        if (!button->ops.is_pressed) {
		return;
	}

        bool pressed       = button->ops.is_pressed();
        LUNA_TICK_TYPE now = LUNA_GET_TICK();

        switch (button->state) {
                case RELEASE:
			if (button->repeat > 0) {
				if (LUNA_LESS_THAN(LUNA_TICK_TYPE, BUTTON_CLICK_INTERVAL, now - button->tick)) {
					if (button->handle.clicks) {
						button->handle.clicks(button);
					}
					button->repeat = 0;
				}
			}
                        if (pressed) {
                                button->state = PRE_PRESSED;
                                button->tick  = now;
                        }
		break;
                case PRE_PRESSED:
                        if (pressed) {
                                if (LUNA_LESS_THAN(LUNA_TICK_TYPE, BUTTON_DEBOUNCE_TICK, now - button->tick)) {
                                        button->state = PRESSED;
                                        button->tick  = now;
					++button->repeat;
					if (button->handle.pressed) {
						button->handle.pressed(button);
					}
                                }
                        } else {
                                button->state = RELEASE;
                        }
		break;
                case PRESSED:
                        if (!pressed) {
                                button->state = PRE_RELEASE;
                                button->tick = now;
                        } else {
                                if (now - button->tick >= BUTTON_LONG_TICK) {
                                        button->state = LONG_PRESSED;
					if (button->handle.long_pressed) {
						button->handle.long_pressed(button);
					}
                                }
                        }
		break;
                case PRE_RELEASE:
                        if (!pressed) {
                                if (LUNA_LESS_THAN(LUNA_TICK_TYPE, BUTTON_DEBOUNCE_TICK, now - button->tick)) {
					button->state = RELEASE;
					button->tick  = now;
					if (button->handle.release) {
						button->handle.release(button);
					}
                                }
                        } else {
                                button->state = PRESSED;
                        }
		break;
                case LONG_PRESSED:
                        if (!pressed) {
                                button->state = PRE_RELEASE;
                                button->tick  = now;
                        }
		break;
                default:
                        button->state  = RELEASE;
			button->tick   = now;
			button->repeat = 0;
		break;
        }
}

#endif

