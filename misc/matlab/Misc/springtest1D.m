clear
steps = 100;
dt=0.1;
pl = true;

%Springs
x = [-1,0,1.1,2,3.1];
v = [0,0,0,0,0];
k=2;
a=1;

N=size(x,2);

fs = @(k,a,dx) -k * ( dx - a );

if(pl)
    sol = zeros(steps,N);
end

v_tmp = zeros(1,N);
x_tmp = zeros(1,N);
for i = 1:steps
 
    if i==1
        dtt = dt/2;
    else
        dtt = dt;
    end
   
    
    for j=1:N
        if(j==1)
            v_tmp(j) = v(j) + dtt *( -fs( k, a, x(j+1)-x(j)) );
        elseif(j==N)
            v_tmp(j) = v(j) + dtt *( fs( k, a, x(j)-x(j-1)) );
        else
            v_tmp(j) = v(j) + dtt *(  fs(k,a,x(j)-x(j-1))-fs(k,a,x(j+1)-x(j)) );
        end
        
        x_tmp(j) = x(j) + dtt*v_tmp(j);
    end
    
    x = x_tmp;
    v = v_tmp;

    if(pl)
        plot(x,zeros(1,N),'.');
        axis([-1 4 -1 1])
        pause(0.01);
    else
        sol(i,:) = x;
    end
    
end
