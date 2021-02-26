#ifndef __WATCHDOG_H
#define __WATCHDOG_H


void wdt_init(uint32_t time_ms);
void wdt_feed();
void wdt_stop(void);

#endif