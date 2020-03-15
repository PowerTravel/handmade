clear
steps = 1000;
dt=0.1;
pl = true;

%Springs
x = [-2,0,0; 0,0,0; 2,0,0];
v = [ 0.5, -0.5 ,0; -1, 1 ,0; 0.5,-0.5,0];
k=2;
a=2;

N=size(x,1);
dim = size(x,2);

fs = @(k,a,dx) -k * ( dx - a );

%if(pl)
    %sol = zeros(steps,N);
%end

v_tmp = zeros(N,dim);
x_tmp = zeros(N,dim);
for i = 1:steps
 
    if i==1
        dtt = dt/2;
    else
        dtt = dt;
    end
    
    for j=1:N
        if(j==1)
            dir = x(j+1,:)-x(j,:);
            len = norm(dir);
            dir = dir/len;

            v_tmp(j,:) = v(j,:) + dtt *( -fs( k, a, len) ) * dir;
        elseif(j==N)
            dir = x(j,:)-x(j-1,:);
            len = norm(dir);
            dir = dir/len;            
            
            v_tmp(j,:) = v(j,:) + dtt *( fs( k, a, len) ) * dir ;
            
        else
            dir1 = x(j+1,:)-x(j,:);
            len1 = norm(dir1);
            dir1 = dir1/len1;
            
            dir2 = x(j,:)-x(j-1,:);
            len2 = norm(dir2);
            dir2 = dir2/len2;
            
            v_tmp(j,:) = v(j,:) + dtt *(  fs(k,a,len2)*dir2 -...
                                      fs(k,a,len1)*dir1 );
                                  
        end
        
        x_tmp(j,:) = x(j,:) + dtt*v_tmp(j,:);
    end
    
    x = x_tmp;
    v = v_tmp;
    
    if(pl)
        %center = x(2,:);
        center = [0,0,0];

        colorstring = 'kbgrymc';
        plot3(x(:,1),x(:,2),x(:,3));
        hold on
        for k=1:N
            plot3(x(k,1),x(k,2), x(k,3),'.' ,'Color', colorstring(mod(k,7)+1))
        end
        axis([-2+center(1), 2+center(1), -2+center(2), 2+center(2), -2+center(3), 2+center(3)])

        pause(0.01);
        hold off
 %   else
 %       sol(i,:) = x;
    end
    
end
