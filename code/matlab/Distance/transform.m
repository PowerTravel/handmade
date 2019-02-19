ax = axes('XLim',[-1.5 1.5],'YLim',[-1.5 1.5],'ZLim',[-1.5 1.5]);
view(3)
grid on
[x,y,z] = cylinder([.2 0]);
h(1) = surface(x,y,[1:size(x,2);1:size(x,2)],'FaceColor','red');
%h(2) = surface(x,y,-z,'FaceColor','green');
%h(3) = surface(z,x,y,'FaceColor','blue');
%h(4) = surface(-z,x,y,'FaceColor','cyan');
%h(5) = surface(y,z,x,'FaceColor','magenta');
%h(6) = surface(y,-z,x,'FaceColor','yellow');