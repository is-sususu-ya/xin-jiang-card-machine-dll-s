
#ifndef GPIO_INCLUDED
#define GPIO_INCLUDED

#ifdef __cplusplus
extern "C"{
#endif

int GPIO_ReadDI();
int GPIO_initialize();
int GPIO_terminate();
void GPIO_register_callback( void(*)(int, int));
int GPIO_input( int pin);
int GPIO_output( int pin, int val);
int GPIO_pulse(int pin, int count, int nhalfPeriod);
int GPIO_pulse_negative(int pin, int count, int nhalfPeriod);

#ifdef __cplusplus
}
#endif

#endif
