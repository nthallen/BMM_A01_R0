function [s, port_out] = serial_port_init(port)
% s = serial_port_init(port);
% [s, port] = serial_port_init;
% Returns an open serial object with timeout set to 0.1
% The serial object should be closed with the following sequence:
% fclose(s);
% delete(s);
% clear s;
%
% If opening the serial port fails, an error dialog is displayed and
% an empty array is returned.
s = [];
if nargout > 1
  port_out = '';
end
if (nargin < 1 || isempty(port)) && exist('./PulserPort.mat','file')
  rport = load('./PulserPort.mat');
  if isfield(rport,'port')
    port = rport.port;
  end
  clear rport
end
hw = instrhwinfo('serial');
if isempty(hw.AvailableSerialPorts)
  port = '';
elseif length(hw.AvailableSerialPorts) == 1
  port = hw.AvailableSerialPorts{1};
else
  if ~isempty(port) && ...
      ~any(strcmpi(port, hw.AvailableSerialPorts))
    % This is not a good choice
    port = '';
  end
end
if isempty(port)
  if isempty(hw.AvailableSerialPorts)
    % closereq;
    h = errordlg('No serial port found','Pulser Port Error','modal');
    uiwait(h);
    return;
  else
    sel = listdlg('ListString',hw.AvailableSerialPorts,...
      'SelectionMode','single','Name','Pulser', ...
      'PromptString','Select Serial Port:', ...
      'ListSize',[160 50]);
    if isempty(sel)
      % closereq;
      return;
    else
      port = hw.AvailableSerialPorts{sel};
      save PulserPort.mat port
    end
  end
end

if nargout > 1
  port_out = port;
end
isobj = 0;
isopen = 0;
try
  s = serial(port,'BaudRate',115200,'InputBufferSize',3000);
  isobj = 1;
  fopen(s);
  isopen = 1;
  set(s,'Timeout',0.1,'Terminator',10);
  warning('off','MATLAB:serial:fgetl:unsuccessfulRead');
  tline = 'a';
  while ~isempty(tline)
    tline = fgetl(s);
  end
catch ME
  h = errordlg(sprintf('Error: %s\nMessage: %s\nport = %s\n', ...
    ME.identifier, ME.message, port), ...
    'Pulser Port Error', 'modal');
  uiwait(h);
  if isopen
    fclose(s);
  end
  if isobj
    delete(s);
  end
  s = [];
end
