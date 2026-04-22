# Example
``` c
/* main.c */
#include <stdio.h>

#define LUNA_BUTTON_IMPLEMENTATION
#include "luna_button.h"

#define LUNA_TIMER_IMPLEMENTATION
#include "luna_timer.h"

#include <windows.h>

struct button button;

void button_pressed(void *arg)
{
	struct button *button = (struct button *)arg;
	printf("pressed.\n");
}

void button_release(void *arg)
{
	struct button *button = (struct button *)arg;
	printf("release.\n");
}

void button_clicks(void *arg)
{
	struct button *button = (struct button *)arg;
	printf("clicks %d.\n", button->repeat);
}

void button_long_pressed(void *arg)
{
	struct button *button = (struct button *)arg;
	printf("long_pressed.\n");
}


bool button_is_pressed(void)
{
	return (GetAsyncKeyState('A') & 0x8000) != 0;
}


static struct core_timer *timer_head = NULL;

void timer_call_callback(void *arg)
{
        luna_button_process(&button);
}

int main(void)
{
        struct auto_timer timer;

        luna_timer_init(&timer,
			&timer_head,
			5,
			TIMER_PERIODIC,
			timer_call_callback,
			NULL);
        luna_timer_start(&timer);



	struct button_handle_ops handle = {
		button_pressed,
		button_release,
		button_clicks,
		button_long_pressed
	};
	luna_button_init(&button, button_is_pressed);
	luna_button_register(&button, handle);
        while (1)
        {
                luna_timer_run(&timer_head);
                usleep(1000);
        }

        return 0;
}


```
