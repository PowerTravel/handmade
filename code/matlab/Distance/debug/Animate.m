
for i=1:1
%cd '../../../_build'
%system('./dna')
%cd '../matlab/Distance/debug'

data = load('trajectory');
%data = load('../particle_trajectory.dna');

N = size(data,1);
animate = 0;
x = 1:N;

sep_x = 0.9;
sep_y = 2;

%plot_epsiloid([sep_x, -sep_y, -sep_x], [1,1,1]);
%plot_epsiloid([-sep_x, -sep_y, -sep_x], [1,1,1]);
%plot_epsiloid([sep_x, -sep_y, sep_x], [1,1,1]);
%plot_epsiloid([-sep_x, -sep_y, sep_x], [1,1,1]);

%plot_epsiloid([sep_x, sep_y, -sep_x], [1,1,1]);
%plot_epsiloid([-sep_x, sep_y, -sep_x], [1,1,1]);
%plot_epsiloid([sep_x, sep_y, sep_x], [1,1,1]);
%plot_epsiloid([-sep_x, sep_y, sep_x], [1,1,1]);
        

frame_jump = 1;
if(animate==0)
    T = -10:0.1:10;
    plot3(data(:,1),data(:,2),data(:,3),'.');
    hold on
    T = -10:0.1:10;
    xlabel('x')
    ylabel('y')
else
    r=5;
    xlabel('x')
    ylabel('y')
    zlabel('z')
    for i = 1:N
        if(mod(i,1)==0)
            hold on 
            plot3(data(i,1),data(i,2), data(i,3),'.-', 'linewidth', 4);
            axis([-r,r,-r,r,-r,r]);
            pause(0.01);
        end
    end
    hold off
end
pause(0.1)
end
