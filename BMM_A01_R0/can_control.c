#include <string.h>
#include "driver_init.h"
#include "can_control.h"

bool can_tx_completed = true;
bool can_rx_completed = false;
bool can_busy = false;


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

static struct {
    uint8_t idata[8];
    uint8_t odata[8];
    struct can_message omsg;
    uint8_t len;
    uint8_t cmd;
    uint8_t seq;
    uint8_t count_limit;
    uint8_t count;
    uint8_t addr;
    uint16_t value;
    bool half_word;
    bool tx_blocked;
    bool pending;
  } cur_req;

static void can_send_error_1(uint16_t id, uint8_t err_code, uint8_t arg);
static void can_send_error_2(uint16_t id, uint8_t err_code, uint8_t arg1,
         uint8_t arg2);

static void service_can_request(bool new_request) {
  // pick up where we left off...
  assert(!(new_request && cur_req.pending),__FILE__,__LINE__);
  cur_req.pending = cur_req.pending || new_request;
  if (!cur_req.pending) return;
  if (cur_req.tx_blocked) {
    uint32_t rv = can_async_write(&CAN_CTRL,&cur_req.omsg);
    switch (rv) {
      case ERR_NONE:
        cur_req.tx_blocked = false;
        break;
      default:
        record_can_error(rv);
        return;
    }
  } else {
    switch (cur_req.cmd) {
      case CAN_CMD_CODE_RD:
        cur_req.odata[0] = cur_req.cmd | CAN_CMD_SEQ(cur_req.seq++);
        cur_req.omsg.len = 1;
        if (cur_req.half_word) {
          cur_req.odata[cur_req.omsg.len++] = (cur_req.value >> 8) & 0xFF;
          cur_req.half_word = false;
        }
        while (cur_req.count < cur_req.count_limit) {
          uint32_t rv;
          while (cur_req.omsg.len < 8 && cur_req.count < cur_req.count_limit) {
            uint16_t addr = cur_req.idata[cur_req.count+1];
            if (subbus_read(addr, &cur_req.value)) {
              cur_req.odata[cur_req.omsg.len++] = cur_req.value & 0xFF;
              if (cur_req.omsg.len < 8) {
                cur_req.odata[cur_req.omsg.len++] = (cur_req.value >> 8) & 0xFF;
              } else {
                cur_req.half_word = true;
              }
              ++cur_req.count;
            } else {
              can_send_error_2(cur_req.omsg.id, CAN_ERR_NACK, cur_req.cmd, addr);
              return;
            }
          }
          rv = can_async_write(&CAN_CTRL, &cur_req.omsg);
          if (rv != ERR_NONE) {
            record_can_error(rv);
            if (rv == ERR_NO_RESOURCE) {
              cur_req.tx_blocked = true;
            } else {
              can_send_error_2(cur_req.omsg.id, CAN_ERR_OTHER, cur_req.cmd, -rv);
            }
            return;
          }
        }
        cur_req.pending = false;
        break;
      default:
        assert(false, __FILE__,__LINE__);
    }
  }
}

static void cur_req_init(uint32_t id, uint8_t cmd) {
  cur_req.cmd = cmd;
  cur_req.omsg.id = id | CAN_ID_REPLY_BIT;
  cur_req.omsg.type = CAN_TYPE_DATA;
  cur_req.omsg.fmt = CAN_FMT_STDID;
  cur_req.omsg.data = cur_req.odata;
  cur_req.len = 0;
  cur_req.seq = 0;
  cur_req.count = 0;
  cur_req.half_word = false;
  cur_req.tx_blocked = false;
  cur_req.pending = false;
}

static void can_send_error_1(uint16_t id, uint8_t err_code, uint8_t arg) {
  cur_req_init(id, CAN_CMD_CODE_ERROR);
  cur_req.odata[1] = err_code;
  cur_req.odata[2] = arg;
  cur_req.omsg.len = 3;
  service_can_request(true);
}

static void can_send_error_2(uint16_t id, uint8_t err_code, uint8_t arg1, uint8_t arg2) {
  cur_req_init(id, CAN_CMD_CODE_ERROR);
  cur_req.odata[1] = err_code;
  cur_req.odata[2] = arg1;
  cur_req.odata[3] = arg2;
  cur_req.omsg.len = 4;
  service_can_request(true);
}

void setup_can_request(struct can_message *msg) {
  cur_req_init(msg->id, msg->data[0]);
  memcpy(cur_req.idata, msg->data, msg->len);
  cur_req.len = msg->len;
  switch (cur_req.cmd) {
    case CAN_CMD_CODE_RD:
      cur_req.odata[0] =
      cur_req.count_limit = cur_req.len-1;
      break;
    default:
      assert(false,__FILE__,__LINE__);
  }
  service_can_request(true);
}

static void process_can_request(struct can_message *msg) {
  if (CAN_REQUEST_MATCH(msg->id,CAN_BOARD_ID) &&
        msg->type == CAN_TYPE_DATA &&
        msg->fmt == CAN_FMT_STDID &&
        msg->len > 0) {
    uint8_t cmd = msg->data[0];
    switch (cmd & CAN_CMD_CODE) {
      case CAN_CMD_CODE_RD:
        setup_can_request(msg);
        return;
      case CAN_CMD_CODE_WR_INC:
      case CAN_CMD_CODE_RD_INC:
      case CAN_CMD_CODE_RD_NOINC:
      case CAN_CMD_CODE_RD_CNT_NOINC:
      case CAN_CMD_CODE_WR_NOINC:
      case CAN_CMD_CODE_ERROR:
      default:
        can_send_error_1(msg->id, CAN_ERR_INVALID_CMD, cmd);
        return;
    }
  } else {
    can_send_error_1(msg->id, CAN_ERR_BAD_ADDRESS, msg->id);
    return;
  }
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
  { 0, 0, true,  false,  false, false, false }, // Offset 0: R: CAN_Error_0
  { 0, 0, true,  false, false, false, false }   // Offset 1: R: CAN_Error_1
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
  if (can_busy) {
    service_can_request(false);
  } else if (can_rx_completed) {
    struct can_message msg;
    uint8_t data[64];
    int32_t err;
    msg.data = data;
    err = can_control_read(&msg);
    if (err) record_can_error(err);
    else process_can_request(&msg);
  }
}

subbus_driver_t sb_can = {
  CAN_BASE_ADDR, CAN_HIGH_ADDR, // address range
  can_cache,
  can_control_init,
  poll_can_control,
  0, // Dynamic function
  false
};
