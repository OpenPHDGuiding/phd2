function z = minCubic(x, df, s0, s1, extr)   % minimizer of approximating cubic
INT = 0.1; EXT = 5.0;                    % interpolate and extrapolation limits
A = -6*df+3*(s0+s1)*x; B = 3*df-(2*s0+s1)*x;
if B<0, z = s0*x/(s0-s1); else z = -s0*x*x/(B+sqrt(B*B-A*s0*x)); end
if extr                                                 % are we extrapolating?
  if ~isreal(z) | ~isfinite(z) | z < x | z > x*EXT, z = EXT*x; end  % fix bad z
  z = max(z, (1+INT)*x);                          % extrapolate by at least INT
else                                               % else, we are interpolating
  if ~isreal(z) | ~isfinite(z) | z < 0 | z > x, z = x/2; end;       % fix bad z
  z = min(max(z, INT*x), (1-INT)*x);    % at least INT away from the boundaries
end