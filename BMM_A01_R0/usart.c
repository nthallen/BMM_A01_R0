#include "driver_init.h"
#include <peripheral_clk_config.h>
#include <hpl_gclk_base.h>
#include <hpl_pm_base.h>
#include "usart.h"


/*! Buffer for the receive ringbuffer */
static uint8_t USART_Diag_rx_buffer[USART_Diag_RX_BUFFER_SIZE];

/*! Buffer to accumulate output before sending */
static uint8_t USART_Diag_tx_buffer[USART_Diag_TX_BUFFER_SIZE];
 /*! The number of characters in the tx buffer */
static int nc_tx;
volatile int USART_Diag_tx_busy = 0;
static struct io_descriptor *USART_Diag_io;

static void tx_cb_USART_Diag(const struct usart_async_descriptor *const io_descr) {
	/* Transfer completed */
	USART_Diag_tx_busy = 0;
}

/**
 * \brief Callback for received characters.
 * We do nothing here, but if we don't set it up, the low-level receive character
 * function won't be called either. This is of course undocumented behavior.
 */
static void rx_cb_USART_Diag(const struct usart_async_descriptor *const io_descr) {}

static void USART_Diag_write(const uint8_t *text, int count) {
	while (USART_Diag_tx_busy) {}
	USART_Diag_tx_busy = 1;
	io_write(USART_Diag_io, text, count);
}

/* This version replaces the one in driver_init.c in order to specify the
 * the buffer size we want and distinguish the input from output buffers.
 * This is called by system_init().
 */
void USART_Diag_init(void) {
	USART_Diag_CLOCK_init();
	// usart_async_init(&USART_Diag, SERCOM3, USART_Diag_buffer, USART_DIAG_BUFFER_SIZE, (void *)NULL);
	usart_async_init(&USART_Diag, SERCOM3, USART_Diag_rx_buffer,
  	USART_Diag_RX_BUFFER_SIZE, (void *)NULL);
	USART_Diag_PORT_init();
}

void uart_init(void) {
	usart_async_register_callback(&USART_Diag, USART_ASYNC_TXC_CB, tx_cb_USART_Diag);
	usart_async_register_callback(&USART_Diag, USART_ASYNC_RXC_CB, rx_cb_USART_Diag);
	usart_async_register_callback(&USART_Diag, USART_ASYNC_ERROR_CB, 0);
	usart_async_get_io_descriptor(&USART_Diag, &USART_Diag_io);
	usart_async_enable(&USART_Diag);
  nc_tx = 0;
}

int uart_recv(uint8_t *buf, int nbytes) {
	return io_read(USART_Diag_io, (uint8_t *)buf, nbytes);
}

void uart_flush_input(void) {
  usart_async_flush_rx_buffer(&USART_Diag);
}

void uart_send_char(int8_t c) {
  if (nc_tx >= USART_Diag_TX_BUFFER_SIZE) {
    /* We can't be flushing, or nc_tx would be zero. Characters cannot be
       added to the buffer until _tx_busy is clear. */
    assert(USART_Diag_tx_busy == 0,__FILE__,__LINE__);
    uart_flush_output();
  }
  while (USART_Diag_tx_busy) {}
  assert(nc_tx < USART_Diag_TX_BUFFER_SIZE,__FILE__,__LINE__);
  USART_Diag_tx_buffer[nc_tx++] = (uint8_t)c;
}

void uart_flush_output(void) {
  int nc = nc_tx;
  assert(USART_Diag_tx_busy == 0,__FILE__,__LINE__);
  nc_tx = 0;
  USART_Diag_write(USART_Diag_tx_buffer, nc);
}
