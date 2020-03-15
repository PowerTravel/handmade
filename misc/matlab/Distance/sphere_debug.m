clear all
set_sphere_debug_variables;



separation = line_point-sphere_center;
ld_dot_ld = line_dir' * line_dir;

ld_dot_separation = line_dir' *separation;

sep_dot_sep = separation'*separation;

p = 2.0 * ld_dot_separation / ld_dot_ld;
q = (sep_dot_sep - sphere_radius^2) / ld_dot_ld;
 
p_half = (p/2);
p2 = p_half^2;
intersection_scalar_1 = -p_half  - sqrt( p2 - q );
intersection_scalar_2 = -p_half  + sqrt( p2 - q );


line = @(t,p, l) p + t.*l;
vstart = imag(intersection_scalar_1);
vend = imag(intersection_scalar_2);
t(1) = vstart;
steps = 100;
dt = (vend - vstart)/steps;
for i = 1:steps
    L(i,:) = line(t(i) , line_point, line_dir)';
    if(i~=steps)
        t(i+1) = t(i)+dt;
    end
end
figure(2)
[ex,ey,ez]=ellipsoid(sphere_center(1),sphere_center(2),sphere_center(3),sphere_radius,sphere_radius,sphere_radius);
surf(ex,ey,ez);
alpha(.4)
hold on
plot3(L(:,1),L(:,2),L(:,3));
plot3(L(1,1),L(1,2),L(1,3),'.','linewidth', 10)
axis equal
