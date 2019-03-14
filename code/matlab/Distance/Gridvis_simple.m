gr = load('grid.txt');
traj = load('particle_trajectory.dna');

gridsize = 1;

len = size(gr,1);


figure()
hold on
%plot3(gr(:,1),gr(:,2),gr(:,3), '.r', 'linewidth', 5)
for i=1:len
    if(gr(i,1) == 1)
       
       p = (gr(i,3:2:7) + gr(i,2:2:6))./2;
       w = (gr(i,3:2:7) - gr(i,2:2:6))./2;
       
       plot3(p(1),p(2),p(3), '.r', 'linewidth', 5)
    end

end

grid on
axis equal
N = size(traj,1);
r=5;
for i = 1:N 
   r=5;
    for i = 1:N
        hold on 
        plot3(traj(i,1),traj(i,2), traj(i,3),'.', 'linewidth', 4);
        axis([-r,r,-r,r,-r,r]);
        pause(0.01);
    end
end