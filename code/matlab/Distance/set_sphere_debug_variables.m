%% input
sphere_center  = [0; 0;0];
sphere_radius = 0.55000000000000004;
line_point  =  [0.7309,  0.73099, 0.730988];
line_dir = [-0.00212813 , 0.00404455 , 0.00391235];


particle_position = [0.499979;  0.50004; 0.500039];
particle_vel = [-0.00212813;  0.00404455;  0.00391235];
particle_radius = 0.4;

contact_normal = [-0.577303; -0.577375; -0.577373];
contact_point = [0.7309;  0.73099; 0.730988];


%% derivred values
collision_normal_vector = zeros(3,2) ;
collision_normal_vector(:,1) = sphere_center;
collision_normal_vector(:,2) = sphere_center+contact_normal;

particle_velocity_vec = zeros(3,2) ;
particle_velocity_vec(:,1) = particle_position;
particle_velocity_vec(:,2) = particle_position+particle_vel./norm(particle_vel);

figure(1)
plot_epsiloid(sphere_center , [sphere_radius,sphere_radius,sphere_radius])
plot_epsiloid(particle_position , [particle_radius,particle_radius,particle_radius])
alpha(.4)
hold on
plot3(contact_point(1),contact_point(2),contact_point(3) ,'.','linewidth', 10)

% plot collision normal arrow
plot3(collision_normal_vector(1,:),collision_normal_vector(2,:),collision_normal_vector(3,:))
plot3(collision_normal_vector(1,1),collision_normal_vector(2,1),collision_normal_vector(3,1),'.','linewidth', 10)

% plot particle vel dir
plot3(particle_velocity_vec(1,:),particle_velocity_vec(2,:),particle_velocity_vec(3,:))
plot3(particle_velocity_vec(1,1),particle_velocity_vec(2,1),particle_velocity_vec(3,1),'.','linewidth', 10)

axis equal