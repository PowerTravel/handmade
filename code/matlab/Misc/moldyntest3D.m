clear all
steps = 100;
dt=0.01;
pl = true;
sim = true;
checkFrac = false;
%Springs
x = [0,0,0; 0,0,1; 1,0,1; 1,1,1; 0,1,1; 0,1,0; 1,1,0; 1,0,0 ;2,0,0; 2,0,1; 
    2,1,1; 2,1,0; 2,1,1];
S=6;
x = gen(S);
k  = 1; 
xi = 1;
a  = 1;
alpha = 1;
rad = a/2;

N=size(x,1);
dim = size(x,2);

fs = @(k,a,dx) -k * ( dx - a );

sol = zeros(steps,N,dim);

v_tmp = zeros(N,dim);
v = v_tmp;


if checkFrac
    LAX = 1000;
    EEE = zeros(LAX,2);
    for ax=1:LAX
        x = gen(S);
        len = size(x,1);
        EEE(ax,1) = sqrt(len);
        EEE(ax,2) = norm(x(1,:) - x(len,:),2);
    end
    EEE;
    hist(EEE(:,1)-EEE(:,2))
end

if sim

for i = 1:steps
 
    if i==1
        dtt = dt/2;
    else
        dtt = dt;
    end
    
    for j=1:N
        ran = [random('unif', -1,1), random('unif', -1,1), random('unif', -1,1)];
        if(j==1)
            dir = x(j+1,:)-x(j,:);
            len = norm(dir);
            dir = dir/len;

            v_tmp(j,:) =v(j,:)+ (1/xi) * ( ( -fs( k, a, len) ) * dir + alpha * ran );
        elseif(j==N)
            dir = x(j,:)-x(j-1,:);
            len = norm(dir);
            dir = dir/len;            
            
            v_tmp(j,:) = v(j,:) + (1/xi) * ( ( fs( k, a, len) ) * dir +  alpha * ran );
            
        else
            dir1 = x(j+1,:)-x(j,:);
            len1 = norm(dir1);
            dir1 = dir1/len1;
            
            dir2 = x(j,:)-x(j-1,:);
            len2 = norm(dir2);
            dir2 = dir2/len2;
            
            v_tmp(j,:) =v(j,:)+ (1/xi) * ((fs(k,a,len2)*dir2 -...
                                      fs(k,a,len1)*dir1 ) +  alpha * ran);                            
        end
        
        x_tmp(j,:) = x(j,:) + dtt*v_tmp(j,:);
    end
    
    fixpoints = zeros(4,3);
    fixpoints(1,:) = x(1,:);
    fixpoints(2,:) = x(N,:);
    fixpoints(3,:) = v(1,:);
    fixpoints(4,:) = v(N,:);
    
    v = v_tmp;
    
    x = x_tmp;
    
    x(1,:) = fixpoints(1,:);
    x(N,:) = fixpoints(2,:);
    v(1,:) = fixpoints(3,:);
    v(N,:) = fixpoints(4,:);
   
    sol(i,:,:) = x;
end

    
    for i=1:steps
        %center = x(2,:);
        center = [0,0,0];

        colorstring = 'kbgrymc';
        plot3(sol(i,:,1),sol(i,:,2),sol(i,:,3));
        hold on
        for k=1:N
            plot3(sol(i,k,1),sol(i,k,2), sol(i,k,3),'.' ,'Color', colorstring(mod(k,7)+1))
        end
        axis([-S+center(1), S+center(1), -S+center(2), S+center(2), -S+center(3), S+center(3)])

        pause(0.01);
        hold off
    end
    
end