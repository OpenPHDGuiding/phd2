function [K, D] = fastCovPS(hyp, x, z)

% Stationary covariance function for a combined kernel:
% 
% Periodic*SE

if nargin<2, K = '4'; return; end                  % report number of parameters
if nargin<3, z = []; end                                   % make sure, z exists
xeqz = numel(z)==0; dg = strcmp(z,'diag') && numel(z)>0;        % determine mode

Per1_ell = exp(hyp(1));
Per1_p   = exp(hyp(2));
Per1_sf2 = exp(2*hyp(3));

SE1_ell = exp(hyp(4));

% precompute distances
if dg                                                               % vector kxx
  SD = zeros(size(x,1),1);
  AD = sqrt(SD);
else
  if xeqz                                                 % symmetric matrix Kxx
    SD = sq_dist(x');
    AD = sqrt(SD);
  else                                                   % cross covariances Kxz
    SD = sq_dist(x',z');
    AD = sqrt(SD);
  end
end

PI = pi;
P1 = pi*AD/Per1_p;
S1 = sin(P1)/Per1_ell;                                                        
Q1 = S1.*S1;

K1 = Per1_sf2*exp(-2*Q1); % covariances

E2 = SD/(SE1_ell^2);
K2 = exp(-E2/2);

K = K1.*K2;

if nargout>1 % derivatives
  D1 = 4*K1.*Q1.*K2;
  D2 = 4/Per1_ell*K1.*S1.*cos(P1).*P1.*K2;
  D3 = 2*K1.*K2;
  D4 = K2.*E2.*K1;
  D(1,:,:) = D1;
  D(2,:,:) = D2;
  D(3,:,:) = D3;
  D(4,:,:) = D4;
end