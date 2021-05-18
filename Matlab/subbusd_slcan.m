classdef subbusd_slcan < handle
  % A subbusd_slcan object provides subbus level access to a
  % serial line CAN (SLCAN) interface.
  %
  % The subbus CAN protocol uses an SBCAN ID to identify a particular
  % node on the CAN bus. The SBCAN ID is combined with several other
  % bits to make up the 11-bit CAN ID. We generally assign SBCAN IDs
  % sequentially starting from 1. The subbus CAN protocol limits the
  % number of SBCAN IDs to 15 devices, although the mapping could be
  % adjusted if more devices were required.
  %
  % Ordinarily, the subbus API uses 16-bit addresses. In the C++
  % interface, we have formed these 16-bit addresses by combining the
  % SBCAN ID and the 8-bit board-specific register address. For the
  % interactive purposes of this Matlab library, it seemed to make sense to
  % expose the SBCAN ID as a separate parameter rather than forcing all
  % references to combine the two only to have the library separate them
  % again.
  %
  % The SBCAN_* methods will all throw errors if there are any failures.
  % the read_ack and write_ack methods suppress these errors, allowing
  % expected errors to be handled programmatically.
  %
  % To date, I have not implemented code to interpret the standard
  % string-based multi-read protocol, instead exposing the related SBCAN
  % protocols that would be the targets of that interpretation. The
  % subbusd_CAN and subbusd_slcan drivers still perform this interpretation
  % for C++ programs.
  %
  % See also:
  %  subbusd_slcan.subbusd_slcan
  %  subbusd_slcan.open
  %  subbusd_slcan.close
  %  subbusd_slcan.SBCAN_read_addrs
  %  subbusd_slcan.SBCAN_read_inc
  %  subbusd_slcan.SBCAN_read_noinc
  %  subbusd_slcan.SBCAN_read_cnt_noinc
  %  subbusd_slcan.SBCAN_read_cnt_noinc_bytes
  %  subbusd_slcan.SBCAN_read_cnt_noinc_char
  %  subbusd_slcan.SBCAN_write_inc
  %  subbusd_slcan.write_ack

  properties
    port             % The suggested port name
    s                % The serial port object
    cur_req_ID       % The current request ID number
    verbose_fileio   % If true, provide output during open and close.(true)
    verbose_slcan    % If true, print slcan packets sent or rcv (false)
    verbose_sbslcan  % If true, print subbus CAN messgs sent or rcv (false)
  end
  
  methods
    function self = subbusd_slcan(port)
      % sbsl = subbusd_slcan;
      % sbsl = subbusd_slcan(port);
      % 
      % Creates a subbusd_slcan object. If specified, port indicates
      % a preferred serial port.
      %
      % See also:
      %   subbusd_slcan.open
      %   subbusd_slcan.close
      if nargin >= 1 && ~isempty(port)
        self.port = port;
      else
        self.port = [];
      end
      s = [];
      self.cur_req_ID = 0;
      self.verbose_fileio = false;
      self.verbose_slcan = false;
      self.verbose_sbslcan = false;
    end
    
    function open(self, port)
      % s = sbsl.open([port]);
      % [s, port] = sbsl.open;
      % Returns an open serial object with timeout set to 0.1
      % The serial object should be closed with sbsl.close
      %
      % If opening the serial port fails, an error dialog is displayed and
      % an empty array is returned.
      %
      % See also:
      %   subbusd_slcan.close
      if ~isempty(self.s)
        self.close;
      end
      sl = [];
      if (nargin < 2 || isempty(port))
        if ~isempty(self.port)
          port = self.port;
        elseif exist('./CANSerialPort.mat','file')
          rport = load('./CANSerialPort.mat');
          if isfield(rport,'port')
            port = rport.port;
          end
          clear rport
        else
          port = [];
        end
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
          h = errordlg('No serial port found','CAN Serial Port Error','modal');
          uiwait(h);
          return;
        else
          sel = listdlg('ListString',hw.AvailableSerialPorts,...
            'SelectionMode','single','Name','CAN_Port', ...
            'PromptString','Select Serial Port:', ...
            'ListSize',[160 50]);
          if isempty(sel)
            % closereq;
            return;
          else
            port = hw.AvailableSerialPorts{sel};
            save CANSerialPort.mat port
          end
        end
      end
      self.port = port;
      isobj = 0;
      isopen = 0;
      try
        sl = serial(port,'BaudRate',57600,'InputBufferSize',3000);
        isobj = 1;
        fopen(sl);
        isopen = 1;
        % 
        % changed Timeout to 0.2 for too many timeouts
        set(sl,'Timeout',0.2,'Terminator',13);
        warning('off','MATLAB:serial:fgetl:unsuccessfulRead');
        tline = 'a';
        while ~isempty(tline)
          tline = fgetl(sl);
        end
        if self.verbose_fileio
          fprintf(1, 'Sending initialization:\n');
        end
        fprintf(sl, 'C\nS2\nO\nV\n'); % \n replaced with \r based on Terminator setting
        % A = fread(sl,50);
        tline = 'a';
        while ~isempty(tline)
          tline = fgetl(sl);
          if ~isempty(tline) && self.verbose_fileio
            fprintf(1, 'Recd: %s\n', tline);
          end
        end
        self.s = sl;
      catch ME
        h = errordlg(sprintf('Error: %s\nMessage: %s\nport = %s\n', ...
          ME.identifier, ME.message, port), ...
          'CAN Serial Port Error', 'modal');
        uiwait(h);
        if isopen
          fclose(sl);
        end
        if isobj
          delete(sl);
        end
        sl = [];
      end
    end
    
    function close(self)
      % sbsl.close()
      if ~isempty(self.s)
        if strcmp(self.s.Status, 'open')
          if self.verbose_fileio
            fprintf(1,'Closing port %s\n', self.s.Port);
          end
          fclose(self.s);
        end
        delete(self.s);
        self.s = [];
        self.port = [];
      end
      % Clear up any other ports too.
      sl = instrfind;
      if ~isempty(sl)
        for i=1:length(sl)
          if strcmp(sl(i).Status, 'open')
            if self.verbose_fileio
              fprintf(1,'Closing port %s\n', sl(i).Port);
            end
            fclose(sl(i));
          end
          delete(sl(i));
        end
      end
    end
    
    function CAN_send(self, CAN_id, packet)
      % sbsl.CAN_send(CAN_id, packet)
      % CAN_id is the 11-bit CAN-level ID formed by combining the SBCAN ID,
      % (bits 10:7) the reply bit (bit 6) and the request sequence number
      % (bits 5:0).
      % packet is the 2-8 byte vector that defines the CAN packet data.
      % The CAN packet's dlc is simply length(packet).
      %
      % See Also:
      %   subbusd_slcan.CAN_recv
      pkt = sprintf('t%03X%1X%s', CAN_id, length(packet), ...
        sprintf('%02x', packet));
      if self.verbose_slcan
        fprintf(1, 'Sent: %s\n', pkt);
      end
      fprintf(self.s, '%s\r', pkt);
    end
    
    function result = CAN_recv(self, CAN_id)
      % result = sbsl.CAN_recv(CAN_id, cmd_code);
      % CAN_id and cmd_code are the values previously sent to
      % SBCAN_transaction which are used for reference here and
      % error-checking.
      %
      % See Also:
      %   subbusd_slcan.CAN_send
      while true
        str = fgetl(self.s);
        if isempty(str)
          error('Timeout receiving from CAN');
        end
        if self.verbose_slcan
          fprintf(1,'Recd: %s\n', str);
        end
        % check for timeout, error response, etc.
        A = sscanf(str, 't%03X%1X');
        if length(A) ~= 2
          warning('Invalid response from CAN: "%s"', str);
          continue;
        end
        if A(1) ~= CAN_id + 2^6
          warning('Invalid response ID: %03X, expected %03X', A(1), CAN_id+2^6);
          continue;
        end
        B = sscanf(str(6:end), '%02X');
        if A(2) ~= length(B)
          warning('dlc (%d) did not match packet length (%d) "%s"', ...
            A(2), length(B));
          continue;
        end
        result = B;
        break;
      end
    end
    
    function ID = next_req_ID(self)
      ID = self.cur_req_ID;
      self.cur_req_ID = self.cur_req_ID + 1;
      if self.cur_req_ID >= 2^6
        self.cur_req_ID = 0;
      end
    end
    
    function response = SBCAN_transaction(self, dev_id, cmd_code, data)
      % response = SBCAN_transaction(dev_id, cmd_code, data);
      % dev_id: the SBCAN device ID
      % cmd_code: the SBCAN command code
      % data: a column vector of data bytes
      % response: a column vector of byte values returned
      % NACK produces and error(), which should be trappable
      % with a try/catch.
      if dev_id < 1 || dev_id > 15
        error('Invalid SBCAN device ID: %.0f', dev_id);
      end
      CAN_ID = dev_id * (2^7) + self.next_req_ID();
      cmd_seq = 0;
      req_len = length(data);
      if (req_len+1) > 32*7
        error('SBCAN transaction data exceeds max length');
      end
      n_packets = ceil((req_len+1)/7);
      bytes_sent = 0;
      req_len_byte = 1;
      req_len_pkt = req_len;
      for cmd_seq = 0:n_packets-1
        CmdSeqByte = cmd_code + cmd_seq*8;
        pktdatalen = min(7-req_len_byte, req_len - bytes_sent);
        pkt = [CmdSeqByte; req_len_pkt; data(bytes_sent+(1:pktdatalen))];
        self.CAN_send(CAN_ID, pkt);
        req_len_byte = 0;
        req_len_pkt = [];
        bytes_sent = bytes_sent + pktdatalen;
      end
      if self.verbose_sbslcan
        fprintf(1, 'Sent: %s\n', self.decode_SBSLCAN(CAN_ID, cmd_code, data));
      end
      reply = self.CAN_recv(CAN_ID);
      if length(reply) < 2
        error('Short reply (%d)', length(reply));
      end
      if reply(1) == 6
        % reply(2) is the number of bytes in the response
        if length(reply)-2 < reply(2)
          error('Error message shorter that len value');
        end
        switch reply(3)
          case 1
            msg = 'CAN_ERR_BAD_REQ_RESP';
          case 2
            if reply(2) ~= 3
              error('Expected 3 args for NACK error, recd %d', reply(2));
            end
            msg = sprintf('NACK on %s at addr 0x%02X', ...
              self.decode_cmd(reply(4)), reply(5));
          case 3
            msg = 'CAN_ERR_OTHER';
          case 4
            msg = 'CAN_ERR_INVALID_CMD';
          case 5
            msg = 'CAN_ERR_BAD_ADDRESS';
          case 6
            msg = 'CAN_ERR_OVERFLOW';
          case 7
            msg = 'CAN_ERR_INVALID_SEQ';
          otherwise
            msg = sprintf('Undefined error #%d:%s', reply(2), ...
              sprintf(' %0xX', reply));
        end
        error('%s', msg);
      end
      if reply(1) ~= cmd_code
        error('Reply cmd_code (%d) expected %d', reply(1), cmd_code);
      end
      reply_len = reply(2);
      response = zeros(reply_len,1);
      nb_pkt_data = length(reply)-2;
      response(1:nb_pkt_data) = reply(3:end);
      nb_recd = nb_pkt_data;
      cmd_seq = 1;
      while nb_recd < reply_len
        reply = self.CAN_recv(CAN_ID);
        if isempty(reply)
          error('Unexpected empty reply');
        end
        if reply(1) ~= cmd_code + cmd_seq*(2^3)
          error('cmd_seq byte (%02X) expected %02X', reply(1), ...
            cmd_code+cmd_seq*(2^3));
        end
        cmd_seq = cmd_seq + 1;
        nb_pkt_data = length(reply)-1;
        response(nb_recd+(1:nb_pkt_data)) = reply(2:end);
        nb_recd = nb_recd + nb_pkt_data;
      end
      if self.verbose_sbslcan
        fprintf(1, 'Recd: %s\n', ...
          self.decode_SBSLCAN(CAN_ID+2^6, cmd_code, response));
      end
    end
    
    function values = SBCAN_read_addrs(self, dev_id, addrs)
      % values = sbsl.SBCAN_read_addrs(dev_id, addrs);
      % dev_id: the SBCAN device ID
      % addrs: a column vector of addresses to read
      % values: a column vector of values read, unsigned
      % NACK produces and error(), which should be trappable
      % with a try/catch.
      addrs = self.check_column('SBCAN_read_addrs', 'addrs', addrs);
      response = self.SBCAN_transaction(dev_id, 0, addrs);
      if length(response) ~= 2*length(addrs)
        warning('SBCAN_read_addrs() response/address mismatch');
      end
      ii = 1:2:length(response);
      values = response(ii) + 256*response(ii+1);
    end
    
    function values = SBCAN_read_incs(self, dev_id, cmd_code, count, addr)
      % values = SBCAN_read_incs(dev_id, cmd_code, count, addr);
      % dev_id: the SBCAN device ID
      % cmd_code: either 1 for read_inc or 2 for read_noinc
      % count: the number of successive registers to read from
      % addr: the starting address
      % values: a column vector of values read, unsigned
      % NACK produces and error(), which should be trappable
      % with a try/catch.
      self.check_scalar('SBCAN_read_incs', 'cmd_code', cmd_code);
      self.check_scalar('SBCAN_read_incs', 'count', count);
      self.check_scalar('SBCAN_read_incs', 'addr', addr);
      if cmd_code ~= 1 && cmd_code ~= 2
        error('SBCAN_read_incs: Invalid cmd_code %d', cmd_code);
      end
      if count < 1 || count > 111
        error('SBCAN_read_inc: count (%d) out of range [1,111]', count);
      end
      response = self.SBCAN_transaction(dev_id, cmd_code, [count; addr]);
      if length(response) ~= 2*count
        warning('SBCAN_read_inc() response/count mismatch');
      end
      ii = 1:2:length(response);
      values = response(ii) + 256*response(ii+1);
    end
      
    function values = SBCAN_read_inc(self, dev_id, count, addr)
      % values = SBCAN_read_incs(dev_id, cmd_code, count, addr);
      % dev_id: the SBCAN device ID
      % count: the number of successive registers to read from
      % addr: the starting address
      % values: a column vector of values read, unsigned
      % NACK produces and error(), which should be trappable
      % with a try/catch.
      values = self.SBCAN_read_incs(dev_id, 1, count, addr);
    end
    
    function values = SBCAN_read_noinc(self, dev_id, count, addr)
      % values = SBCAN_read_incs(dev_id, cmd_code, count, addr);
      % dev_id: the SBCAN device ID
      % count: the number of successive registers to read from
      % addr: the starting address
      % values: a column vector of values read, unsigned
      % NACK produces and error(), which should be trappable
      % with a try/catch.
      values = self.SBCAN_read_incs(dev_id, 2, count, addr);
    end
    
    function [values, ret_cnt] = SBCAN_read_cnt_noinc_bytes(self, ...
        dev_id, cnt_addr, max_cnt, addr)
      % Reads bytes from subbus FIFO
      %
      % values = sbsl.SBCAN_read_cnt_noinc_bytes( ...
      %    dev_id, cnt_addr, max_cnt, addr);
      % [values, ret_cnt] = sbsl.SBCAN_read_cnt_noinc_bytes( ...
      %    dev_id, cnt_addr, max_cnt, addr);
      %
      % Reads the number of words available from the cnt_addr,
      % then reads that many words or max_cnt words, whichever is smaller,
      % from the second address. Returns the values as bytes.
      % If requested, returns the value read from cnt_addr as ret_cnt.
      self.check_scalar('SBCAN_read_cnt_noinc', 'dev_id', dev_id);
      self.check_scalar('SBCAN_read_cnt_noinc', 'cnt_addr', cnt_addr);
      self.check_scalar('SBCAN_read_cnt_noinc', 'max_cnt', max_cnt);
      self.check_scalar('SBCAN_read_cnt_noinc', 'addr', addr);
      res = self.SBCAN_transaction(dev_id, 3, ...
        [ cnt_addr; max_cnt; addr]);
      values = res(3:end);
      if nargout > 1
        ret_cnt = res(1) + 256*res(2);
      end
    end
    
    function [str, ret_cnt] = SBCAN_read_cnt_noinc_char(self, dev_id, ...
        cnt_addr, max_cnt, addr)
      % Reads character string from subbus FIFO
      %
      % str = sbsl.SBCAN_read_cnt_noinc_char( ...
      %    dev_id, cnt_addr, max_cnt, addr);
      % [str, ret_cnt] = sbsl.SBCAN_read_cnt_noinc_char( ...
      %    dev_id, cnt_addr, max_cnt, addr);
      %
      % Reads the number of words available from the cnt_addr,
      % then reads that many words or max_cnt words, whichever is smaller,
      % from the second address. Returns the values as a character string.
      % If requested, returns the value read from cnt_addr as ret_cnt.
      [bytes, rcnt] = self.SBCAN_read_cnt_noinc_bytes(dev_id, cnt_addr, max_cnt, addr);
      if nargout > 1
        ret_cnt = rcnt;
      end
      str = char(bytes');
    end
    
    function [values, ret_cnt] = SBCAN_read_cnt_noinc(self, dev_id, cnt_addr, max_cnt, addr)
      % Reads words from subbus FIFO
      %
      % values = sbsl.SBCAN_read_cnt_noinc( ...
      %    dev_id, cnt_addr, max_cnt, addr);
      % [values, ret_cnt] = sbsl.SBCAN_read_cnt_noinc( ...
      %    dev_id, cnt_addr, max_cnt, addr);
      %
      % Reads the number of words available from the cnt_addr,
      % then reads that many words or max_cnt words, whichever is smaller,
      % from the second address. Returns the values as 16-bit words.
      % If requested, returns the value read from cnt_addr as ret_cnt.
      [bytes, rcnt] = self.SBCAN_read_cnt_noinc_bytes(dev_id, cnt_addr, max_cnt, addr);
      ii = 1:2:length(bytes);
      values = bytes(ii) + 256*bytes(ii+1);
      if nargout > 1
        ret_cnt = rcnt;
      end
    end
    
    function SBCAN_write_inc(self, dev_id, addr, values)
      % sbsl.SBCAN_write_inc(dev_id, addr, values)
      % dev_id: The SBCAN device ID
      self.check_scalar('SBCAN_write_noinc', 'dev_id', dev_id);
      self.check_scalar('SBCAN_write_noinc', 'addr', addr);
      values = self.check_column('SBCAN_write_noinc', 'values', values);
      res = self.SBCAN_transaction(dev_id, 4, ...
        [ addr; self.words2bytes(values)]);
    end
    
    function [values, msg] = read_ack(self, dev_id, addrs)
      % values = sbsl.read_ack(dev_id, addrs);
      % [values, msg] = sbsl.read_ack(dev_id, addrs);
      %
      % read_ack wraps SBCAN_read_addrs in try/catch to return even if
      % an error is detected. The first form can be used to completely
      % ignore any errors, although the values array will be empty.
      % The other two forms 
      %
      % values will be empty if an error occurred.
      % If requested, msg is set to 'OK' or the relevant error message.
      try
        values = self.SBCAN_read_addrs(dev_id, addrs);
        if nargout > 1
          msg = 'OK';
        end
      catch ME
        if nargout > 1
          msg = ME.message;
        end
        values = [];
      end
    end
    
    function [success, msg] = write_ack(self, dev_id, addr, values)
      % success = self.write_ack(dev_id, addr, values);
      % [success, msg] = self.write_ack(dev_id, addr, values);
      % success is set to true if the write succeeds.
      % If requested, msg is set to 'OK' or the relevant error message
      try
        self.SBCAN_write_inc(dev_id, addr, values);
        if nargout > 1
          msg = 'OK';
        end
        success = true;
      catch ME
        if nargout > 1
          msg = ME.message;
        end
        success = false;
      end
    end
  end
  
  methods (Static)
    function check_scalar(func, varname, var)
      % check_scalar(funcname, varname, var)
      % Checks that var is a scalar, reporting an error if not.
      if ~isscalar(var)
        error('%s: argument %s must be scalar', func, varname);
      end
    end
    
    function varout = check_column(func, varname, var)
      % varout = check_column(funcname, varname, var)
      % Checks that var is a column or row, returning it as a column.
      % Reports and error if it is neither.
      if ~iscolumn(var)
        if isrow(var)
          var = var';
        else
          error('%s: %s must be a vector', func, varname);
        end
      end
      varout = var;
    end
    
    function bvals = words2bytes(words)
      % Internal function to convert column of 16-bit words into an
      % interleaved column of 8-bit bytes.
      words = subbusd_slcan.check_column('words2bytes', 'words', words);
      barray = [ mod(words,256), floor(words/256) ]';
      bvals = barray(:);
    end
    
    function str = decode_cmd(cmd)
      % Converts cmd code into 'Read' or 'Write'.
      switch (cmd)
        case 0
          str = 'Read';
        case 1
          str = 'Read';
        case 2
          str = 'Read';
        case 3
          str = 'Read';
        case 4
          str = 'Write';
        case 5
          str = 'Write';
        case 6
          str = 'Error';
        otherwise
          str = '<invalid cmd code>';
      end
    end
    
    function str = decode_cmd_v(cmd)
      % Converts cmd code into its explicit SBCAN command mnemonic
      switch (cmd)
        case 0
          str = 'Read';
        case 1
          str = 'Read_Inc';
        case 2
          str = 'Read_NoInc';
        case 3
          str = 'Read_Cnt_NoInc';
        case 4
          str = 'Write_Inc';
        case 5
          str = 'Write_NoInc';
        case 6
          str = 'Error';
        otherwise
          str = '<invalid cmd code>';
      end
    end
    
    function str = decode_CAN_ID(CAN_ID)
      % Decodes the CAN ID input into SBCAN ID, Sequence Number and Reply
      dev_id = floor(CAN_ID/(2^7));
      if bitand(CAN_ID, 2^6)
        rep = ' Reply';
      else
        rep = '';
      end
      seq = mod(CAN_ID,2^6);
      str = sprintf('CAN_ID:%03X=[DevID:%d seq:%d%s]', CAN_ID, dev_id, seq, rep);
    end
    
    function str = decode_SBSLCAN(CAN_ID, cmd_code, data)
      % Decodes a full subbus CAN protocol message into text
      str = sprintf('%s cmd:%d=[%s] data:%s', ...
        subbusd_slcan.decode_CAN_ID(CAN_ID), cmd_code, ...
        subbusd_slcan.decode_cmd_v(cmd_code), ...
        sprintf(' %02X',data));
    end
  end
end