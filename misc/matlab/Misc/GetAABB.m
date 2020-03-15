function [X,Y, VX, VY ] = GetAABB( AABB, V )

 X = [ AABB(1,1), AABB(2,1), AABB(2,1), AABB(1,1), AABB(1,1)];
 Y = [ AABB(1,2), AABB(1,2), AABB(2,2), AABB(2,2), AABB(1,2)];

 CM = ( AABB(1,:) + AABB(2,:) )/2;
 VX = [CM(1), CM(1)+ V(1)];
 VY = [CM(2), CM(2)+ V(2)];
 
end

