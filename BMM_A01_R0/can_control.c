#include "can_control.h"

bool can_tx_completed = true;
bool can_rx_completed = false;


static void CAN_CTRL_tx_callback(struct can_async_descriptor *const descr) {
  can_tx_completed = true;
	(void)descr;
}

static void CAN_CTRL_rx_callback(struct can_async_descriptor *const descr) {
  can_rx_completed = true;
  (void)descr;
	// struct can_message msg;
	// uint8_t            data[64];
	// msg.data = data;
	// can_async_read(descr, &msg);
	// return;
}

int32_t can_control_read(struct can_message *msg) {
  can_rx_completed = false;
  return can_async_read(&CAN_CTRL, msg);
}

/**
 * @brief Enqueues data for CAN transmission
 * @param ID The CAN ID for the message
 * @param data Pointer to the data
 * @param nb The number of bytes of data (0-8)
 * @return
 */
int32_t can_control_write(uint16_t ID, uint8_t *data, int nb) {
	struct can_message msg;
  int32_t rv;

  if (nb < 0 || nb > 8) {
    return ERR_WRONG_LENGTH;
  }
	msg.id   = ID;
	msg.type = CAN_TYPE_DATA;
	msg.data = data;
	msg.len  = nb;
	msg.fmt  = CAN_FMT_STDID;
  can_tx_completed = false;
	return can_async_write(&CAN_CTRL, &msg);
}

static void process_can_msg(struct can_message *msg) {
  if (msg->id == CAN_BOARD_ID) {

  }
}

static void record_can_error(int32_t err) {
  uint16_t bits;
  err = -err;
  if (err > 0  && err <= 15) {
    bits = 1 << err;
  } else if (err >= 16 && err <= 31) {
    bits = 1 << (err-16);
    sb_can.cache[1].cache |= bits;
    return;
  } else {
    bits = 1;
  }
  sb_can.cache[0].cache |= bits;
}

/**
 *
 */
static void can_control_init(void) {
	struct can_filter  filter;

	can_async_register_callback(&CAN_CTRL, CAN_ASYNC_TX_CB, (FUNC_PTR)CAN_CTRL_tx_callback);
	can_async_register_callback(&CAN_CTRL, CAN_ASYNC_RX_CB, (FUNC_PTR)CAN_CTRL_rx_callback);
	can_async_enable(&CAN_CTRL);
  /*
   * This should allow only messages to ID 1, unless I believe the hardware
   * docs as far as I've followed them, in which case it will accept all
   * addresses.
   */
	filter.id   = 0x1;
	filter.mask = 0;
	can_async_set_filter(&CAN_CTRL, 0, CAN_FMT_STDID, &filter);
}

static subbus_cache_word_t can_cache[CAN_HIGH_ADDR-CAN_BASE_ADDR+1] = {
  { 0, 0, true,  false,  false, false }, // Offset 0: R: CAN_Error_0
  { 0, 0, true,  false, false, false }   // Offset 1: R: CAN_Error_1
};

static void poll_can_control() {
  if (can_cache[0].was_read) {
    can_cache[0].was_read = false;
    can_cache[0].cache = 0;
  }
  if (can_cache[1].was_read) {
    can_cache[1].was_read = false;
    can_cache[1].cache = 0;
  }
  if (can_rx_completed) {
    struct can_message msg;
    uint8_t data[64];
    int32_t err;
    msg.data = data;
    err = can_control_read(&msg);
    if (err) record_can_error(err);
    else process_can_msg(msg);
  }
}

subbus_driver_t sb_can = {
  CAN_BASE_ADDR, CAN_HIGH_ADDR, // address range
  can_cache,
  can_control_init,
  poll_can_control,
  false
};
