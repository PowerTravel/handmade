clear all
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

for (idx = 1:nRow)

  hold on

  row = polytype(idx,:)

  arrowStart = row(2:4)
  arrowLength = row(5:7) - arrowStart;
  border = row(1)
  if (border == 0)
    color = 'red';
  else
    color = 'green';
  end
  quiver3(arrowStart(1), arrowStart(2), arrowStart(3), arrowLength(1), arrowLength(2), arrowLength(3), 'linewidth', 2, 'color', color)
  pause
end

hold off
