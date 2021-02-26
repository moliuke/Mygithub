#ifndef __GPIO_H
#define __GPIO_H

inline void GPIO_pin_init(void);
inline void restore_pin_init(void);
inline int restore_pin_level(void);
int check_manMadeRestore(void);
int check_IPRestore(void);
#endif

