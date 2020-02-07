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
  arrowLength = row(:,5:7) - arrowStart;
  color = ['g';'g';'g'];
  color(row(:,1)==0) = 'r';
  quiver3(arrowStart(1,1), arrowStart(1,2), arrowStart(1,3), arrowLength(1,1), arrowLength(1,2), arrowLength(1,3), 'linewidth', 2, 'color', color(1))
  quiver3(arrowStart(2,1), arrowStart(2,2), arrowStart(2,3), arrowLength(2,1), arrowLength(2,2), arrowLength(2,3), 'linewidth', 2, 'color', color(2))
  quiver3(arrowStart(3,1), arrowStart(3,2), arrowStart(3,3), arrowLength(3,1), arrowLength(3,2), arrowLength(3,3), 'linewidth', 2, 'color', color(3))
  pause
  
end

hold off
