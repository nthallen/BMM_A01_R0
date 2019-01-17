function ack_out = write_subbus(s, addr, value)
  % ack = write_subbus(s, addr, value);
  % s: serial port object
  % addr: subbus address
  % value: data
  % ack: 1 on successful acknowledge, 0 on nack, -1 on timeout,
  %      -2 on other errors
  fprintf(s, '%s\n', sprintf('W%X:%X', addr, value));
  tline = fgetl(s);
  if isempty(tline)
    ack = -1;
  elseif tline(1) == 'W'
    ack = 1;
  elseif tline(1) == 'w'
    ack = 0;
  else
    ack = -2;
  end
  if nargout > 0
    ack_out = ack;
  elseif ack ~= 1
    error(sprintf('ack=%d on write_subbus(0x%X)', ack, addr));
  end
