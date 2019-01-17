function [ value, ack_out ] = read_subbus(s, addr)
  % [value, ack] = read_subbus(s, addr);
  % s: serial port object
  % addr: subbus address
  % value: data
  % ack: 1 on successful acknowledge, 0 on nack, -1 on timeout,
  %      -2 on other errors
  fprintf(s, 'R%X\n', addr);
  tline = fgetl(s);
  if isempty(tline)
    ack = -1;
  elseif tline(1) == 'R'
    ack = 1;
  elseif tline(1) == 'r'
    ack = 0;
  else
    ack = -2;
  end
  if ack >= 0
    value = hex2dec(tline(2:end));
  else
    value = 0;
  end
  if addr == 36
      fprintf(1, 'Value @ 36 is 0x%X [%s]\n', value, tline);
  end
  if nargout > 1
    ack_out = ack;
  end
