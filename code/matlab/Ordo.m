% 
% Data = [ 1000	0.0027
% 2000	0.0085
% 4000	0.0307
% 8000	0.143
% 10000	0.2303
% 16000	0.6095
% 20000	0.9641
% 30000	2.2172
% 32000	2.5193
% 40000	4.0361
% 50000	6.3049
% 60000	9.0299
% 64000	10.2922
% 70000	12.3231
% 80000	16.1143
% 90000	20.4949
% 100000	25.3859
% 110000	30.4015
% 128000	41.1191
% 200000,  4*25.3859];
% 
% 
% g = @(n) n.*n;
% 
% n = Data(:,1);
% f = Data(:,2);
% gn = g(n);
% 
% fg_ratio = f./gn;
% 
% 
% 
% figure(1)
% plot(n,fg_ratio,'.-r', 'markersize',20)
% figure(2)
% plot(n,f,'.-r', 'markersize',20)
% hold on
% plot(n,g(n), '.-b', 'markersize',20)
% xlabel('n')
% ylabel('t');
% 
% hold off


f = @(N) 0.5.*N .*(N+1);
g = @(N) N.*log(N);
g = @(N) 0.5*N.*N;
N = 1:100;
plot(N, f(N)./g(N));

j = 0;
N= 10000;
for i = 0:N
    j = j+i;
end

j
T = (N*(N+1))/2

j - T