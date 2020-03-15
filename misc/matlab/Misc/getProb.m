function [ h, n_idx ] = getProb( i,j,k )
    global frac;

    A = 10000;
    epsilon = 10e-9;
    P = zeros(7,1);
    p = zeros(7,1);
    nNeig = zeros(6,1);
    for h=1:6
        idx = [i,j,k] + getIdx(h);     %  idx is a neighbour of i,j,k

        % if the cite is unoccupied see how many occupied neighbours it has
        if frac(idx(1),idx(2),idx(3)) == 0;
            nrNeig = 0;
            for m=1:6
                cn = idx + getIdx(m);
                % if Cite neighbour is ocuppied
                if frac( cn(1), cn(2), cn(3)) ~=0
                    nrNeig = nrNeig+1;
                    nNeig(h) = nNeig(h)+1;
                end
            end
            if h>1
                p(h+1) = 1 + A * nrNeig;
                P(h+1) = P(h) + 1 + A * nrNeig;
            else
                P(h+1) = 1 + A * nrNeig;
            end
        else
            % if the cite is occupied, give a small probability 
            if h>1
                p(h+1) = epsilon ;
                P(h+1) = P(h) + epsilon;
            else
                p(h+1) = epsilon;
                P(h+1) = epsilon;
            end
        end
    end
    P;
    p;
    nNeig;
    nr = random('unif',P(1),P(7));
	for h=1:6
        if nr > P(h) && nr < P(h+1)
            n_idx = getIdx(h) + [i,j,k];
            break;
        end
	end
end

function [R] = getIdx(i)
	switch i 
        case 1
	        a = 1;
            b = 0;
            c = 0;
	    case 2
            a = -1;
            b = 0;
            c = 0;
	    case 3
            a = 0;
            b = 1;
	        c = 0;
        case 4    
	        a = 0;
            b = -1;
            c = 0;
	    case 5
            a = 0;
            b = 0;
	        c = 1;
        case 6
            a = 0;
            b = 0;
	        c = -1;
	end
    R = [a,b,c];
end
