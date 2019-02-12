#include <string.h>
#include "driver_init.h"
#include "can_control.h"

bool can_tx_completed = true;
bool can_rx_completed = false;

typedef struct {
  uint8_t buf[CAN_MAX_TXFR];
  //* Number of bytes in the buffer. Must be <= len and must be equal
  //* to len before processing begins starts
  int nc;
  //* The number of bytes that have been processed
  int cp;
  //* The CAN ID of the message
  uint8_t id;
  //* The cmd code of the message
  uint8_t cmd;
  //* The current sequence number
  uint8_t seq;
  //* Message length, set in io_msg_init() before content is added
  uint8_t len;
  //* Indicates an error is already being sent
  bool err_flagged;
  //* Indicates the message is being built
  bool in_progress;
} can_io_buf;
static can_io_buf send_buf, recv_buf;

static void io_buf_init(can_io_buf *io) {
  io->nc = io->cp = io->len = 0;
  io->id = io->cmd = io->seq = 0;
  io->err_flagged = false;
  io->in_progress = false;
}

/**
 * @brief Add data to the specified buffer
 * @param io Pointer to buffer structure
 * @param src Pointer to the source data
 * @param len The number of bytes to be added
 * @return true if the operation will overflow the buffer
 */
static bool io_append(can_io_buf *io, uint8_t *src, int len) {
  if (len + io->nc > io->len) {
    return true;
  }
  memcpy(&io->buf[io->nc], src, len);
  io->nc += len;
  return false;
}

/**
 * @brief Setup an can_io_buf structure for a new message
 * @param io Pointer to buffer structure
 * @param cmd The command code for this message
 * @param len The number of bytes to be added
 * @return true if the setup parameters are illegal
 */
static bool io_msg_init(can_io_buf *io, uint8_t id, uint8_t cmd, int len) {
  if (io->in_progress || len > CAN_MAX_TXFR) {
    return true;
  }
  io->cp = io->nc = 0;
  io->id = id;
  io->cmd = cmd;
  io->seq = 0;
  io->len = len;
  io->in_progress = true;
  return false;
}

/**
 * @brief Flags a message as being in error
 *
 * Used to decide whether to send an error code or not, limiting errors to
 * one per message. This clears the in_progress flag and updates the id.
 * @param io Pointer to the message structure
 * @param id The message id to compare to
 * @return true if message has already been flagged
 */
static bool io_msg_flagged(can_io_buf *io, uint8_t id) {
  if (io->id == id && io->err_flagged) {
    return true;
  } else {
    io->id = id;
    io->err_flagged = true;
    return false;
  }
}

static void CAN_CTRL_tx_callback(struct can_async_descriptor *const descr) {
  can_tx_completed = true;
	(void)descr;
}

static void CAN_CTRL_rx_callback(struct can_async_descriptor *const descr) {
  can_rx_completed = true;
  (void)descr;
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
    uint8_t odata[8];
    struct can_message omsg;
    bool tx_blocked;
    bool pending;
  } cur_req;

static void can_send_error_1(uint16_t id, uint8_t err_code, uint8_t arg);
static void can_send_error_2(uint16_t id, uint8_t err_code, uint8_t arg1,
         uint8_t arg2);

static void cur_req_init(uint32_t id, uint8_t cmd, uint8_t len) {
  // cur_req.cmd = cmd;
  cur_req.omsg.id = id | CAN_ID_REPLY_BIT;
  cur_req.omsg.type = CAN_TYPE_DATA;
  cur_req.omsg.fmt = CAN_FMT_STDID;
  cur_req.omsg.data = cur_req.odata;
  cur_req.tx_blocked = false;
  cur_req.pending = false;
  cur_req.odata[0] = cmd;
  cur_req.odata[1] = len;
  cur_req.omsg.len = 2;
}

/**
 * Services transmission in progress. The raw message to be transmitted
 * is in send_buf. service_can_request() is responsible for configuring
 * cur_req.omsg for each packet and sending. If the send fails, we
 * set tx_blocked and leave everything where it is, so the send can be
 * retried later.
 * pending is set to inhibit accepting new requests while the transmission
 * is in process.
 */
static void service_can_request(bool new_request) {
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
    if (send_buf.cp >= send_buf.nc) {
      cur_req.pending = false;
      return;
    }
  }
  do {
    if (send_buf.cp == 0) {
      send_buf.seq = 1;
      cur_req_init(send_buf.id, send_buf.cmd, send_buf.len);
      cur_req.pending = true;
    } else {
      cur_req.odata[0] = send_buf.cmd | CAN_SEQ_CMD(send_buf.seq++);
      cur_req.omsg.len = 1;
    }
    { int nb_msg = 8 - cur_req.omsg.len;
      if (send_buf.cp + nb_msg > send_buf.nc) {
        nb_msg = send_buf.nc - send_buf.cp;
      }
      if (nb_msg) {
        memcpy(&cur_req.odata[cur_req.omsg.len], &send_buf.buf[send_buf.cp], nb_msg);
        cur_req.omsg.len += nb_msg;
        send_buf.cp += nb_msg;
      }
    }
    { uint32_t rv;
      rv = can_async_write(&CAN_CTRL, &cur_req.omsg);
      if (rv != ERR_NONE) {
        record_can_error(rv);
        if (rv == ERR_NO_RESOURCE) {
          cur_req.tx_blocked = true;
        } else {
          can_send_error_2(cur_req.omsg.id, CAN_ERR_OTHER, send_buf.cmd, -rv);
        }
        return;
      }
    }
  } while (send_buf.cp < send_buf.nc);
  cur_req.pending = false;
  cur_req.tx_blocked = false;
}

static void can_send_error_1(uint16_t id, uint8_t err_code, uint8_t arg) {
  if (io_msg_flagged(&recv_buf, id)) {
    return;
  }
  send_buf.in_progress = false;
  io_msg_init(&send_buf, id, CAN_CMD_CODE_ERROR, 2);
  send_buf.buf[send_buf.nc++] = err_code;
  send_buf.buf[send_buf.nc++] = arg;
  service_can_request(true);
}

static void can_send_error_2(uint16_t id, uint8_t err_code, uint8_t arg1, uint8_t arg2) {
  if (io_msg_flagged(&recv_buf, id)) {
    return;
  }
  send_buf.in_progress = false;
  io_msg_init(&send_buf, id, CAN_CMD_CODE_ERROR, 3);
  send_buf.buf[send_buf.nc++] = err_code;
  send_buf.buf[send_buf.nc++] = arg1;
  send_buf.buf[send_buf.nc++] = arg2;
  service_can_request(true);
}

void setup_can_response(void) {
  uint16_t value;
  uint8_t addr;
  bool increment = false;
  switch (recv_buf.cmd) {
    case CAN_CMD_CODE_RD:
      increment = true;
      if (io_msg_init(&send_buf, recv_buf.id, recv_buf.cmd, recv_buf.nc*2)) {
        can_send_error_1(recv_buf.id, CAN_ERR_OVERFLOW, recv_buf.cmd);
        return;
      }
      while (recv_buf.cp < recv_buf.nc) {
        uint8_t addr = recv_buf.buf[recv_buf.cp++];
        if (!subbus_read(addr, &value)) {
          can_send_error_2(recv_buf.id, CAN_ERR_NACK, recv_buf.cmd, addr);
          return;
        }
        if (io_append(&send_buf, (uint8_t*)&value, sizeof(value))) {
          can_send_error_1(recv_buf.id, CAN_ERR_OVERFLOW, recv_buf.cmd);
          return;
        }
      }
      break;
    case CAN_CMD_CODE_RD_INC:
      increment = true; // no break!
    case CAN_CMD_CODE_RD_NOINC:
    case CAN_CMD_CODE_RD_CNT_NOINC:
      if (recv_buf.cmd == CAN_CMD_CODE_RD_CNT_NOINC) {
        if (recv_buf.nc != 3) {
          can_send_error_2(recv_buf.id, CAN_ERR_INVALID_CMD, recv_buf.cmd, recv_buf.nc);
          return;
        }
        addr = recv_buf.buf[0];
        if (!subbus_read(addr, &value)) {
          can_send_error_2(recv_buf.id, CAN_ERR_NACK, recv_buf.cmd, addr);
          return;
        }
        if (value > recv_buf.buf[1]) {
          value = recv_buf.buf[1];
        }
        if (io_msg_init(&send_buf, recv_buf.id, recv_buf.cmd, (value+1)*2) ||
            io_append(&send_buf, (uint8_t*)&value, sizeof(value))) {
          can_send_error_1(recv_buf.id, CAN_ERR_OVERFLOW, recv_buf.cmd);
          return;
        }
        addr = recv_buf.buf[2];
      } else {
        if (recv_buf.nc != 2) {
          can_send_error_2(recv_buf.id, CAN_ERR_INVALID_CMD, recv_buf.cmd, recv_buf.nc);
          return;
        }
        if (io_msg_init(&send_buf, recv_buf.id, recv_buf.cmd, recv_buf.buf[0]*2)) {
          can_send_error_1(recv_buf.id, CAN_ERR_OVERFLOW, recv_buf.cmd);
          return;
        }
        addr = recv_buf.buf[1];
      }
      {
        while (send_buf.nc < send_buf.len) {
          if (!subbus_read(addr, &value)) {
            can_send_error_2(recv_buf.id, CAN_ERR_NACK, recv_buf.cmd, addr);
            return;
          }
          if (io_append(&send_buf, (uint8_t*)&value, sizeof(value))) {
            can_send_error_1(recv_buf.id, CAN_ERR_OVERFLOW, recv_buf.cmd);
            return;
          }
          if (increment) {
            ++addr;
          }
        }
      }
      break;
    case CAN_CMD_CODE_WR_INC:
      increment = true; // no break!
    case CAN_CMD_CODE_WR_NOINC:
      if ((recv_buf.nc & 1) == 0) {
        can_send_error_2(recv_buf.id, CAN_ERR_INVALID_CMD, recv_buf.cmd, recv_buf.nc);
        return;
      }
      addr = recv_buf.buf[recv_buf.cp++];
      while (recv_buf.cp+1 < recv_buf.nc) {
        value = recv_buf.buf[recv_buf.cp++];
        value += (recv_buf.buf[recv_buf.cp++] << 8);
        if (!subbus_write(addr, value)) {
          can_send_error_2(recv_buf.id, CAN_ERR_NACK, recv_buf.cmd, addr);
          return;
        }
        if (increment) {
          ++addr;
        }
      }
      if (io_msg_init(&send_buf, recv_buf.id, recv_buf.cmd, 0)) {
        can_send_error_1(recv_buf.id, CAN_ERR_OVERFLOW, recv_buf.cmd);
        return;
      }
      break;
    default:
      assert(false,__FILE__,__LINE__);
  }
  send_buf.in_progress = false;
  service_can_request(true);
}

static void process_can_request(struct can_message *msg) {
  if (CAN_REQUEST_MATCH(msg->id,CAN_BOARD_ID) &&
        // msg->type == CAN_TYPE_DATA &&
        msg->fmt == CAN_FMT_STDID &&
        msg->len > 0) {
    uint8_t cmd = msg->data[0];
    if (recv_buf.in_progress && recv_buf.id != msg->id) {
      // Note that we missed something on the previous command
      // But we've missed our opportunity to send an error msg
      recv_buf.in_progress = false;
      // don't return
    }
    if (recv_buf.in_progress) {
      if (CAN_CMD_CODE(cmd) == recv_buf.cmd &&
          CAN_CMD_SEQ(cmd) == recv_buf.seq) {
        if (io_append(&recv_buf, &msg->data[1], msg->len-1)) {
          can_send_error_2(msg->id, CAN_ERR_OVERFLOW, cmd, msg->data[1]);
          return;
        }
        ++recv_buf.seq;
      } else {
        can_send_error_1(msg->id, CAN_ERR_INVALID_SEQ, cmd);
        return;
      }
      // check cmd, sequence
      // add data
    } else {
      // This is a new request
      if (CAN_CMD_SEQ(cmd) != 0 || msg->len < 2) {
        // ACTUAL COMPLAINT: Expected seq 0 with a minimum of 2 bytes
        can_send_error_2(msg->id, CAN_ERR_INVALID_CMD, cmd, msg->len);
        return;
      }
      if (io_msg_init(&recv_buf,msg->id, cmd, msg->data[1]) ||
          io_append(&recv_buf, &msg->data[2], msg->len-2)) {
        can_send_error_2(msg->id, CAN_ERR_OVERFLOW, cmd, msg->data[1]);
        return;
      }
    }
    if (recv_buf.nc == recv_buf.len) {
      recv_buf.in_progress = false;
      setup_can_response();
    }
  } else {
    can_send_error_1(msg->id, CAN_ERR_BAD_ADDRESS, msg->id);
  }
}

/**
 *
 */
static void can_control_init(void) {
	struct can_filter  filter;

  io_buf_init(&send_buf);
  io_buf_init(&recv_buf);
	can_async_register_callback(&CAN_CTRL, CAN_ASYNC_TX_CB, (FUNC_PTR)CAN_CTRL_tx_callback);
	can_async_register_callback(&CAN_CTRL, CAN_ASYNC_RX_CB, (FUNC_PTR)CAN_CTRL_rx_callback);
	can_async_enable(&CAN_CTRL);
  /*
   * This should allow only messages to ID 1, unless I believe the hardware
   * docs as far as I've followed them, in which case it will accept all
   * addresses.
   */
	filter.id   = CAN_ID_BOARD(CAN_BOARD_ID);
	filter.mask = CAN_ID_BOARD_MASK | CAN_ID_REPLY_BIT;
	can_async_set_filter(&CAN_CTRL, 0, CAN_FMT_STDID, &filter);
}

static subbus_cache_word_t can_cache[CAN_HIGH_ADDR-CAN_BASE_ADDR+1] = {
  { 0, 0, true,  false,  false, false, false }, // Offset 0: R: CAN_Error_0
  { 0, 0, true,  false, false, false, false },   // Offset 1: R: CAN_Error_1
  // Offset 2: R: Maximum bytes not counting cmd bytes
  { CAN_MAX_TXFR, 0, true,  false, false, false, false }
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
  if (cur_req.pending) {
    service_can_request(false);
  } else {
    struct can_message msg;
    uint8_t data[64];
    int32_t err;
    msg.data = data;
    err = can_control_read(&msg);
    if (err) {
      // can_async_read() returns ERR_NOT_FOUND if no message is waiting
      if (err != ERR_NOT_FOUND) {
        record_can_error(err);
      }
    } else process_can_request(&msg);
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

extern subbus_driver_t sb_can_desc;

static subbus_cache_word_t can_desc_cache[2] = {
  { 0, 0, true, false, false, false, false },
  { 0, 0, true, false, false, false, true }
};

static struct can_desc_t {
  const char *desc;
  int cp;
  int nc;
} can_desc;

static void can_desc_init(void) {
  can_desc.desc = CAN_BOARD_REV;
  can_desc.cp = 0;
  can_desc.nc = strlen(can_desc.desc)+1; // Include the trailing NUL
  subbus_cache_update(&sb_can_desc, SUBBUS_DESC_FIFO_SIZE_ADDR, (can_desc.nc+1)/2);
  subbus_cache_update(&sb_can_desc, SUBBUS_DESC_FIFO_ADDR,
    (can_desc.desc[0] & 0xFF) + (can_desc.desc[1]<<8));
}

static void can_desc_action(void) {
  if (can_desc_cache[1].was_read) {
    can_desc.cp += 2;
    if (can_desc.cp >= can_desc.nc) {
      can_desc.cp = 0;
    }
  }
  subbus_cache_update(&sb_can_desc, SUBBUS_DESC_FIFO_SIZE_ADDR,
    ((can_desc.nc-can_desc.cp)+1)/2);
  subbus_cache_update(&sb_can_desc, SUBBUS_DESC_FIFO_ADDR,
    (can_desc.desc[can_desc.cp] & 0xFF) + (can_desc.desc[can_desc.cp+1]<<8));
}

subbus_driver_t sb_can_desc = {
  SUBBUS_DESC_FIFO_SIZE_ADDR, SUBBUS_DESC_FIFO_ADDR,
  can_desc_cache, can_desc_init, 0, can_desc_action,
  false };
