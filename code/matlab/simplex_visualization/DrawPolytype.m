clear all
hold off
polytype = load("../data/EPAPolytypeSeries.m");

nCol = size(polytype,2);
nRow = size(polytype,1);
div = find(polytype(:,1) < 0);

border = 0.4;
figure();
camup([0 1 0])
grid on
pbaspect([1 1 1])
rotate3d on

xlabel('X')
ylabel('Y')
zlabel('Z')

idx = 1;
while idx < nRow

	cla
	hold on

	vh = polytype(idx,:)
	vstart  = idx+1;
	vstop   = vstart + vh(2)-1;
	newvidx = vh(3);
	vertices     = polytype(vstart:vstop,2:nCol)
	idx = vstop+1;


	minVal = min(min(vertices)) - border;
	maxVal = max(max(vertices)) + border;
	axis([minVal, maxVal, minVal, maxVal, minVal, maxVal]);

	fh = polytype(idx,:);
	fstart = idx+1;
	fstop  = fstart+fh(2)-1;
	idx = fstop+1;

	faces = polytype(fstart:fstop, 2:nCol);

	draw_simplex(vertices,faces, 'cyan');

	if(newvidx~=0)

		newVertex = vertices(newvidx,:);
		hold on
    	quiver3(0, 0, 0, newVertex(1), newVertex(2), newVertex(3), 'linewidth', 2, 'color', 'green')
    	hold off
	end
	hold off
    pause
end
