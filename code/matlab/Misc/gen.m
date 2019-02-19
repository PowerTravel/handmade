function [chain] = gen(N)

%N = 10; % storleken av en kvadrant

% Nummret i cellen gor fron 0 till 6
% 0 = unoccupied
% 1 = kom fron x i positiv riktning
% 2 = kom fron x i negativ riktning
% 3 = kom fron y i positiv riktning
% 4 = kom fron y i negativ riktning
% 5 = kom fron z i positiv riktning
% 6 = kom fron z i negativ riktning

% cell (size+1,size+1,size+1) er startpositionen men er alltid 1

global frac 
frac = zeros(2*N,2*N,2*N);
Oc = {N+1,N+1,N+1}; % Origin as cell for easy indexing
Od = [N+1,N+1,N+1]; % Origin as matrix for easy mathing
frac(Oc{:}) = 1;

i=0;
j=0;
k=0;
link = 1;
chain = [1,1,1];
while chain(link,1)+2<N && chain(link,1)-2 > -N && ...
        chain(link,2)+2<N && chain(link,2)-2 > -N && ...
        chain(link,3)+2<N && chain(link,3)-2 > -N
    
    % Generate probability for all neighbouring sites
    [n, idx] = getProb(chain(link,1)+N,chain(link,2)+N,chain(link,3)+N);
    frac(idx(1),idx(2),idx(3)) = n;
    
    chain(link+1, : ) = idx-N;

    %%
	%center = [0,0,0];
	%hold on
    %axis([-N+center(1), N+center(1), -N+center(2), N+center(2), -N+center(3), N+center(3)])
    %plot3(chain(:,1),chain(:,2),chain(:,3))
    %plot3(chain(:,1),chain(:,2),chain(:,3),'.')
    %view(3)
    
    %%
    
    %pause(0.2)
   % waitforbuttonpress
    link = link+1;
    
    
    
	%break;
end

if false
center = [0,0,0];
%plot3(x(:,1),x(:,2),x(:,3));
hold on
axis([-N+center(1), N+center(1), -N+center(2), N+center(2), -N+center(3), N+center(3)])
plot3(chain(:,1),chain(:,2),chain(:,3))
plot3(chain(:,1),chain(:,2),chain(:,3),'.')
view(3)
end
end    