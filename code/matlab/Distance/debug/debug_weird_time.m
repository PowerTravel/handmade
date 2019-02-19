clear all

addpath('..')

anim=1;

data = read_debug_data();

dt = data{1,2};
particle_radius = data{1,3};
particle_position = data{1,4};
particle_veclocity = data{1,5};

nr_coll_geoms = data{4,2}(1);
buggy_geom = data{4,2}(2)+1;
figure(1)

%% Plot particle
p = (data{1,4});
v = (data{1,5});
w = [data{1,3},data{1,3},data{1,3}];
[x,y,z]=ellipsoid(p(1),p(2),p(3),w(1),w(2),w(3),20);
hold on
surf(x, y, z,'FaceColor','g');
hold off

vel = [p', p'+ 2*particle_radius* particle_veclocity'/norm(particle_veclocity)];

hold on
plot3(vel(1,:), vel(2,:), vel(3,:), 'linewidth', 4);
hold off
traj = load('particle_traj_dump');
N = size(traj,1);
xlabel('x')
ylabel('y')
zlabel('z')
if N~=0
hold on
plot3(traj(:,1),traj(:,2), traj(:,3),'.', 'markersize', 4)
plot3(traj(1,1),traj(1,2), traj(1,3),'.g', 'markersize', 10)
plot3(traj(N,1),traj(N,2), traj(N,3),'.r', 'markersize', 10)
hold off
end
%% Plot Collision geoms
for i = 3:(nr_coll_geoms+2)
    p = (data{4,i}(3:2:7) + data{4,i}(2:2:6))./2;
    w = (data{4,i}(3:2:7) - data{4,i}(2:2:6))./2;
    [x,y,z]=ellipsoid(p(1),p(2),p(3),w(1),w(2),w(3),20);
    hold on
    if (i-2) == buggy_geom
    surf(x, y, z,'FaceColor','r');
    else 
    surf(x, y, z,'FaceColor','y');
    end
    hold off
end
alpha(0.3)


grid on
axis equal





