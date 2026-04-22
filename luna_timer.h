/* luna_timer.h */
#ifndef LUNA_TIMER_H
#define LUNA_TIMER_H

#include <stdint.h>

struct core_timer;

typedef void (*core_timer_callback_t)(struct core_timer *timer);

#ifndef LUNA_TICK_TYPE
#define LUNA_TICK_TYPE                  uint32_t
#endif

#ifndef LUNA_GET_TICK
#include <time.h>
#define LUNA_GET_TICK                   LUNA_GET_TICK
static inline LUNA_TICK_TYPE LUNA_GET_TICK(void)
{
        return (LUNA_TICK_TYPE)(clock() * 1000 / CLOCKS_PER_SEC);
}
#endif

#ifndef LUNA_ASSERT
#include <assert.h>
#define LUNA_ASSERT                     assert
#endif

#ifndef LUNA_EXPIRED
#define luna_expired(type, diff)        ((diff) > (((type)-1) >> 1))
#define LUNA_EXPIRED                    luna_expired
#endif

#ifndef LUNA_LESS_THAN
#define LUNA_LESS_THAN(type, a, b)      LUNA_EXPIRED(type, (a) - (b))
#endif

struct core_timer {
        struct core_timer *next;
        LUNA_TICK_TYPE when;
        core_timer_callback_t callback;
};

void luna_timer_append(struct core_timer **head, struct core_timer *timer);
void luna_timer_remove(struct core_timer **head, struct core_timer *timer);

LUNA_TICK_TYPE luna_timer_get_next_expiry(struct core_timer **head);
LUNA_TICK_TYPE luna_timer_run(struct core_timer **head);

typedef void(*auto_timer_callback_t)(void *arg);

typedef enum {
	TIMER_ONE_SHOT = 0,
	TIMER_PERIODIC = 1,
} auto_timer_mode_t;

struct auto_timer {
	struct core_timer     super;

	struct core_timer   **header;
	uint32_t              running;
	LUNA_TICK_TYPE        interval;
	auto_timer_mode_t     mode;

	auto_timer_callback_t callback;
	void                 *arg;
};

void luna_timer_init(struct auto_timer *timer, struct core_timer **header, LUNA_TICK_TYPE interval, auto_timer_mode_t mode, auto_timer_callback_t callback, void *user_data);
void luna_timer_start(struct auto_timer *timer);
void luna_timer_stop(struct auto_timer *timer);
void luna_timer_restart(struct auto_timer *timer);
void luna_timer_set_interval(struct auto_timer *timer, LUNA_TICK_TYPE interval);

#endif

#ifdef LUNA_TIMER_IMPLEMENTATION

void luna_timer_append(struct core_timer **head, struct core_timer *timer)
{
        LUNA_ASSERT(head);
        LUNA_ASSERT(timer);

        struct core_timer **next = head;
        LUNA_TICK_TYPE when = timer->when;

        while (*next && LUNA_LESS_THAN(LUNA_TICK_TYPE, (*next)->when, when)) {
                next = &((*next)->next);
        }
        timer->next = *next;
        *next = timer;
}

void luna_timer_remove(struct core_timer **head, struct core_timer *timer)
{
        LUNA_ASSERT(head);
        LUNA_ASSERT(timer);
        if(!(*head)) {
                return;
        }
        struct core_timer **next = head;
        while (*next) {
                if (*next == timer) {
                        *next = timer->next;
                        return;
                }
                next = &(*next)->next;
        }
}

LUNA_TICK_TYPE luna_timer_get_next_expiry(struct core_timer **head)
{
        LUNA_ASSERT(head);
        if (!(*head)) {
                return (LUNA_TICK_TYPE)-1;
        }
        LUNA_TICK_TYPE now  = LUNA_GET_TICK();
        LUNA_TICK_TYPE when = (*head)->when;
        if (LUNA_LESS_THAN(LUNA_TICK_TYPE, when, now)) {
                return 0;
        }

        return when - now;
}

LUNA_TICK_TYPE luna_timer_run(struct core_timer **head)
{
        LUNA_ASSERT(head);
        if(!(*head)) {
                return (LUNA_TICK_TYPE)-1;
        }
        LUNA_TICK_TYPE next_expiry;
        next_expiry = luna_timer_get_next_expiry(head);
        if (0 == next_expiry) {
               struct core_timer *timer = *head;
                *head                   = timer->next;
                timer->next             = 0;
                if (timer->callback) {
                        timer->callback(timer);
                }
        }
        return next_expiry;
}

///////////////////////////////////////////////////////////////////////////////

static void _core_timer_callback(struct core_timer *super)
{
	struct auto_timer *timer = (struct auto_timer *)super;
	timer->running = 0;

	if (timer->mode == TIMER_ONE_SHOT) {

	} else {
		timer->super.when = timer->super.when + timer->interval;
		luna_timer_append(timer->header, &timer->super);
		timer->running    = 1;
	}
	if (timer->callback) {
		timer->callback(timer->arg);
	}
}

void luna_timer_init(struct auto_timer *timer, struct core_timer **header, LUNA_TICK_TYPE interval, auto_timer_mode_t mode, auto_timer_callback_t callback, void *arg)
{
	LUNA_ASSERT(timer);
	LUNA_ASSERT(interval <= (((LUNA_TICK_TYPE)-1) >> 1));

	timer->super.next     = NULL;
	timer->super.callback = _core_timer_callback;
	timer->header         = header;
	timer->interval       = interval;
	timer->mode           = mode;
	timer->running        = 0;
	timer->callback       = callback;
	timer->arg            = arg;
}

void luna_timer_start(struct auto_timer *timer)
{
	LUNA_ASSERT(timer);

	if (timer->running) {
		return;
	}
	timer->super.when = LUNA_GET_TICK() + timer->interval;
	luna_timer_append(timer->header, &timer->super);
	timer->running    = 1;
}

void luna_timer_stop(struct auto_timer *timer)
{
	LUNA_ASSERT(timer);

	if (!timer->running) {
		return;
	}
	luna_timer_remove(timer->header, &timer->super);
	timer->running = 0;
}

void luna_timer_restart(struct auto_timer *timer)
{
	LUNA_ASSERT(timer);

	luna_timer_stop(timer);
	luna_timer_start(timer);
}

void luna_timer_set_interval(struct auto_timer *timer, LUNA_TICK_TYPE interval)
{
	LUNA_ASSERT(timer);
        LUNA_ASSERT(interval > 0);
	LUNA_ASSERT(interval <= (((LUNA_TICK_TYPE)-1) >> 1));
        if (timer->running) {
            luna_timer_stop(timer);
        }
	timer->interval = interval;
	luna_timer_start(timer);
}

#endif
