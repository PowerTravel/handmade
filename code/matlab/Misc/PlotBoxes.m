clear all 
A = [ 5.75000000, 6.00000000, 0.000000000;
      6.75000000, 8.00000000, 0.000000000]

AV = [ 2.00000000, -2.00000000, 0.000000000 ]

B1 = [8,4,0;
      9,5,0]
B2 = [5,8,0;
      6,9,0]
B3 = [6,8,0;
      7,9,0]
B4 = [7,8,0;
      8,9,0]
B5 = [8,8,0;
      9,9,0]
A(1,1)
hold on
[X,Y,VX,VY] = GetAABB(A, AV);
plot( X,Y )
plot( VX,VY )
[X,Y, VX, VY ] = GetAABB(B1, [0,0,0]);
plot( X,Y )
%[X,Y, VX, VY ] = GetAABB(B2, [0,0,0]);
%plot( X,Y )
%[X,Y, VX, VY ] = GetAABB(B3, [0,0,0]);
%plot( X,Y )
[X,Y, VX, VY ] = GetAABB(B4, [0,0,0]);
plot( X,Y )
%[X,Y, VX, VY ] = GetAABB(B5, [0,0,0]);
%plot( X,Y )

hold off

v = -AV;


OT1x = ( A(2,1) - B1(1,1) ) / v(1)
VT1x = ( A(1,1) - B1(2,1) ) / v(1)

OT1y = ( A(2,2) - B1(1,2) ) / v(2)
VT1y = ( A(1,2) - B1(2,2) ) / v(2)


OT2x = ( A(1,1) - B2(2,1) ) / v(1)
VT2x = ( A(2,1) - B2(1,1) ) / v(1)

OT2y = ( A(1,2) - B2(2,2) ) / v(2)
VT2y = ( A(2,2) - B2(1,2) ) / v(2)
