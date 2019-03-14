function [k, reduced ] = cut_globule( chain, span )
%CUT_GLOBULE Summary of this function goes here
%   Detailed explanation goes here
N = size(chain,1);
j = 1;
k = 1;
partial_chain = cell(N,1);
inside = false;
entering = false;
exiting  = false;

for i=1:N
    
    if  chain(i,1) >= span(1) && ...
        chain(i,1) <= span(2) && ...
        chain(i,2) >= span(3) && ...
        chain(i,2) <= span(4) && ...
        chain(i,3) >= span(5) && ...
        chain(i,3) <= span(6)
        
        if ~inside
            entering  = true;
            inside = true;
        end
        
    else
        if inside
            exiting = true; 
            inside = false; 
        end
    end
        
    if entering
        partial = zeros(N,3);
        j=1;
        entering = false;
    end
    
    if inside
        partial(j,:) = chain(i,:);
        j=j+1;
    end
    
    if exiting 
        partial(j:end,:) = [];
        partial_chain{k} = partial;
        k=k+1;
        exiting = false;
    end
    
end

k=k-1;
reduced = cell(k,1);
for i = 1:k
    reduced{i} = partial_chain{i};
end

end

