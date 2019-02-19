
A = importdata('../data/Verify_Default.dna');

N  = size(A(:,1),1);

nm = A(:,22);
RvarMean = A(:,23);
R = A(:,2);
R_var = A(:,3);

R_theo = A(:,4);
%errorbar(R, sqrt(R_var), '.k')
plot(log(nm), log(R))
hold on
plot(log(nm), log(R+sqrt(R_var./100)),'.')
plot(log(nm), log(R-sqrt(R_var./100)),'.')
plot(log(n), log(R_theo), 'r');

