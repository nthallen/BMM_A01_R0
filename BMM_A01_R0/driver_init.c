/*
 * Code generated from Atmel Start.
 *
 * This file will be overwritten when reconfiguring your Atmel Start project.
 * Please copy examples or other code you want to keep to a separate file
 * to avoid losing it when reconfiguring.
 */

#include "driver_init.h"
#include <peripheral_clk_config.h>
#include <utils.h>
#include <hal_init.h>

/*! The buffer size for USART */
#define USART_DIAG_BUFFER_SIZE 16

struct usart_async_descriptor USART_Diag;
struct can_async_descriptor   CAN_CTRL;

static uint8_t USART_Diag_buffer[USART_DIAG_BUFFER_SIZE];

struct calendar_descriptor CALENDAR;

struct i2c_m_async_desc I2C;

void CALENDAR_CLOCK_init(void)
{
	hri_mclk_set_APBAMASK_RTC_bit(MCLK);
}

void CALENDAR_init(void)
{
	CALENDAR_CLOCK_init();
	calendar_init(&CALENDAR, RTC);
}

void I2C_PORT_init(void)
{

	gpio_set_pin_pull_mode(PA08,
	                       // <y> Pull configuration
	                       // <id> pad_pull_config
	                       // <GPIO_PULL_OFF"> Off
	                       // <GPIO_PULL_UP"> Pull-up
	                       // <GPIO_PULL_DOWN"> Pull-down
	                       GPIO_PULL_OFF);

	gpio_set_pin_function(PA08, PINMUX_PA08C_SERCOM0_PAD0);

	gpio_set_pin_pull_mode(PA09,
	                       // <y> Pull configuration
	                       // <id> pad_pull_config
	                       // <GPIO_PULL_OFF"> Off
	                       // <GPIO_PULL_UP"> Pull-up
	                       // <GPIO_PULL_DOWN"> Pull-down
	                       GPIO_PULL_OFF);

	gpio_set_pin_function(PA09, PINMUX_PA09C_SERCOM0_PAD1);
}

void I2C_CLOCK_init(void)
{
	hri_gclk_write_PCHCTRL_reg(GCLK, SERCOM0_GCLK_ID_CORE, CONF_GCLK_SERCOM0_CORE_SRC | (1 << GCLK_PCHCTRL_CHEN_Pos));
	hri_gclk_write_PCHCTRL_reg(GCLK, SERCOM0_GCLK_ID_SLOW, CONF_GCLK_SERCOM0_SLOW_SRC | (1 << GCLK_PCHCTRL_CHEN_Pos));
	hri_mclk_set_APBCMASK_SERCOM0_bit(MCLK);
}

void I2C_init(void)
{
	I2C_CLOCK_init();
	i2c_m_async_init(&I2C, SERCOM0);
	I2C_PORT_init();
}

/**
 * \brief USART Clock initialization function
 *
 * Enables register interface and peripheral clock
 */
void USART_Diag_CLOCK_init()
{

	hri_gclk_write_PCHCTRL_reg(GCLK, SERCOM3_GCLK_ID_CORE, CONF_GCLK_SERCOM3_CORE_SRC | (1 << GCLK_PCHCTRL_CHEN_Pos));
	hri_gclk_write_PCHCTRL_reg(GCLK, SERCOM3_GCLK_ID_SLOW, CONF_GCLK_SERCOM3_SLOW_SRC | (1 << GCLK_PCHCTRL_CHEN_Pos));
	hri_mclk_set_APBCMASK_SERCOM3_bit(MCLK);
}

/**
 * \brief USART pinmux initialization function
 *
 * Set each required pin to USART functionality
 */
void USART_Diag_PORT_init()
{

	gpio_set_pin_function(PA24, PINMUX_PA24C_SERCOM3_PAD2);

	gpio_set_pin_function(PA25, PINMUX_PA25C_SERCOM3_PAD3);
}

/**
 * \brief USART initialization function
 *
 * Enables USART peripheral, clocks and initializes USART driver
 */
void USART_Diag_init(void)
{
	USART_Diag_CLOCK_init();
	usart_async_init(&USART_Diag, SERCOM3, USART_Diag_buffer, USART_DIAG_BUFFER_SIZE, (void *)NULL);
	USART_Diag_PORT_init();
}

void CAN_CTRL_PORT_init(void)
{

	gpio_set_pin_function(PB11, PINMUX_PB11G_CAN1_RX);

	gpio_set_pin_function(ALRT, PINMUX_PB10G_CAN1_TX);
}
/**
 * \brief CAN initialization function
 *
 * Enables CAN peripheral, clocks and initializes CAN driver
 */
void CAN_CTRL_init(void)
{
	hri_mclk_set_AHBMASK_CAN1_bit(MCLK);
	hri_gclk_write_PCHCTRL_reg(GCLK, CAN1_GCLK_ID, CONF_GCLK_CAN1_SRC | (1 << GCLK_PCHCTRL_CHEN_Pos));
	can_async_init(&CAN_CTRL, CAN1);
	CAN_CTRL_PORT_init();
}

void system_init(void)
{
	init_mcu();

	// GPIO on PA12

	// Set pin direction to input
	gpio_set_pin_direction(VDD2SENSE, GPIO_DIRECTION_IN);

	gpio_set_pin_pull_mode(VDD2SENSE,
	                       // <y> Pull configuration
	                       // <id> pad_pull_config
	                       // <GPIO_PULL_OFF"> Off
	                       // <GPIO_PULL_UP"> Pull-up
	                       // <GPIO_PULL_DOWN"> Pull-down
	                       GPIO_PULL_OFF);

	gpio_set_pin_function(VDD2SENSE, GPIO_PIN_FUNCTION_OFF);

	// GPIO on PA16

	gpio_set_pin_level(STATUS_LED,
	                   // <y> Initial level
	                   // <id> pad_initial_level
	                   // <false"> Low
	                   // <true"> High
	                   false);

	// Set pin direction to output
	gpio_set_pin_direction(STATUS_LED, GPIO_DIRECTION_OUT);

	gpio_set_pin_function(STATUS_LED, GPIO_PIN_FUNCTION_OFF);

	// GPIO on PA17

	gpio_set_pin_level(FAULT_LED,
	                   // <y> Initial level
	                   // <id> pad_initial_level
	                   // <false"> Low
	                   // <true"> High
	                   false);

	// Set pin direction to output
	gpio_set_pin_direction(FAULT_LED, GPIO_DIRECTION_OUT);

	gpio_set_pin_function(FAULT_LED, GPIO_PIN_FUNCTION_OFF);

	// GPIO on PB03

	gpio_set_pin_level(SHDN_N,
	                   // <y> Initial level
	                   // <id> pad_initial_level
	                   // <false"> Low
	                   // <true"> High
	                   true);

	// Set pin direction to output
	gpio_set_pin_direction(SHDN_N, GPIO_DIRECTION_OUT);

	gpio_set_pin_function(SHDN_N, GPIO_PIN_FUNCTION_OFF);

	CALENDAR_init();

	I2C_init();
	USART_Diag_init();
	CAN_CTRL_init();
}
