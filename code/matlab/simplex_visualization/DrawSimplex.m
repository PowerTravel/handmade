clear all
hold off
simplex = load("../data/EPAPolytypeSeries.m");

nRow = size(simplex,1);
nCol = size(simplex,2);
div = find(simplex(:,1) < 0);
vertices = simplex(1:div-1,2:nCol)
faces = simplex(div+1:nRow,2:nCol)

draw_simplex(vertices, faces, -1);