function serial_port_clear
% serial_port_clear
s = instrfind;
if isempty(s)
  fprintf(1,'No open serial ports found\n');
else
  for i=1:length(s)
    if strcmp(s(i).Status, 'open')
      fprintf(1,'Closing port %s\n', s(i).Port);
      fclose(s(i));
    end
    delete(s(i));
  end
end
