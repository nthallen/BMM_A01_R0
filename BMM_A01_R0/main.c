#include <atmel_start.h>
#include "usart.h"
#include "can_control.h"
#include "subbus.h"
#include "control.h"
#include "i2c.h"
#include "commands.h"

int main(void)
{
	/* Initializes MCU, drivers and middleware */
	atmel_start_init();
  i2c_enable(I2C_ENABLE_DEFAULT);
  if (subbus_add_driver(&sb_base)
      || subbus_add_driver(&sb_fail_sw)
      || subbus_add_driver(&sb_i2c)
      || subbus_add_driver(&sb_cmd)
      || subbus_add_driver(&sb_can)
     )
  {
    while (true) ; // some driver is misconfigured.
  }
  subbus_reset();
  // uart_init();
  while (1) {
    // poll_control();
    subbus_poll();
    #if SUBBUS_INTERRUPTS
      if (subbus_intr_req)
        intr_service();
    #endif
  }
}
