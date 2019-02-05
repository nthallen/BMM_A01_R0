#ifndef CAN_CONTROL_H_INCLUDED
#define CAN_CONTROL_H_INCLUDED
#include <stdint.h>
#include <stdbool.h>
#include <hal_can_async.h>
#include <err_codes.h>
#include "subbus.h"

#define CAN_BOARD_INSTRUMENT "SCoPEx"
#define CAN_BOARD_BOARD_TYPE "BMM"
#define CAN_BOARD_BOARD_REV "Rev A"
#define CAN_BOARD_FIRMWARE_REV "V1.0"
#define CAN_BOARD_TYPE 10
#define CAN_BOARD_SN 1
#define CAN_BOARD_ID 0x01
#define CAN_BOARD_LOCATION "28V BUS"

#define CAN_BOARD_REV #CAN_BOARD_INSTRUMENT " " #CAN_BOARD_BOARD_TYPE " " #CAN_BOARD_BOARD_REV " " #CAN_BOARD_FIRMWARE_REV \
                      " S/N " #CAN_BOARD_SN " CAN ID " #CAN_BOARD_ID " " #CAN_BOARD_LOCATION

#define CAN_BASE_ADDR 0x34
#define CAN_HIGH_ADDR 0x35

extern bool can_tx_completed;

int32_t can_control_read(struct can_message *msg);
int32_t can_control_write(uint16_t ID, uint8_t *data, int nb);

#endif
