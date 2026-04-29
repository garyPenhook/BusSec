#ifndef TC0_PERIODIC_H
#define TC0_PERIODIC_H

/* Callback runs in TC0 interrupt context; keep implementations short. */
typedef void (*tc0_periodic_callback_t)(void);

/* Configure TC0 to call the callback every 500 ms. */
void tc0_periodic_init_500ms(tc0_periodic_callback_t callback);

#endif /* TC0_PERIODIC_H */
