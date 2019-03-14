
[ex,ey,ez]=ellipsoid(-0.71723682172839798,-0.20814674547121634,0.95652471515699222,0.3,0.3,0.3);
surf(ex,ey,ez);
hold on
[ex,ey,ez]=ellipsoid(-2,0,0.95652471515699222,1,1,1);
surf(ex,ey,ez);

p_start = [-0.98708953342897176,0.16016945087929513,0]

plot3([0,-0.98708953342897176],[0,0.16016945087929513],[0,0])

plot3(0,0,0, '.', 'markersize',3);
xlabel('x')
    ylabel('y')
    zlabel('z')
axis equal