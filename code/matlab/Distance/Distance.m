hold off
clear all
DData = load('distance.dna');

N = size(DData,1);

time = DData(1:N,1);
lgt = log(time);

D = DData(1:N,2);
D_v = DData(1:N,3);
P = DData(1:N,4:6);
P_v = DData(1:N,7:9);

f = fittype('a*x+b');
fitD_m = fit(lgt,log( D ),f,'StartPoint',[1 1]);

figure(1);
plot(lgt, log(D), '-k.', lgt ,log(D+sqrt(D_v)), ...
            '.k', lgt, log(D-sqrt(D_v)), '.k', 'linewidth',2);
   
title('loglog of diffusion distance vs time');
xlabel('Log of time');
ylabel('Log diffusion distance');
legend({'Data','Standard Deviation'});
annotation('textbox', [.2 .8 .1 .1], 'String', ...
                     ['Measured slope: ', num2str(fitD_m.a) ]);
                 
figure(2)
plot3(P(:,1),P(:,2),P(:,3))