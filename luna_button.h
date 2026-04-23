/* luna_button.h */
#ifndef LUNA_BUTTON_H
#define LUNA_BUTTON_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#ifndef LUNA_ASSERT
#include <assert.h>
#define LUNA_ASSERT                     assert
#endif

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
#define BUTTON_LONG_TICK		(2000)
#endif

#ifndef BUTTON_CLICK_INTERVAL
#define BUTTON_CLICK_INTERVAL		(200)
#endif

typedef enum {
        RELEASE = 0,
        PRE_PRESSED,
        PRESSED,
        PRE_RELEASE,
} button_state_t;

struct button;

struct button_ops {
        bool (*is_pressed)(void);
};

struct button_config_ops {
        void (*click)(struct button *button);
        void (*pressed)(struct button *button);
        void (*release)(struct button *button);
        void (*long_pressed)(struct button *button);
};

struct button {
        button_state_t           state;
        struct button_ops        ops;
	struct button_config_ops callback;
        LUNA_TICK_TYPE           tick;
	uint32_t                 repeat;
};

void luna_button_init(struct button *button, bool (*is_pressed)(void));
void luna_button_bind(struct button *button, const struct button_config_ops *callback);
void luna_button_poll(struct button *button);

#endif

#ifdef LUNA_BUTTON_IMPLEMENTATION

void luna_button_init(struct button *button, bool (*is_pressed)(void))
{
        LUNA_ASSERT(button);
	if (!button) {
		return;
	}
	button->state          = RELEASE;
	button->tick           = LUNA_GET_TICK();
	button->repeat         = 0;
	button->ops.is_pressed = is_pressed;
}

void luna_button_bind(struct button *button, const struct button_config_ops *callback)
{
        LUNA_ASSERT(button);
        LUNA_ASSERT(callback);
	button->callback = *callback;
}

void luna_button_poll(struct button *button)
{
        LUNA_ASSERT(button);
        if (!button->ops.is_pressed) {
		return;
	}

        bool pressed       = button->ops.is_pressed();
        LUNA_TICK_TYPE now = LUNA_GET_TICK();

        switch (button->state) {
                case RELEASE:
			if (button->repeat > 0) {
				if (LUNA_LESS_THAN(LUNA_TICK_TYPE, BUTTON_CLICK_INTERVAL, now - button->tick)) {
					if (button->callback.click) {
						button->callback.click(button);
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
					if (button->callback.pressed) {
						button->callback.pressed(button);
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
                                if (LUNA_LESS_THAN(LUNA_TICK_TYPE, BUTTON_LONG_TICK, now - button->tick)) {
					button->tick  = now;
                                        if (button->callback.long_pressed) {
                                                button->callback.long_pressed(button);
                                        }
                                }
                        }
		break;
                case PRE_RELEASE:
                        if (!pressed) {
                                if (LUNA_LESS_THAN(LUNA_TICK_TYPE, BUTTON_DEBOUNCE_TICK, now - button->tick)) {
					button->state = RELEASE;
                                        button->tick  = now;
					if (button->callback.release) {
						button->callback.release(button);
					}
                                }
                        } else {
                                button->state = PRESSED;
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

