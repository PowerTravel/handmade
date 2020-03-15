clear all
%system('../_build/dna');

%data = load('../data/Visualize_Default.dna');
globule = load('Distance/debug/chain');
N = size(globule,1);

traj = load('Distance/debug/trajectory');
plot_traj = true;

plot_steps = 10;
animate = false;

lim = floor(N^(1/3));
dt = 0.01;
dn = floor(N/plot_steps);

intN = 4;
s = intN*N;
glob = zeros(s,3);

%color_grad = [0,0,1
%              1,0,1
%              1,0,0;
%              1,1,0;
%              0,1,0;
%              0,1,1;
%              0,0,1];
color_grad = [0,0,1
              0,1,0
              1,0,0];
              ,0,1];col = zeros(N,3);
col_size = size(color_grad,1)-1;
for i = 1:col_size
    d_size = floor(N/col_size);
    col_start = (i-1)*d_size+1; 
    col_end =  i*d_size ;
    col_interpolation = interpolate_l(color_grad( i, : ),color_grad( i+1, : ), d_size-1, 0.5);
    col(col_start:col_end , : ) = col_interpolation;
end

for i = 1:N-1
    st = globule(i,:);
    ed = globule(i+1,:);
    tmp = interpolate_l(st, ed, intN,1);
    
    startind = (i-1)*intN+1;
    endind = i*intN;
%    
    plot3(tmp(:,1),tmp(:,2),tmp(:,3), 'color', col(i,:),  'LineWidth',10);
    hold on
    plot3(globule(i,1),globule(i,2),globule(i,3), '.','color', col(i,:),  'MarkerSize',60);
    glob(startind :endind , :) = tmp(1:intN,:);
end
plot3(globule(end,1),globule(end,2),globule(end,3), '.','color', col(end,:),  'MarkerSize',60);

axis equal
%sp=3.025;
sp=10;
spans = [-sp, sp, -sp, sp, -sp, sp];
spans = [-11.5, 0.5, 22.5, 37.5, 12.5, 23.5];
tic
[Nc , reduced_globule] = cut_globule(globule,spans);
toc

xlabel('X')
ylabel('Y')
zlabel('Z')
    
    
% figure(2)
% for k = 1:Nc     
%    p_gl = reduced_globule{k};
%    N2 = size(p_gl,1);
%    hold on
%    plot3(p_gl(:,1),p_gl(:,2), p_gl(:,3),'.-','LineWidth',3);
%    hold off
% end
%    
%     axis(spans) 
%     
%     
%     xlabel('X')
%     ylabel('Y')
%     zlabel('Z')
% if plot_traj
%     axis equal;
% hold on
% plot3(traj(:,1),traj(:,2),traj(:,3),'.');
% hold off
% end

