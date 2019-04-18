Firmware for the first generation Battery Monitoring Module (BMM)

The firmware is intended to identify the specific board and its logical
location within an instrument. All of the configurations are defined
in can_control.h, and the active configuration is selected by
specifying CAN_BOARD_SN (serial number). I do this using

Build->Properties->C Compiler->Symbols

adding CAN_BOARD_SN=1 for serial number 1. Further, I have created
custom Atmel Studio configurations for each serial number, so simply
switching configurations gets me to a specific board.
