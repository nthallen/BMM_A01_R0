/*
 * Code generated from Atmel Start.
 *
 * This file will be overwritten when reconfiguring your Atmel Start project.
 * Please copy examples or other code you want to keep to a separate file
 * to avoid losing it when reconfiguring.
 */

#include "driver_examples.h"
#include "driver_init.h"
#include "utils.h"

/**
 * Example of using CALENDAR.
 */
static struct calendar_alarm alarm;

static void alarm_cb(struct calendar_descriptor *const descr)
{
	/* alarm expired */
}

void CALENDAR_example(void)
{
	struct calendar_date date;
	struct calendar_time time;

	calendar_enable(&CALENDAR);

	date.year  = 2000;
	date.month = 12;
	date.day   = 31;

	time.hour = 12;
	time.min  = 59;
	time.sec  = 59;

	calendar_set_date(&CALENDAR, &date);
	calendar_set_time(&CALENDAR, &time);

	alarm.cal_alarm.datetime.time.sec = 4;
	alarm.cal_alarm.option            = CALENDAR_ALARM_MATCH_SEC;
	alarm.cal_alarm.mode              = REPEAT;

	calendar_set_alarm(&CALENDAR, &alarm, alarm_cb);
}

static uint8_t I2C_example_str[12] = "Hello World!";

void I2C_tx_complete(struct i2c_m_async_desc *const i2c)
{
}

void I2C_example(void)
{
	struct io_descriptor *I2C_io;

	i2c_m_async_get_io_descriptor(&I2C, &I2C_io);
	i2c_m_async_enable(&I2C);
	i2c_m_async_register_callback(&I2C, I2C_M_ASYNC_TX_COMPLETE, (FUNC_PTR)I2C_tx_complete);
	i2c_m_async_set_slaveaddr(&I2C, 0x12, I2C_M_SEVEN);

	io_write(I2C_io, I2C_example_str, 12);
}

/**
 * Example of using USART_Diag to write "Hello World" using the IO abstraction.
 *
 * Since the driver is asynchronous we need to use statically allocated memory for string
 * because driver initiates transfer and then returns before the transmission is completed.
 *
 * Once transfer has been completed the tx_cb function will be called.
 */

static uint8_t example_USART_Diag[12] = "Hello World!";

static void tx_cb_USART_Diag(const struct usart_async_descriptor *const io_descr)
{
	/* Transfer completed */
}

void USART_Diag_example(void)
{
	struct io_descriptor *io;

	usart_async_register_callback(&USART_Diag, USART_ASYNC_TXC_CB, tx_cb_USART_Diag);
	/*usart_async_register_callback(&USART_Diag, USART_ASYNC_RXC_CB, rx_cb);
	usart_async_register_callback(&USART_Diag, USART_ASYNC_ERROR_CB, err_cb);*/
	usart_async_get_io_descriptor(&USART_Diag, &io);
	usart_async_enable(&USART_Diag);

	io_write(io, example_USART_Diag, 12);
}

void CAN_CTRL_tx_callback(struct can_async_descriptor *const descr)
{
	(void)descr;
}
void CAN_CTRL_rx_callback(struct can_async_descriptor *const descr)
{
	struct can_message msg;
	uint8_t            data[64];
	msg.data = data;
	can_async_read(descr, &msg);
	return;
}

/**
 * Example of using CAN_CTRL to Encrypt/Decrypt datas.
 */
void CAN_CTRL_example(void)
{
	struct can_message msg;
	struct can_filter  filter;
	uint8_t            send_data[4];
	send_data[0] = 0x00;
	send_data[1] = 0x01;
	send_data[2] = 0x02;
	send_data[3] = 0x03;

	msg.id   = 0x45A;
	msg.type = CAN_TYPE_DATA;
	msg.data = send_data;
	msg.len  = 4;
	msg.fmt  = CAN_FMT_STDID;
	can_async_register_callback(&CAN_CTRL, CAN_ASYNC_TX_CB, (FUNC_PTR)CAN_CTRL_tx_callback);
	can_async_enable(&CAN_CTRL);

	/**
	 * CAN_CTRL_tx_callback callback should be invoked after call
	 * can_async_write, and remote device should recieve message with ID=0x45A
	 */
	can_async_write(&CAN_CTRL, &msg);

	msg.id  = 0x100000A5;
	msg.fmt = CAN_FMT_EXTID;
	/**
	 * remote device should recieve message with ID=0x100000A5
	 */
	can_async_write(&CAN_CTRL, &msg);

	/**
	 * CAN_CTRL_rx_callback callback should be invoked after call
	 * can_async_set_filter and remote device send CAN Message with the same
	 * content as the filter.
	 */
	can_async_register_callback(&CAN_CTRL, CAN_ASYNC_RX_CB, (FUNC_PTR)CAN_CTRL_rx_callback);
	filter.id   = 0x469;
	filter.mask = 0;
	can_async_set_filter(&CAN_CTRL, 0, CAN_FMT_STDID, &filter);

	filter.id   = 0x10000096;
	filter.mask = 0;
	can_async_set_filter(&CAN_CTRL, 1, CAN_FMT_EXTID, &filter);
}
