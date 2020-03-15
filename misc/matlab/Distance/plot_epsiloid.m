function [ ] = plot_epsiloid( p, r )
%PLOT_SPHERE Summary of this function goes here
%   Detailed explanation goes here

[x,y,z]=ellipsoid(p(1),p(2),p(3),r(1),r(2),r(3),20);
hold on
surf(x, y, z,'FaceColor','c');
hold off

end

