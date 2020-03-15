function [T] = get_sphere_line(sphere_center, sphere_radius, line_point, line_direction)


    separation = line_point - sphere_center;
    
    ld_dot_ld = line_direction * line_direction';
    ld_dot_separation = line_direction*separation';
    sep_dot_sep = separation * separation';
    
    p = 2 * ld_dot_separation/ld_dot_ld;
    q = (sep_dot_sep - sphere_radius)/ld_dot_ld;
    
    p_half = p/2;
    p2 = p_half^2;
    
    ip1 = 0;
    ip2 = 0;
    if( ~(p2<q) )
      ip1 = -p_half - sqrt(p2 - q);
      ip2 = -p_half + sqrt(p2 - q); 
    end
    
    T = [ip1,ip2];

end