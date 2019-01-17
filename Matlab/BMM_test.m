%%
cd C:\Users\nort\Documents\Documents\Exp\SCoPEx\BMM_A01_R0\Matlab
%%
serial_port_clear();
%%
[s,port] = serial_port_init('COM7');
%set(s,'BaudRate',57600);
set(s,'BaudRate',9600);
%%
% First check that the board is an FCC
BdID = read_subbus(s, 3);
if BdID ~= 14
  error('Expected BdID 14. Reported %d', BdID);
end
Build = read_subbus(s,2);
fprintf(1, 'Attached to BMM %d Build # %d\n', BdID, Build);
%%
PI = read_subbus(s, 33);
PV = read_subbus(s, 34);
Vout = read_subbus(s, 35);
NReadings = read_subbus(s, 36);
Rshunt = 0.007;
fprintf(1, 'PI = (%d) %.0f A  PV = %.3f Vout = %.3f NR = %d\n', ...
    PI/16, PI*.02e-3/(16*Rshunt), PV * 0.025/16, ...
    Vout*5e-4*31.4e3/(16*2e3), NReadings);