#ifndef CAN_CONTROL_H_INCLUDED
#define CAN_CONTROL_H_INCLUDED
#include <stdint.h>
#include <stdbool.h>
#include <hal_can_async.h>
#include <err_codes.h>
#include "subbus.h"

// These parameters are common to all boards built with this code
#define CAN_BOARD_INSTRUMENT "SCoPEx"
#define CAN_BOARD_BOARD_TYPE "BMM"
#define CAN_BOARD_BOARD_REV "Rev A"
#define CAN_BOARD_FIRMWARE_REV "V1.0"
#define CAN_BOARD_TYPE 10

#if ! defined(CAN_BOARD_SN)
#error Must define CAN_BOARD_SN in Build Properties
#endif

#if CAN_BOARD_SN == 1
#define CAN_BOARD_ID 3
#define CAN_BOARD_LOCATION "28V Bus"
#endif

#if CAN_BOARD_SN == 3
#define CAN_BOARD_ID 1
#define CAN_BOARD_LOCATION "50V BUS"
#endif

#if ! defined(CAN_BOARD_ID) || ! defined(CAN_BOARD_LOCATION)
#error Specified CAN_BOARD_SN apparently not configured in can_control.h
#endif

#define CAN_BOARD_REV_STR(SN,ID) CAN_BOARD_INSTRUMENT " " CAN_BOARD_BOARD_TYPE " " \
     CAN_BOARD_BOARD_REV " " CAN_BOARD_FIRMWARE_REV \
     " S/N:" #SN " CAN ID:" #ID " " CAN_BOARD_LOCATION
#define CAN_BOARD_REV_XSTR(CAN_BOARD_SN,CAN_BOARD_ID) CAN_BOARD_REV_STR(CAN_BOARD_SN,CAN_BOARD_ID)
#define CAN_BOARD_REV CAN_BOARD_REV_XSTR(CAN_BOARD_SN,CAN_BOARD_ID)

#define CAN_BASE_ADDR 0x34
#define CAN_HIGH_ADDR 0x36

#define CAN_ID_BOARD_MASK 0x780
#define CAN_ID_BOARD(x) (((x)<<7)&CAN_ID_BOARD_MASK)
#define CAN_ID_REPLY_BIT 0x040
#define CAN_ID_REQID_MASK 0x3F
#define CAN_ID_REQID(x) ((x)&CAN_ID_REQID_MASK)
#define CAN_REPLY_ID(bd,req) \
    (CAN_ID_BOARD(bd)|(req&CAN_ID_REQID)|CAN_ID_REPLY_BIT)
#define CAN_REQUEST_ID(bd,req) (CAN_ID_BOARD(bd)|(req&CAN_ID_REQID))
#define CAN_REQUEST_MATCH(id,bd) \
    ((id & (CAN_ID_BOARD_MASK|CAN_ID_REPLY_BIT)) == CAN_ID_BOARD(bd))

#define CAN_CMD_CODE_MASK 0x7
#define CAN_CMD_CODE(x) ((x) & CAN_CMD_CODE_MASK)
#define CAN_CMD_CODE_RD 0x0
#define CAN_CMD_CODE_RD_INC 0x1
#define CAN_CMD_CODE_RD_NOINC 0x2
#define CAN_CMD_CODE_RD_CNT_NOINC 0x3
#define CAN_CMD_CODE_WR_INC 0x4
#define CAN_CMD_CODE_WR_NOINC 0x5
#define CAN_CMD_CODE_ERROR 0x6
#define CAN_CMD_SEQ_MASK 0xF8 // Can report up to 112 words without wrapping
#define CAN_SEQ_CMD(s) (((s)<<3)&CAN_CMD_SEQ_MASK)
#define CAN_CMD_SEQ(c) (((c)&CAN_CMD_SEQ_MASK)>>3)

#define CAN_MAX_TXFR 224

#define CAN_ERR_BAD_REQ_RESP 1
#define CAN_ERR_NACK 2
#define CAN_ERR_OTHER 3
#define CAN_ERR_INVALID_CMD 4
#define CAN_ERR_BAD_ADDRESS 5
#define CAN_ERR_OVERFLOW 6
#define CAN_ERR_INVALID_SEQ 7

extern bool can_tx_completed;

int32_t can_control_read(struct can_message *msg);
int32_t can_control_write(uint16_t ID, uint8_t *data, int nb);
extern subbus_driver_t sb_can;
extern subbus_driver_t sb_can_desc;

#endif
