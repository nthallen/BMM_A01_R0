%%
%cd C:\Users\nort\Documents\Documents\Exp\SCoPEx\BMM_A01_R0\Matlab
%%
sbsl = subbusd_slcan;           % create the subbus_serial_CAN object
sbsl.close;                     % close its serial port
sbsl.open;                      % open its serial port
%
CAN_ID = 1; % Set as needed. Default is Test CAN_ID = 1

% First check that the board is a BMM : BdID = 10
while(1)
  try
    fprintf(1, 'Attempting CAN ID %d\n', CAN_ID);
    BdID = sbsl.SBCAN_read_addrs(CAN_ID,2); % BdID
    if BdID == 10
      fprintf(1, 'Good Connection with CAN ID %d! BMM Board ID = %d\n', CAN_ID, BdID);
  	  break
	else
      fprintf(1, 'CAN ID %d is not BMM. Returned Board ID = %d\n', CAN_ID, BdID);
    end
  catch MExc
    disp(MExc.message); % Uncomment to show 
	warning('No response from CAN ID %d\n', CAN_ID); 
  end
  prompt = 'Enter new CAN ID: \n';
  CAN_ID = input(prompt);  % Type in new CAN ID 
end

Build = sbsl.SBCAN_read_addrs(CAN_ID,3); % Addr 3 is Build number
fprintf(1, 'Attached to BMM %d Build # %d\n', BdID, Build);

%%
PI = sbsl.SBCAN_read_addrs(CAN_ID, 33);
PV = sbsl.SBCAN_read_addrs(CAN_ID, 34);
Vout = sbsl.SBCAN_read_addrs(CAN_ID, 35);
NReadings = sbsl.SBCAN_read_addrs(CAN_ID, 36);
CmdStatus = sbsl.SBCAN_read_addrs(CAN_ID, 48);
Rshunt = 0.007;
fprintf(1, 'PI = (%d) %.2f A  PV = %.3f Vout = %.3f NR = %d  cmds = %d\n', ...
    PI/16, PI*.02e-3/(16*Rshunt), PV * 0.025/16, ...
    Vout*5e-4*31.4e3/(16*2e3), NReadings, CmdStatus);
%%
% TRU_ENABLE ON (was "Shutdown Active")
BMM_cmd(sbsl, CAN_ID, 4, 3);
%%
% TRU_ENABLE OFF (was "Power on")
BMM_cmd(sbsl, CAN_ID, 5, 3);
%%
% Status LED On
BMM_cmd(sbsl, CAN_ID, 1, 3);
%%
% Status LED Off
BMM_cmd(sbsl, CAN_ID, 0, 3);
%%
% Fault LED On
BMM_cmd(sbsl, CAN_ID, 3, 3);
%%
% Fault LED Off
BMM_cmd(sbsl, CAN_ID, 2, 3);
%%
Rshunt = 0.003;
Vout_div = 2/(29.4+2);
Vout_div1 = 2/(59+2);
fprintf(1, 'PI = (%d) %.2f A  PV = %.3f Vout = %.3f NR = %d  cmds = %d\n', ...
    PI/16, PI*.02e-3/(16*Rshunt), PV * 0.025/16, ...
    Vout*5e-4*31.4e3/(16*2e3), NReadings, CmdStatus);

