%clear all
hold on
polytype = load("../data/EPAPolytypeEdges.m");

nCol = size(polytype,2);
nRow = size(polytype,1);
div = find(polytype(:,1) < 0);

border = 0.4;
camup([0 1 0])
grid on
pbaspect([1 1 1])
rotate3d on

xlabel('X')
ylabel('Y')
zlabel('Z')

triangle = 1;
for (idx = 1:3:nRow)

  hold on
  triangle
  triangle = triangle+ 1;
  row = polytype(idx:idx+2,:);

  arrowStart = row(:,2:4);
  arrowEnd = row(:,5:7);
  color = ['g';'g';'g'];
  color(row(:,1)==0) = 'r';
  draw_edge(arrowStart(1,:),arrowEnd(1,:), color(1));
  draw_edge(arrowStart(2,:),arrowEnd(2,:), color(2));
  draw_edge(arrowStart(3,:),arrowEnd(3,:), color(3));
  pause
end

hold off
