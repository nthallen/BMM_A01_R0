/*
 * Code generated from Atmel Start.
 *
 * This file will be overwritten when reconfiguring your Atmel Start project.
 * Please copy examples or other code you want to keep to a separate file
 * to avoid losing it when reconfiguring.
 */
#ifndef DRIVER_INIT_INCLUDED
#define DRIVER_INIT_INCLUDED

#include "atmel_start_pins.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <hal_atomic.h>
#include <hal_delay.h>
#include <hal_gpio.h>
#include <hal_init.h>
#include <hal_io.h>
#include <hal_sleep.h>

#include <hal_calendar.h>

#include <hal_i2c_m_async.h>
#include <hal_usart_async.h>
#include <hal_can_async.h>

extern struct calendar_descriptor CALENDAR;

extern struct i2c_m_async_desc       I2C;
extern struct usart_async_descriptor USART_Diag;
extern struct can_async_descriptor   CAN_CTRL;

void CALENDAR_CLOCK_init(void);
void CALENDAR_init(void);

void I2C_PORT_init(void);
void I2C_CLOCK_init(void);
void I2C_init(void);

void USART_Diag_PORT_init(void);
void USART_Diag_CLOCK_init(void);
void USART_Diag_init(void);

/**
 * \brief Perform system initialization, initialize pins and clocks for
 * peripherals
 */
void system_init(void);

#ifdef __cplusplus
}
#endif
#endif // DRIVER_INIT_INCLUDED
