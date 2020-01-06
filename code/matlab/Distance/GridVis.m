% Visualize the collisionGrid

gr = load('grid.txt');
gridsize = 1;

pointsPerGeom = 20;
show_cyl = true;

len = size(gr,1);
figure(1)
% parse the file
for i=1:len
    % gridbox
    if(gr(i,1)==0)
       w = [gridsize, gridsize, gridsize];
       draw_rect( gr( i, 2:4) , w, 'r', 1);

    % sphere
    elseif(gr(i,1)==1)
        p = (gr(i,3:2:7) + gr(i,2:2:6))./2;
        w = (gr(i,3:2:7) - gr(i,2:2:6))./2;
        [x,y,z]=ellipsoid(p(1),p(2),p(3),w(1),w(2),w(3),pointsPerGeom);
        hold on
        surf(x, y, z,'FaceColor','c');
        hold off

    % cylinder
    elseif(gr(i,1)==2 && show_cyl)

        %p = [gr(i, 2), gr(i, 4), gr(i, 6)];
        %w = [gr(i, 3)-gr(i, 2), gr(i, 5)-gr(i, 4),...
            %gr(i, 7)-gr(i, 6)];
        p = (gr(i,3:2:7) + gr(i,2:2:6))./2;
        w = (gr(i,3:2:7) - gr(i,2:2:6))./2;
        %draw_rect( p, w, 'b', 3 );
      %  [x,y,z]=ellipsoid(p(1),p(2),p(3),w(1),w(2),w(3),pointsPerGeom);
        hold on
        surf(x, y, z,'FaceColor','c');
        hold off
    end
    alpha(0.3)
    grid on
    axis equal
     %pause(1)
end