clear all

simplex = load("../data/GJKSimplexSeries.m");
nRow = size(simplex,1);
nCol = size(simplex,2);


border = 0.4;
minTot = min(min(simplex(:,2:nCol)))-border;
maxTot = max(max(simplex(:,2:nCol)))+border;
minx = min(simplex(:,2))-border;
miny = min(simplex(:,3))-border;
minz = min(simplex(:,4))-border;
maxx = max(simplex(:,2))+border;
maxy = max(simplex(:,3))+border;
maxz = max(simplex(:,4))+border;
axis([minx, maxx, miny, maxy, minz, maxz]);
%axis([minTot, maxTot, minTot, maxTot, minTot, maxTot]);

camup([0 1 0])
grid on
pbaspect([1 1 1])
rotate3d on

vertices = simplex(1:end-1,2:nCol)
plot3(0,0,0,'o', 'markersize', 10)
hold on
dim = size(vertices,1)

if (dim == 1)
  plot3(vertices(1),vertices(2),vertices(3),'o');
elseif(dim == 2)
  x1 = vertices(1,:);
  x2 = vertices(2,:);
  draw_edge(x1,x2, 'r');
elseif(dim == 3)
  x1 = vertices(1,:);
  x2 = vertices(2,:);
  x3 = vertices(3,:);
  draw_edge(x1,x2, 'b');
  draw_edge(x2,x3, 'r');
  draw_edge(x3,x1, 'r');
elseif(dim == 4)
  x1 = vertices(1,:);
  x2 = vertices(2,:);
  x3 = vertices(3,:);
  x4 = vertices(4,:);
  draw_edge(x1,x2, 'b');
  draw_edge(x2,x3, 'b');
  draw_edge(x3,x1, 'b');

  draw_edge(x2,x4, 'r');
  draw_edge(x4,x1, 'r');

  draw_edge(x3,x4, 'r');
  draw_edge(x4,x2, 'r');

  draw_edge(x1,x4, 'r');
  draw_edge(x4,x3, 'r');
end
origin = [0,0,0]
axis equal
pause
draw_edge(origin,simplex(end,2:nCol), 'k');
hold off
