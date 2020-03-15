function [ t1, t2 ] = pq_formula( p, q )

    p_half = p*0.5;
    p2 = p_half*p_half;
    
    t1 = -p_half - sqrt(p2-q);
    t2 = -p_half + sqrt(p2-q);

end

