while true
timestamps = [];
measurements = [];
size = 0;

% Receive the amount of elements
received = false;
while not(received)
   try
       message = judp('receive', 1308, 8, 3000);
       received = true;
       size = typecast(message, 'double');
    
       % Send random value back to C++, so C++ continues
       judp('send', 1309, '127.0.0.1', typecast(4.0, 'int8'));

   catch err
        if strcmp(strtrim(err.message), 'judp.m--Failed to receive UDP packet; connection timed out.')
            disp '.1'; % do nothing
            % judp('send', 1309, '127.0.0.1', typecast(-dtc*u/as_px, 'int8'));
        else
            rethrow(err);
        end
   end
    
end

% Receive measurements
received = false;
while not(received)
   try
       message = judp('receive', 1308, 181*8, 3000);
       received = true;
       for i=0:(size-1)
          block = message(i * 8 + 1 : (i + 1) * 8)
          measurements = [measurements, typecast(block, 'double')];
       end
       % Send random value back to C++, so C++ continues
       judp('send', 1309, '127.0.0.1', typecast(4.0, 'int8'));
       
   catch err
        if strcmp(strtrim(err.message), 'judp.m--Failed to receive UDP packet; connection timed out.')
            disp '.2'; % do nothing
            %judp('send', 1309, '127.0.0.1', typecast(-dtc*u/as_px, 'int8'));
        else
            rethrow(err);
        end
   end
    
end

% Receive timestamps
received = false;
while not(received)
   try
       message = judp('receive', 1308, 181*8, 3000);
       received = true;
       for i=0:(size-1)
          block = message(i * 8 + 1 : (i + 1) * 8)
          timestamps = [timestamps, typecast(block, 'double')];
       end
       
   catch err
        if strcmp(strtrim(err.message), 'judp.m--Failed to receive UDP packet; connection timed out.')
            disp '.3'; % do nothing
            %judp('send', 1309, '127.0.0.1', typecast(-dtc*u/as_px, 'int8'));
        else
            rethrow(err);
        end
   end
    
end

measurements
timestamps
judp('send', 1309, '127.0.0.1', typecast(measurements(size), 'int8'));

end