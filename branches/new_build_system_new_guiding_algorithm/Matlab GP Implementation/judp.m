function [varargout] = judp(actionStr,varargin)
%
% judp.m--Uses Matlab's Java interface to handle User Datagram Protocol
% (UDP) communications with another application, either on the same
% computer or a remote one. 
%
% JUDP('SEND',PORT,HOST,MSSG) sends a message to the specifed port and
% host. HOST can be either a hostname (e.g., 'www.example.com') or a string
% representation of an IP address (e.g., '192.0.34.166'). Port is an
% integer port number between 1025 and 65535. The specified port must be
% open in the receiving machine's firewall.
% 
% MSSG = JUDP('RECEIVE',PORT,PACKETLENGTH) receives a message over the
% specified port. PACKETLENGTH should be set to the maximum expected
% message length to be received in the UDP packet; if too small, the
% message will be truncated.
%
% MSSG = JUDP('RECEIVE',PORT,PACKETLENGTH,TIMEOUT) attempts to receive a
% message but times out after TIMEOUT milliseconds. If TIMEOUT is not
% specified, as in the previous example, a default value is used.
%
% [MSSG,SOURCEHOST] = JUDP('RECEIVE',...) returns a string representation
% of the originating host's IP address, in addition to the received
% message. 
%
% Messages sent by judp.m are in int8 format. The int8.m function can be
% used to convert integers or characters to the correct format (use
% double.m or char.m to convert back after the datagram is received). 
% Non-integer values can be converted to int8 format using typecast.m (use
% typecast.m to convert back).
% 
% e.g.,   mssg = judp('receive',21566,10); char(mssg')
%         judp('send',21566,'208.77.188.166',int8('Howdy!'))         
%
% e.g.,   [mssg,sourceHost] = judp('receive',21566,10,5000)
%         judp('send',21566,'www.example.com',int8([1 2 3 4]))
%         
% e.g.,   mssg = judp('receive',21566,200); typecast(mssg,'double')
%         judp('send',21566,'localhost',typecast([1.1 2.2 3.3],'int8'))

% Developed in Matlab 7.8.0.347 (R2009a) on GLNX86.
% Kevin Bartlett (kpb@uvic.ca), 2009-06-18 16:11
%-------------------------------------------------------------------------

SEND = 1;
RECEIVE = 2;
DEFAULT_TIMEOUT = 1000; % [milliseconds]

% Handle input arguments.
if strcmpi(actionStr,'send')
    action = SEND;
    
    if nargin < 4
        error([mfilename '.m--SEND mode requires 4 input arguments.']);
    end % if
    
    port = varargin{1};
    host = varargin{2};
    mssg = varargin{3};
    
elseif strcmpi(actionStr,'receive')
    action = RECEIVE;
    
    if nargin < 3
        error([mfilename '.m--RECEIVE mode requires 3 input arguments.']);
    end % if
    
    port = varargin{1};
    packetLength = varargin{2};
    
    timeout = DEFAULT_TIMEOUT;
    
    if nargin > 3
        % Override default timeout if specified.
        timeout = varargin{3};
    end % if
    
else
    error([mfilename '.m--Unrecognised actionStr ''' actionStr ''.']);
end % if

% Test validity of input arguments.        
if ~isnumeric(port) || rem(port,1)~=0 || port < 1025 || port > 65535
    error([mfilename '.m--Port number must be an integer between 1025 and 65535.']);
end % if

if action == SEND
    if ~ischar(host)
        error([mfilename '.m--Host name/IP must be a string (e.g., ''www.examplecom'' or ''208.77.188.166''.).']);
    end % if
    
    if ~isa(mssg,'int8')
        %error([mfilename '.m--Mssg must be int8 format.']);
        mssg = typecast(mssg, 'int8');
    end % if
    
elseif action == RECEIVE    
    
    if ~isnumeric(packetLength) || rem(packetLength,1)~=0 || packetLength < 1
        error([mfilename '.m--packetLength must be a positive integer.']);
    end % if
    
    if ~isnumeric(timeout) || timeout <= 0
        error([mfilename '.m--timeout must be positive.']);
    end % if    
    
end % if

% Code borrowed from O'Reilly Learning Java, edition 2, chapter 12.
import java.io.*
import java.net.DatagramSocket
import java.net.DatagramPacket
import java.net.InetAddress

if action == SEND
    try
        addr = InetAddress.getByName(host);
        packet = DatagramPacket(mssg, length(mssg), addr, port);
        socket = DatagramSocket;
        socket.setReuseAddress(1);
        socket.send(packet);
        socket.close;
    catch sendPacketError
        disp('error while sending');
        try
            socket.close;
        catch closeError
            % do nothing.          
        end % try
        
        error('%s.m--Failed to send UDP packet.\nJava error message follows:\n%s',mfilename,sendPacketError.message);
        
    end % try
    
else
    try
        socket = DatagramSocket(port);
        socket.setSoTimeout(timeout);
        socket.setReuseAddress(1);
        packet = DatagramPacket(zeros(1,packetLength,'int8'),packetLength);        
        socket.receive(packet);
        socket.close;
        mssg = packet.getData;
        mssg = mssg(1:packet.getLength);     
        inetAddress = packet.getAddress;
        sourceHost = char(inetAddress.getHostAddress);
        varargout{1} = mssg;
        
        if nargout > 1
            varargout{2} = sourceHost;
        end % if
        
    catch receiveError

        % Determine whether error occurred because of a timeout.
        if ~isempty(strfind(receiveError.message,'java.net.SocketTimeoutException'))
            errorStr = sprintf('%s.m--Failed to receive UDP packet; connection timed out.\n',mfilename);
        else
            errorStr = sprintf('%s.m--Failed to receive UDP packet.\nJava error message follows:\n%s',mfilename,receiveError.message);
        end % if        

        try
            socket.close;
        catch closeError
            % do nothing.
        end % try

        error(errorStr);
        
    end % try
    
end % if

