function [] = draw_edge(edgeStart, edgeEnd, color)
  hold on
  edgeLength = edgeEnd - edgeStart;

  nrSteps = 20;
  stepsize = 1/nrSteps;
  x = stepsize:stepsize:1-stepsize;
  x1 = edgeStart;
  x2 = edgeEnd;

  f = @(x1,x2,x)  x.*(x2-x1) + x1;

  plot3( f( x1(1), x2(1), x     ), f(x1(2),x2(2), x     ), f(x1(3), x2(3), x     ),      'color', color)
  plot3( f( x1(1), x2(1), x(1)  ), f(x1(2),x2(2), x(1)  ), f(x1(3), x2(3), x(1)  ), 'o', 'color', 'r')
  plot3( f( x1(1), x2(1), x(end)), f(x1(2),x2(2), x(end)), f(x1(3), x2(3), x(end)), 'o', 'color', 'g')
  hold off
end

