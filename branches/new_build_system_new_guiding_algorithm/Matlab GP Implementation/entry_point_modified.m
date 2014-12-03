function exp055_hardware_iterative_control
% % % % % % % % % % % % % % % % % % % % % % % % % % % % % % % % % % % % % %
%
% exp055_hardware_iterative_control
%
% Q: Can we also work with iterative control in a GP setting?
% 
% A: 
%
% % % % % % % % % % % % % % % % % % % % % % % % % % % % % % % % % % % % % %
%
% Author: Edgar Klenske (edgar.klenske@tuebingen.mpg.de)
% Created: 2014-06-25
%
% % % % % % % % % % % % % % % % % % % % % % % % % % % % % % % % % % % % % %
clear all

%% colors
mpg = [0,125,122] ./ 255;
dre = [130,0,0] ./ 255; 
gre = [100,100,100] ./ 255; 
ora = [255,153,51] ./ 255;
blu = [0,0,130] ./ 255;

%%

% description and filename to save the data
desc = 'hardware run with iterative control scheme'
filename = '20140630_plain_control.mat'

% specify the used control method
method = 'ilc' % 'p', 'plain', 'gp', 'gp-es'
control_gain = 1;

% Basic Kernel Hyperparameters
hyp1 = log([
            5.234 % P_ell, length-scale of the periodic kernel
            300 %396.0294 % P_p, period-length of the periodic kernel
            0.355 % P_sf, signal-variance of the periodic kernel
            200 % SE_ell, length-scale of the SE-kernel
            sqrt(2)*0.55*0.2; % tau, model variance for delta kernel 
            ]);
hypopt = hyp1;
cov = @covfunc;
opt_hyp = 1;


sigma = 0.25; % the daytime indoor measurement noise SD is 0.25-0.35

% Prior on hyperparameters
pri.mean = exp(hyp1);
pri.sigm = [
            1 % P_ell, length-scale of the periodic kernel
            100 % P_p, period-length of the periodic kernel
            0.01 % P_sf, signal-variance of the periodic kernel
            1000 % SE_ell, length-scale of the SE-kernel
            0.1 % rho, prior for the linear kernel
            0.01; % tau, model variance for delta kernel
            ];

% zero mean function
meanfunc = @(x1) 0;

% controller parameters
dtc = 3; % controller time
TEND = 1500;
T0 = 0:dtc:TEND; % initialise the time vector
dtm = 3; % measurement time

global A B C
A = 0;
B = 1;
C = 1;

Ad = expm(A*dtc); % Zero order hold discretisation


% compute the integral over expm(A*dt)
tic
wrap = @(t) expm(A.*t);
eAt = integral(wrap, 0, dtc, 'ArrayValued', true);
Bd = eAt*B;
toc


% initialisations
x0 = 0;  % state vector
u0 = 0;  % a zero control signal
u_sequence = u0;  % the control sequence
h0 = exp(hyp1);  % the hyper vector

Tc = 0;  % time vector (control)
Tm = 0;  % time vector (measurement)
U = u0;  % control vector
X = x0;  % state vector
M = x0 + sigma*randn(1); % measurement vector
Hyp = h0;  % init hyper vector
G = [];  % measurement vector
dTc = [];  % dtc-vector
dTm = [];  % dtm-vector

XODE = x0;
TODE = 0;

x = x0;
u = u0;

% inits for the optimiser
He = [];
b = [];

% inits for the real world
t0 = clock;
t1 = clock;
as_px = 2.5221; % measured pixel resolution

disp(length(T0));

InputData  = zeros(1,length(T0));
OutputData = zeros(1,length(T0));

for i=1:length(T0) %iterate over all timepoints
    
    % Set these back to zero every iteration
    timestamps = [];
    measurements = [];
    nMeasurements = 0;
    measurement = 0;
    
    %% Receive input
    received = false;
    while not(received)
       try
           message = judp('receive', 1308, 8, 3000);
           received = true;
           measurement = typecast(message, 'double');

           % Send random value back to C++, so C++ continues
           pause(0.01);
           judp('send', 1309, '127.0.0.1', typecast(4.0, 'int8'));
           pause(0.01);
       catch err
            if strcmp(strtrim(err.message), 'judp.m--Failed to receive UDP packet; connection timed out.')
                disp '.Input'; % do nothing
                
                % judp('send', 1309, '127.0.0.1', typecast(-dtc*u/as_px, 'int8'));
            else
                rethrow(err);
            end
       end
    end

    %% Receive the amount of elements
    received = false;
    while not(received)
       try
           message = judp('receive', 1308, 8, 3000);
           received = true;
           nMeasurements = typecast(message, 'double');

           % Send random value back to C++, so C++ continues
           pause(0.01);
           judp('send', 1309, '127.0.0.1', typecast(4.0, 'int8'));
           pause(0.01);
       catch err
            if strcmp(strtrim(err.message), 'judp.m--Failed to receive UDP packet; connection timed out.')
                disp '.Size'; % do nothing
                % judp('send', 1309, '127.0.0.1', typecast(-dtc*u/as_px, 'int8'));
            else
                disp(err.message);
                rethrow(err);
            end
       end

    end

    %% Receive measurements
    received = false;
    while not(received)
       try
           message = judp('receive', 1308, nMeasurements*8, 3000);
           received = true;
           for j=0:(nMeasurements-1)
              % Get one double value a time (one double consists of 8 int8
              % values)
              block = message(j * 8 + 1 : (j + 1) * 8);
              measurements = [measurements, typecast(block, 'double')];
           end
           % Send random value back to C++, so C++ continues
           pause(0.01);
           judp('send', 1309, '127.0.0.1', typecast(4.0, 'int8'));
           pause(0.01);
       catch err
            if strcmp(strtrim(err.message), 'judp.m--Failed to receive UDP packet; connection timed out.')
                disp '.Measurements'; % do nothing
                %judp('send', 1309, '127.0.0.1', typecast(-dtc*u/as_px, 'int8'));
            else
                disp('Measurement receive error');
                rethrow(err);
            end
       end

    end

    %% Receive timestamps
    received = false;
    while not(received)
       try
           message = judp('receive', 1308, nMeasurements*8, 3000);
           received = true;
           for j=0:(nMeasurements-1)
              % Get one double value a time (one double consists of 8 int8
              % values)
              block = message(j * 8 + 1 : (j + 1) * 8);
              % cast message to double and divide by 1000 in order to get
              % from milliseconds to seconds
              timestamps = [timestamps, typecast(block, 'double') / 1000];
           end

       catch err
            if strcmp(strtrim(err.message), 'judp.m--Failed to receive UDP packet; connection timed out.')
                disp '.Timestamps'; % do nothing
                %judp('send', 1309, '127.0.0.1', typecast(-dtc*u/as_px, 'int8'));
            else
                disp(err.message);
                rethrow(err);
            end
       end

    end
    
    %% Handle the received values
    % Set the vectors that go into the covariance function
    Z = timestamps'; % we need the vectors transposed
    Y1 = measurements';
    l = nMeasurements;
    
    % Get index of the latest timestamp (needed because we?re sending a circular buffer)
    bufferSize = 100;  % Need to set this according to the size of the circular buffers in C++
    if i > bufferSize
        lastTimestampIndex = mod(i, nMeasurements);
        if lastTimestampIndex == 0;   % Matlab counts vector indices from 1
           lastTimestampIndex = nMeasurements;
        end
    else
       lastTimestampIndex = i;
    end
    
    t = timestamps(lastTimestampIndex) + 2 * dtm; % Recompute passed time
    x = measurement; % this is the noisy measurement
    Tc = [Tc, t];         % Update Control-Time Vector
    M = [M, measurement]; % Update measurement vector
    U = [U, u];           % Update control signals vector

    % X = [X, x] Only needed for simulation

    %% controller       
    if l>5
        kZZ1 = feval(cov,hypopt,Z,Z); % covariance between input locations
        CK = chol(kZZ1 + 1e-3*eye(size(kZZ1)));
        Alp = solve_chol(CK, Y1);            
        u = feval(cov,hypopt,(t+dtc/2),Z)*Alp - control_gain*x/dtc;
    else
        u = -x/dtc;
    end

    OutputData(i) = u;

    %% send control to telescope
    sent = false;
    while not(sent)
        try
            judp('send', 1309, '127.0.0.1', typecast(-dtc*u/as_px, 'int8'));
            sent = true;
        catch err
            if strncmp(strtrim(err.message), 'judp.m--Failed to send UDP packet.' , 32)
                disp('.Sending failed'); % do nothing
                disp(err.message);

            else
                rethrow(err);
            end
        end
    end
    
    %% optimise hyperparameters
    if ~mod(l,1) && l > 15 && opt_hyp% only optimise every now and then
        minfunwrap = @(h) (minfun(h,hyp1,pri,cov,meanfunc,Z,Y1));
        hyptemp = hypopt;
        while minfunwrap(hyptemp(2)-log(2))<minfunwrap(hyptemp(2))
            if exp(hyptemp(2)) > 50
                hyptemp(2) = hyptemp(2)-log(2);
            else
                hyptemp = hypopt;
                break;
            end
        end

        [hypopt(2), He, b] = re_minimize(hyptemp(2),He,b,minfunwrap,1); % optimise hyperparameters
        
        
        if exp(hypopt(2)) < 50
        	hypopt(2) = hyp1(2);
        end
        

        P1_per = exp(hypopt(2)) %#ok<NASGU,NOPRT> % print the hypers
  
        
        if exp(hypopt(2)) < 50
            keyboard
        end
    end
    
    if ~mod(i-1,1) % do not plot every iteration (lesser runtime!)
        %% inference for plots

        % build a grid for the 3D-plot
        Tp = linspace(0,1.1*TEND,500);

        if l>1
            z0 = Tp(:); % input vector: time with zero x

            kZZ1 = feval(cov,hypopt,Z,Z); % covariance between input locations
            R1 = chol(kZZ1 + 1e-3*eye(size(kZZ1))); % Cholesky of the Gram matrix
            kzZ1 = feval(cov,hypopt,z0,Z); % covariance between plot and input locations
            kzz1 = feval(cov,hypopt,z0,z0); % covariance between plot locations
            a1 = (kzZ1/R1); % first part of the inverse
            m1 = a1*(R1'\Y1); % mean prediction for zero input
            v1 = kzz1 - a1*a1'; % uncertainty (variance)
            s1 = sqrt(diag(v1)); % standard deviation
        end
        
        %% plotting
        subplot(2,1,1,'replace')
        hold on
        plot(Tc,X,'+b');         % blue plus-sign in upper plot. only used in simulation
        %% plot(Tm,F,'--c');
        plot(Tc,M,'+r');         % red plus-sign in upper plot. shows the measurements
        stairs(Tc(1:end),[U(2:end),U(end)],':m'); % Dotted stairs in upper plot

        if l>1
            subplot(2,1,2,'replace')
            hold off
            plot(Z,Y1,'+k') % black plus-sign in lower plot
            hold on
            f = [m1+2*s1; flipdim(m1-2*s1,1)];
            % Contour plot in lower plot showing the uncertainty
            fill([z0; flipdim(z0,1)], f, [0 0 1],'FaceAlpha',.1,'EdgeColor','b', 'EdgeAlpha', .4); 
            plot(z0,m1,'-b', 'LineWidth', 1);   % solid blue line in lower plot     
            xlim([min(Tp) max(Tp)])

        end
        pause(0.01) % pause and draw
        
        current_rms_error = sqrt(mean(M.^2))
    end
    
% export the results to the base workspace
assignin('base','Timestamps', timestamps);
assignin('base','Measurements', measurements);
assignin('base','Tc',Tc);
assignin('base','Tm',Tm);
assignin('base','M',M);
assignin('base','Y1',Y1);
assignin('base','Z', Z);
assignin('base','X',X);
assignin('base','InputData',InputData);
assignin('base','OutputData',OutputData);
end
if HW
    if ~exist(filename, 'file')
        save(filename, 'G', 'F_', 'F', 'FS', 'Tc', 'Tm', 'M', 'Y1', 'X', 'U', 'H', 'desc', 'method');
    else
        warning('FILE EXISTS AND COULD NOT BE SAVED!')
        keyboard
    end
end

evalin('base','rms_error = rms(M(50:end))');

keyboard % wait for interaction
end


function dx = dynamics(x,u,t)
    persistent gear_function_x gear_function_y
    if isempty(gear_function_x)
        load gear_function_2.mat
    end
    
    global A B
    
    gear = 3; % the gear factor for the correct scaling of the data
    aff = gear*interp1(gear_function_x,gear_function_y,15*t,'nearest');
    
    dx = A*x + B*u + aff;
end

function [K, D] = fastCovDirac(hyp,x1,x2)
    tau2 = exp(2*hyp);
    K = tau2*(bsxfun(@eq, x1(:,1), x2(:,1)') );
    if nargout>1
        D = 2*K;
    end
end

function [K, D] = covfunc(hyp,x1,x2)
    if nargout>1
        [K1, D1] = fastCovPS(hyp(1:4),x1(:,1),x2(:,1));
        [K2, D2] = fastCovDirac(hyp(5),x1,x2);
    
        D = zeros(length(hyp),size(x1,1),size(x2,1)); 
        D(1:4,:,:) = D1;
        D(5,:,:) = D2;
    else
        K1 = fastCovPS(hyp(1:4),x1(:,1),x2(:,1));
        K2 = fastCovDirac(hyp(5),x1,x2);
    end
    K = K1 + K2;
end

function [nll, dnll] = negloglik(hyp,covfunc,meanfunc,X,Y)
    [K, D] = feval(covfunc,hyp,X,X);
    m = meanfunc(X);
    yhat = (Y-m);
    R = chol(K);
    Z = R'\yhat;
    nll = 0.5*(Z'*Z + 2*sum(log(diag(R))));
    
    if nargout > 1
        dnll = zeros(size(D,1),1);
        for i=1:size(D,1)
            RDR = (R'\reshape(D(i,:,:),size(K)))/R;
            dnll(i) = 0.5*(-Z'*RDR*Z + trace(RDR));
        end
    end
end

function [nlp, dnlp] = neglogpri(hyp, pri)
    nlp = 0;
    dnlp = zeros(size(hyp));
    
    for i=1:length(hyp)
        [lp, dlp] = lgamma(hyp(i),pri.mean(i),pri.sigm(i));
        nlp = nlp - lp;
        dnlp(i) = - dlp;
    end
end

function [lp, dlp] = lgamma(h,xhat,sigx)
    t = -0.5*xhat + 0.5*sqrt(xhat^2+4*sigx^2);
    k = (xhat/t)+1;
    lp = (k - 1) * (h) - exp(h)/(t); % note that hyp is encoded as log(hyp)
    dlp = (k - 1) - exp(h)/(t); % derivative
end

function [nlpost, dnlpost] = neglogpost(hyp,pri,covfunc,meanfunc,X,Y)
    if nargout > 1
        [nll, dnll] = negloglik(hyp,covfunc,meanfunc,X,Y);
        [nlp, dnlp] = neglogpri(hyp,pri);
        dnlpost = dnll + dnlp;
    else
        nll = negloglik(hyp,covfunc,meanfunc,X,Y);
        nlp = neglogpri(hyp,pri);
    end
    nlpost = nll + nlp;
end

function [nlmin, dnlmin] = minfun(h,hyp,pri,covfunc,meanfunc,X,Y)
    hyp = [hyp(1); h; hyp(3:end)];
    if nargout > 1
        [nlmin, dnlpost] = neglogpost(hyp,pri,covfunc,meanfunc,X,Y);
        dnlmin = dnlpost(2);
    else
        nlmin = neglogpost(hyp,pri,covfunc,meanfunc,X,Y);
    end
end