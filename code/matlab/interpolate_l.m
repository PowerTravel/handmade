function [ X ] = interpolate_l( b, e, N, eccentricity )
%INTERPOLATE_L Summary of this function goes here
%   Detailed explanation goes here

    l = e-b;
    
    lower = floor( N*(1-eccentricity)/2);
    higher=floor( N*(1+eccentricity)/2);
    count = higher - lower;
    
    X = zeros(N+1,3);
    dphi = pi/(2*count);
    t = sin(0:dphi:pi/2);
    
    inti = 1;
    for i = 1:N
        if(i < lower)
            tmp = b;
        elseif( i>higher)
            tmp = e;
        else
            tmp = b + t(inti)*l;
            inti = inti+1;
        end
       X(i,:) = tmp; 
    end
    X(N+1,:) = e;

end

