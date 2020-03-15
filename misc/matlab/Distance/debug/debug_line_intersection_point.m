clear all

addpath('..')

data = read_debug_data();

particle_point = data{1,4};
particle_dir = data{1,5};

sphere_id = data{4,2}(2);
sphere_span = data{4,3+sphere_id};
sphere_center = (sphere_span(3:2:7) + sphere_span(2:2:6))./2;
sphere_radius = (sphere_span(3:2:7) - sphere_span(2:2:6))./2;

dt = sphere_line_intersection(sphere_center, sphere_radius(1), particle_point, particle_dir);