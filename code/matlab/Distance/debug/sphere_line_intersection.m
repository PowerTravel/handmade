function [ dt ] = sphere_line_intersection( sphere_center, sphere_radius, line_point, line_direction )

    separation = line_point - sphere_center;
    ld_dot_ld = dot(line_direction, line_direction);
    ld_dot_separation = dot(line_direction, separation);
    sep_dot_sep = dot(separation, separation);
    
    p = 2 * ld_dot_separation / ld_dot_ld;
    q = (sep_dot_sep - sphere_radius*sphere_radius)/ld_dot_ld;
    
    [t1,t2] = pq_formula(p,q);
    
    t1
    p1 = line_point + line_direction * t1
    t2
    p2 = line_point + line_direction * t2
    
    dt = 1

end

