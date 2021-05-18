function [ack] = BMM_cmd(canobj, canid, cmd, tries)
%% **************************************************
% Protected BMM command
% Will attempt command until correct acknowledge, for 'tries' times 
% Can return one or two part vector:
% [ack, msg] = BMM_cmd(..) ack, and Error msg, if handled at higher level
% [ack] returns subbus acknowledge only
% canobj = CAN device ID
% canid = CAN ID (CAN address)
% cmd = Command number (0 - 5)
% tries = Max number of attempts before error

if cmd > 5 	
    error('Unrecognized command');
end

BMM_CMD_ADDR = 48; % 0x30 
attempt = 0; % write_ack attempts

while(attempt < tries)
  try
    [ack, msg] = canobj.write_ack(canid, BMM_CMD_ADDR, cmd); % Should I not grab returned values here, so that an error happens?
    break
  catch MExc
    disp(MExc.message);
  end
  % increment attempts, and do something if error
  attempt = attempt + 1;
  warning('Re-trying BMM Command %u', attempt);
  pause(0.5)
end
if attempt == tries
  error('Switch command Error: Too many failed attempts: %s', msg);
end
end