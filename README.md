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

void button_pressed(struct button *button)
{
	printf("[%08u] BUTTON [PRESSED]\n", LUNA_GET_TICK());
}

void button_release(struct button *button)
{
	printf("[%08u] BUTTON [RELEASED]\n", LUNA_GET_TICK());
}

void button_click(struct button *button)
{
	printf("[%08u] BUTTON [CLICK]    count: %u\n", LUNA_GET_TICK(), button->repeat);
}

void button_long_pressed(struct button *button)
{
	printf("[%08u] BUTTON [LONG_PRESSED]  timeout: %dms\n", LUNA_GET_TICK(), BUTTON_LONG_TICK);
}


bool button_is_pressed(void)
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

	struct button_config_ops callback = {
		.click        = button_click,
		.pressed      = button_pressed,
		.release      = button_release,
		.long_pressed = button_long_pressed
	};
	luna_button_init(&button, button_is_pressed);
	luna_button_bind(&button, &callback);

        luna_timer_start(&timer);
        while (1)
        {
                luna_timer_run(&timer_head);
                Sleep(1);
        }

        return 0;
}

```
