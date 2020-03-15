function [] = draw_rect( p, l, color, linewidth )
%GET_SQUARE Summary of this function goes here
%   Detailed explanation goes here
% X is the lower left point of the square so if x=0,0,0 then
% the square fills 0,0,0 to 1,1,1 if size is 1

x0 = p(1);
x1 = p(1)+l(1);

y0 = p(2);
y1 = p(2)+l(2);

z0 = p(3);
z1 = p(3)+l(3);


X = [x0, x1, x1, x1, x1, x0, x0, x0, ...    % lower horizontal
     x0, x1, x1, x1, x1, x0, x0, x0, ...    % upper horizontal
     x0, x0, x1, x1, x1, x1, x0, x0];       % vertical

Y = [y0, y0, y0, y1, y1, y1, y1, y0, ...    % lower
     y0, y0, y0, y1, y1, y1, y1, y0, ...    % upper
     y0, y0, y0, y0, y1, y1, y1, y1];       % vertical

Z = [z0, z0, z0, z0, z0, z0, z0, z0, ...    % lower
     z1, z1, z1, z1, z1, z1, z1, z1, ...    % upper
     z0, z1, z0, z1, z0, z1, z0, z1];          % vertical

hold on
plot3( X(1:8),   Y(1:8),   Z(1:8)  , color, 'linewidth', linewidth )
plot3( X(9:16),  Y(9:16),  Z(9:16) , color, 'linewidth', linewidth )
plot3( X(17:18), Y(17:18), Z(17:18), color, 'linewidth', linewidth )
plot3( X(19:20), Y(19:20), Z(19:20), color, 'linewidth', linewidth )
plot3( X(21:22), Y(21:22), Z(21:22), color, 'linewidth', linewidth )
plot3( X(23:24), Y(23:24), Z(23:24), color, 'linewidth', linewidth )
hold off
end

