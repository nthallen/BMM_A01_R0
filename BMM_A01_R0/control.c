/******************************************************************************
*
* @file control.c
*
* Listens for commands from serial
*
* MODIFICATION HISTORY:
*
* Ver   Who	 Date	 Changes
* ----- --- -------- -----------------------------------------------
* 0.00	mr	11/2/09  Initial release
* 1.00  nta 9/8/10   Full implementation
* 2.00  nta 11/6/17  Significant rewrite for microcontroller architecture
*
*******************************************************************************/

/***************************** Include Files **********************************/
#include <ctype.h>
// #include <stdlib.h>
#include "control.h"
#include "subbus.h"
#include "usart.h"


/**
 * Reads hex string and sets return value and updates the
 * string pointer to point to the next character after the
 * hex string.
 * @param pointer to pointer to start of hex string
 * @param pointer to return value
 * @return non-zero on error (no number)
 */
static int read_hex( uint8_t **sp, uint16_t *rvp) {
  uint8_t *s = *sp;
  uint16_t rv = 0;
  if (! isxdigit(*s)) return 1;
  while ( isxdigit(*s)) {
    rv *= 16;
    if (isdigit(*s)) rv += *s - '0';
    else rv += tolower(*s) - 'a' + 10;
    ++s;
  }
  *rvp = rv;
  *sp = (uint8_t *)s;
  return 0;
}

static void hex_out(uint16_t data) {
  static uint8_t hex[] = { '0', '1', '2', '3', '4', '5', '6', '7',
                    '8','9', 'A', 'B', 'C', 'D', 'E', 'F' };
  if (data & 0xFFF0) {
    if (data & 0xFF00) {
      if (data & 0xF000)
        uart_send_char(hex[(data>>12)&0xF]);
      uart_send_char(hex[(data>>8)&0xF]);
    }
    uart_send_char(hex[(data>>4)&0xF]);
  }
  uart_send_char(hex[data&0xF]);
}

/**
 * Syntax: M<count>#<addr_range>[,<addr_range>...]
 *   <addr_range>
 *     : <addr>
 *     : <addr>:<incr>:<addr>
 *     : <count>@<addr>
 *     : <addr>|<count>@<addr>
 *        Read count2 from first addr. Read count2 or count words from
 *        second addr, whichever is less.
 * Output string: [Mm]<data>...[E\d+]
 * The output string reports acknowledge for each input address. If there is
 * no acknowledge, a zero value (i.e. 'm0') will be reported. If there is an
 * acknowledge, the hex value read will be returned preceeded by 'M'
 * (e.g. M32B5). If at any point in parsing the command string a syntax error
 * is encountered, and error code is returned to terminate the output.
 * (e.g. M32B5m0M32B6E3)
 */
static void read_multi(uint8_t *cmd) {
  uint16_t addr, start, incr, end, count, rep;
  uint16_t result;
  ++cmd;
  if ( read_hex( &cmd, &count ) || count > 500 || *cmd != '#' ) {
    SendErrorMsg("3");
    return;
  }
  ++cmd; // skip over the '#'
  for (;;) {
    if ( read_hex( &cmd, &addr ) ) {
      SendErrorMsg("3");
      return;
    }
    if (*cmd == ':' ) {
      ++cmd;
      if ( read_hex( &cmd, &incr) ||
           *cmd++ != ':' ||
           read_hex( &cmd, &end) ||
           incr >= 0x8000 ) {
        SendErrorMsg("3");
        return;
      }
      rep = count;
    } else if ( *cmd == '@' ) {
      rep = addr;
      incr = 0;
      ++cmd;
      if ( rep > count || read_hex( &cmd, &addr ) ) {
        SendErrorMsg("3");
        return;
      }
      end = addr;
    } else if (*cmd == '|') {
#if USE_SUBBUS
      if ( subbus_read( addr, &result ) ) {
        uart_send_char('M');
        hex_out(result);
      } else {
        uart_send_char('m');
        uart_send_char('0');
        result = 0;
      }
#else
      uart_send_char('m');
      uart_send_char('0');
      result = 0;
#endif
      ++cmd;
      if ( read_hex( &cmd, &rep ) ) {
        SendErrorMsg("3");
        return;
      }
      if (result < rep) {
        rep = result;
      }
      if (*cmd != '@') {
        SendErrorMsg("3");
        return;
      }
      ++cmd;
      if (read_hex(&cmd, &addr)) {
        SendErrorMsg("3");
        return;
      }
      incr = 0;
      end = addr;
    } else {
      incr = 0;
      rep = 1;
      end = addr;
    }
    for ( start = addr; addr >= start && addr <= end && rep > 0; addr += incr, --rep, --count ) {
      if ( count == 0 ) {
        SendErrorMsg("3");
        return;
      }
#if USE_SUBBUS
      if ( subbus_read( addr, &result ) ) {
        uart_send_char('M');
        hex_out(result);
      } else {
        uart_send_char('m');
        uart_send_char('0');
      }
#else
      uart_send_char('m');
      uart_send_char('0');
#endif
    }
    if (*cmd == '\n' || *cmd == '\r') {
      SendMsg("");
      return;
    } else if (*cmd++ != ',') {
      SendErrorMsg("3");
      return;
    }
  }
}

static void parse_command(uint8_t *cmd) {
  int nargs = 0;
  uint8_t cmd_code;
  uint16_t arg1, arg2;
  uint16_t rv;
  int expack;

  switch(*cmd) {
    case 'B':
    case 'f':
    case 'D':
    case 'T':
    case 'A':
    case 'V': nargs = 0; break;
    case 'M': read_multi(cmd); return;
    case 'R':
    case 'C':
    case 'S':
    case 'u':
    case 'F': nargs = 1; break;
    case 'i':
    case 'W': nargs = 2; break;
    case '\r':
    case '\n': SendMsg("0"); return; // special case: NOP
    default: SendErrorMsg("1"); return; // Code 1: Unrecognized command
  }
  cmd_code = *cmd++;
  if (nargs > 0) {
    if (read_hex(&cmd,&arg1)) {
      SendErrorMsg("3");
      return;
    }
    if (nargs > 1) {
      if (*cmd++ != ':' || read_hex(&cmd,&arg2)) {
        SendErrorMsg("3");
        return;
      }
    }
  }
  if (*cmd != '\n' && *cmd != '\r') {
    SendErrorMsg("3");
    return;
  }
  switch(cmd_code) {
    case 'R':                         // READ with ACK 'R'
#if USE_SUBBUS
      expack = subbus_read(arg1, &rv);
#else
      rv = 0;
      expack = 0;
#endif
      SendCodeVal(expack ? 'R' : 'r', rv);
      break;
    case 'W':                         // WRITE with ACK 'W'
#if USE_SUBBUS
      expack = subbus_write(arg1, arg2);
#else
      expack = 0;
#endif
      SendMsg(expack ? "W" : "w");
      break;
    case 'F':
#if USE_SUBBUS
      set_fail(arg1);
#endif
      SendMsg( "F" );
      break;
    case 'f':
#ifdef SUBBUS_FAIL_DEVICE_ID
      arg1 = XGpio_DiscreteRead(&Subbus_Fail,2);
#elif defined(SUBBUS_FAIL_ADDR)
      subbus_read(SUBBUS_FAIL_ADDR, &arg1);
#else
      arg1 = 0;
#endif
      SendCodeVal('f', arg1);
      break;
    case 'C':
    case 'S':
      // Set CmdEnbl or CmdStrobe: currently NOOP
      SendCode(cmd_code);
      break;
    case 'B':
#if USE_SUBBUS
      subbus_reset();
#if SUBBUS_INTERRUPTS
      init_interrupts();
#endif
#endif
      SendMsg("B");
      break;
    case 'V':                         // Board Rev.
      SendMsg(BOARD_REV);             // Respond with Board Rev info
      break;
    case 'D': // Read Switches
#ifdef SUBBUS_SWITCHES_DEVICE_ID
      arg1 = XGpio_DiscreteRead(&Subbus_Switches,1);
#elif defined(SUBBUS_SWITCHES_ADDR)
      subbus_read(SUBBUS_SWITCHES_ADDR, &arg1);
#else
      arg1 = 0;
#endif
      SendCodeVal('D', arg1);
      break;
    case 'T': // Tick: Currently NOOP
      break;
    case 'A': // Disarm 2-second reboot: NOOP
      SendMsg("A");
      break;
    case 'i':
#if SUBBUS_INTERRUPTS
      if ( intr_attach(arg1, arg2))
        SendCodeVal('i', arg1);
      else
#endif
        SendErrorMsg("4");
      break;
    case 'u':
#if SUBBUS_INTERRUPTS
      if ( intr_detach(arg1) )
        SendCodeVal('u', arg1);
      else
#endif
         SendErrorMsg("4");
      break;
    default:                          // Not a command
      SendErrorMsg("10");       // Should not happen
      break;
  }
}

#define RECV_BUF_SIZE USART_Diag_RX_BUFFER_SIZE
static uint8_t cmd[RECV_BUF_SIZE];							// Current Command
static int cmd_byte_num = 0;
#ifdef CMD_RCV_TIMEOUT
  static int cmd_rcv_timer = 0;
#endif

/******************************************************************************
*
* poll_control function. Reads data from uart when available and processes
* commands.
*
* @param    None
*******************************************************************************/
void poll_control(void) {
    int nr, i;
    nr = uart_recv(&cmd[cmd_byte_num], RECV_BUF_SIZE-cmd_byte_num-1);
    if (nr > 0) {
#ifdef CMD_RCV_TIMEOUT
      if (cmd_byte_num == 0) cmd_rcv_timer = 0;
#endif
      for (i = 0; i < nr && cmd_byte_num < RECV_BUF_SIZE; ++i) {
        if (cmd[cmd_byte_num] == '\n' || cmd[cmd_byte_num] == '\r') {
          cmd[++cmd_byte_num] = '\0';
          parse_command(cmd);
          cmd_byte_num = 0;
          break;
        } else {
          ++cmd_byte_num;
        }
      }
      if (cmd_byte_num >= RECV_BUF_SIZE-1) {
        SendErrorMsg("8"); // Error code 8: Too many bytes before NL
        uart_flush_input();
        cmd_byte_num = 0;
      }
#ifdef CMD_RCV_TIMEOUT
    } else if (cmd_byte_num > 0 && ++cmd_rcv_timer > CMD_RCV_TIMEOUT) {
      SendErrorMsg("2"); // Code 2: Rcv Timeout
      cmd_byte_num = 0;
#endif
    }
}

/******************************************************************************
*
* SendErrorMsg(int8_t *code) : Send Error Code Back to Host via USB
*
* @param    Error code string (w/o 'U')
* @return   None
* @note     None
*
*******************************************************************************/
void SendErrorMsg(const char *msg) {
  uart_send_char('U');
  SendMsg(msg);
}

/**
 * Sends String Back to Host via USB, appending a newline and
 * strobing the FTDI "Send Immediate" line. Every response
 * should end by calling this function.
 *
 * @param    String to be sent back to Host via USB
 * @return   None
 *
 */
void SendMsg(const char *msg) {
  while (*msg)
    uart_send_char(*msg++);
  uart_send_char('\n');			// End with NL
  uart_flush_output();
}

void SendCodeVal(int8_t code, uint16_t val) {
  uart_send_char(code);
  hex_out(val);
  SendMsg("");
}

void SendCode(int8_t code) {
  uart_send_char(code);
  SendMsg("");
}
