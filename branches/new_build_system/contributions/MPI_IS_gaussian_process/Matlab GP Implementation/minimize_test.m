function [ x ] = minimize_test(  )
   % rosenbrock([1 1])
    handle = @(x) (xsquared(x))
    x = re_minimize(2,[1],[0.5],handle,2)
end

function [f, df] = xsquared(x)
  f = x * x;
  df = 2 * x;
end

function [f, df] = xsquaredysquared(z)
  x = z(1,1)
  y = z(1,2)
  f = x*x + y*y;
  df = [2*x 2*y]
end

function [f, df] = rosenbrock(z)
    a = 1;
    b = 100;
    
    x = z(1,1);
    y = z(1,2);
    
    f  = (a - x)^2 + b * (y - x^2)^2
    df = [4*b*x^3 - 4*b*y*x + 2*x - 2*a 
          2*b*y - 2*b*x^2]
    2

end