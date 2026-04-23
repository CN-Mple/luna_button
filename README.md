# Example
``` c
/* main.c */
#include <stdio.h>

#define LUNA_BUTTON_IMPLEMENTATION
#include "luna_button.h"

#define LUNA_TIMER_IMPLEMENTATION
#include "luna_timer.h"

#include <windows.h>

static struct button button;

static struct core_timer *timer_head = NULL;

void button_press(void *arg)
{
	(void)arg;
	printf("[%08u] BUTTON [PRESSED]\n", LUNA_GET_TICK());
}

void button_release(void *arg)
{
	(void)arg;
	printf("[%08u] BUTTON [RELEASED]\n", LUNA_GET_TICK());
}

void button_mult_click(void *arg, uint32_t click)
{
	(void)arg;
	printf("[%08u] BUTTON [CLICK][%08u]\n", LUNA_GET_TICK(), click);
}

void button_long_press(void *arg)
{
	(void)arg;
	printf("[%08u] BUTTON [LONG_PRESSED]\n", LUNA_GET_TICK());
}


bool button_is_press(void)
{
	return (GetAsyncKeyState('A') & 0x8000) != 0;
}

void timer_call_callback(void *arg)
{
	struct button *button = (struct button *)arg;
        luna_button_poll(button);
}

int main(void)
{
        struct auto_timer timer;

        luna_timer_init(&timer,
			&timer_head,
			5,
			TIMER_PERIODIC,
			timer_call_callback,
			&button);

	const struct button_callback callback = {
		.mult_click   = button_mult_click,
		.press      = button_press,
		.release      = button_release,
		.long_press = button_long_press
	};
	luna_button_init(&button, button_is_press);
	luna_button_bind(&button, &callback, 0);

	luna_button_enable_long_press(&button, true);

	luna_button_set_debounce_interval         (&button, BUTTON_DEBOUNCE_INTERVAL);
	luna_button_set_long_press_interval       (&button, BUTTON_LONG_PRESSED_INTERVAL);
	luna_button_set_long_press_repeat_interval(&button, BUTTON_LONG_PRESSED_REPEAT_INTERVAL);
	luna_button_set_click_interval            (&button, BUTTON_CLICK_INTERVAL);

        luna_timer_start(&timer);
        while (1)
        {
                luna_timer_run(&timer_head);
                Sleep(1);
        }

        return 0;
}

```
