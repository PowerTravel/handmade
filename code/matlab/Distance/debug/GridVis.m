% Visualize the collisionGrid

addpath('..');

gr = load('grid');
gridsize = gr(1,1);

pointsPerGeom = 20;         
show_cyl = true;

len = size(gr,1);
figure(1)
% parse the file
draw_grid = true;
draw_box = false;
draw_traj = true;
fast = true;
if draw_grid
for i=2:len
    % gridbox
    if(gr(i,1)==0 && draw_box)
       w = [gridsize, gridsize, gridsize];
       draw_rect( gr( i, 2:4) , w, 'r', 1);
        
    % sphere
    elseif(gr(i,1)==1)
        p = (gr(i,3:2:7) + gr(i,2:2:6))./2;
        w = (gr(i,3:2:7) - gr(i,2:2:6))./2;
        hold on
        if ~fast
            [x,y,z]=ellipsoid(p(1),p(2),p(3),w(1),w(2),w(3),pointsPerGeom);
            
            surf(x, y, z,'FaceColor','c');
            
        else
            plot3(p(1),p(2),p(3), '.', 'markersize', 10);
        end
        hold off
    % cylinder
    elseif(gr(i,1)==2 && show_cyl)
     
        %p = [gr(i, 2), gr(i, 4), gr(i, 6)];
        %w = [gr(i, 3)-gr(i, 2), gr(i, 5)-gr(i, 4),...
            %gr(i, 7)-gr(i, 6)];
        p = (gr(i,3:2:7) + gr(i,2:2:6))./2;
        w = (gr(i,3:2:7) - gr(i,2:2:6))./2;      
        %draw_rect( p, w, 'b', 3 );
        [x,y,z]=ellipsoid(p(1),p(2),p(3),w(1),w(2),w(3),pointsPerGeom);
        hold on
        if ~fast
            [x,y,z]=ellipsoid(p(1),p(2),p(3),w(1),w(2),w(3),pointsPerGeom);
            
            surf(x, y, z,'FaceColor','c');
            
        else
            cyl_idx = 1;
            wid = w(1);
            if(wid < w(2))
               wid = w(2);
               cyl_idx = 2;
            end
            if(wid <w(3))
               wid = w(3); 
               cyl_idx = 3;
            end
            cyl_line = zeros(2,3);
            cyl_line(:,1) = p;
            cyl_line(:,2) = p;
            
            cyl_line(wid_idx,1) = p(wid_idx) - wid/2; 
            cyl_line(wid_idx,2) = p(wid_idx) + wid/2; 
            plot3(cyl_line(1,:),cyl_line(2,:),cyl_line(3,:),'color','c', '-', 'linewidth', 4);
        end
        hold off
    end
    alpha(0.3)
    grid on
    axis equal
    %pause(1)   
end
end
hold on
if draw_traj
data = load('trajectory');
N = size(data,2);
xlabel('x')
ylabel('y')
zlabel('z')
plot3(data(:,1),data(:,2), data(:,3),'.', 'markersize', 4);
end