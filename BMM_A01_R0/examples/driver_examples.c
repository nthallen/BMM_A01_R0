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

/*
 * \Write data to usart interface
 *
 * \param[in] buf Data to write to usart
 * \param[in] length The number of bytes to write
 *
 * \return The number of bytes written.
 */
static uint32_t USART_Diag_write(const uint8_t *const buf, const uint16_t length)
{
	uint32_t offset = 0;

	ASSERT(buf && length);

	while (!USART_Diag_is_byte_sent())
		;
	do {
		USART_Diag_write_byte(buf[offset]);
		while (!USART_Diag_is_byte_sent())
			;
	} while (++offset < length);

	return offset;
}

/*
 * \Read data from usart interface
 *
 * \param[in] buf A buffer to read data to
 * \param[in] length The size of a buffer
 *
 * \return The number of bytes read.
 */
static uint32_t USART_Diag_read(uint8_t *const buf, const uint16_t length)
{
	uint32_t offset = 0;

	ASSERT(buf && length);

	do {
		while (!USART_Diag_is_byte_received())
			;
		buf[offset] = USART_Diag_read_byte();
	} while (++offset < length);

	return offset;
}

/**
 * Example of using USART_Diag to write the data which received from the usart interface to IO.
 */
void USART_Diag_example(void)
{
	uint8_t data[2];

	if (USART_Diag_read(data, sizeof(data)) == 2) {
		USART_Diag_write(data, 2);
	}
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
