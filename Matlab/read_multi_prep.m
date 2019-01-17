function rm_obj = read_multi_prep(varargin)
  % rm_obj = read_multi_prep(addr [, addr ...]);
  % addr can be a scalar for a single read, a [count addr] pair or
  % an [addr incr addr] triple.

  % build a command string
  N = length(varargin);
  cmds = cell(N,1);
  count = 0;
  for i=1:N
    addr = varargin{i};
    switch length(addr)
      case 1
        cmds{i} = dec2hex(addr);
        count = count+1;
      case 2
        cmds{i} = sprintf('%X@%X', addr);
        count = count+addr(1);
      case 3
        cmds{i} = sprintf('%X:%X:%X', addr);
        count = count + length(addr(1):addr(2):addr(3));
      case 4 % kluge for new syntax
        cmds{i} = sprintf('%X|%X@%X', addr(1:3));
        count = count + addr(2) + 1;
      otherwise
        error('Invalid argument');
    end
  end
  if count >= 500
    error('read_multi is limited to 500 values');
  end
  if count == 0
    error('No addresses specified');
  end
  rm_obj.count = count;
  if N > 1
    rm_obj.cmd = [sprintf('M%X#%s', count, cmds{1}) sprintf(',%s', cmds{2:end})];
  else
    rm_obj.cmd = sprintf('M%X#%s', count, cmds{1});
  end
  
