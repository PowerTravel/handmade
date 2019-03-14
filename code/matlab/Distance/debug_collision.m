%% Particle
pos_ini = [-0.11848577319805226, 1.5310329368215119, 0];
vel_ini = [-0.81791254554919324, 0.57542946382092675, 0];
rad_p =[0.5,0.5,0.5];
%% Coll_geom
coll_geom_center = [-0.98999999999999999, 2,-0.98999999999999999];
rad_c= [1,1,1];

%% Misc
dt = 0.10000000000000001;
coll_dt = -0.062439588481947139;
coll_n = [0.62257891091856921, -0.33501346790600717, 0.70722095274464203];
contact_point_p = [-0.42977522865733686, 1.6985396707745155, -0.35361047637232101];
geom_to_sphere = [0.87151422680194778, -0.46896706317848813, 0.98999999999999999];

%%

T = get_sphere_line(coll_geom_center, rad_c(1), contact_point_p, vel_ini);

plot_epsiloid(coll_geom_center, rad_c);
plot_epsiloid(pos_ini, rad_p);
hold on
coll_p_c = zeros(3,2);
coll_p_c(:,1) = contact_point_p';
plot3(contact_point_p(1),contact_point_p(2),contact_point_p(3), '.r','markersize', 20)
alpha(.4)
axis equal

% 
% %% Derived values
% collision_normal_vector = zeros(3,2);
% collision_normal_vector(:,1) = contact_point_d';
% collision_normal_vector(:,2) = contact_point_d' + coll_n';
% 
% vel_vector = zeros(3,2);
% vel_vector(:,1) = pos_ini';
% vel_vector(:,2) = pos_ini' + vel_ini';
% 
% figure(1)
% 
% sep_x = 0.9;
% sep_y = 2;
% 
% %% Plotting
% %plot_epsiloid([sep_x, -sep_y, -sep_x], [1,1,1]);
% %plot_epsiloid([-sep_x, -sep_y, -sep_x], [1,1,1]);
% %plot_epsiloid([sep_x, -sep_y, sep_x], [1,1,1]);
% %plot_epsiloid([-sep_x, -sep_y, sep_x], [1,1,1]);
% 
% %plot_epsiloid([sep_x, sep_y, -sep_x], [1,1,1]);
% %plot_epsiloid([-sep_x, sep_y, -sep_x], [1,1,1]);
% %plot_epsiloid([sep_x, sep_y, sep_x], [1,1,1]);
% %plot_epsiloid([-sep_x, sep_y, sep_x], [1,1,1]);
% plot_epsiloid(coll_geom_center,rad_c)
% 
% plot_epsiloid(pos_ini, rad_p);
% axis equal
% alpha(.4)
% hold on
% %plot3(contact_point(1),contact_point(2),contact_point(3), '.r','linewidth',30)
% 
