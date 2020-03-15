function data = read_debug_data()
particle_file = fopen('particle_info');

n = 1;
i = 1;
j = 1;
data = {};
cg_interval = [19,19];
while ~feof(particle_file)
    line  = fgets(particle_file);
    %% Headers and special values
    if n== 1
       data{1,1} = 'Particle';
       j=2;
    elseif n== 6
       data{2,1} = 'collision struct';
    elseif n== 7
       data{2,2} = str2num(line);
    elseif n== 8
       data{2,3} = str2num(line);
    elseif n== 9
       data{3,1} = 'Derived Values';
    elseif n== 10
       data{3,2} = 'Collision Normal';
    elseif n== 11
       data{3,3} = str2num(line);
    elseif n== 12
        data{3,4} = 'Contact point';
    elseif n== 13
        data{3,5} = str2num(line);
    elseif n== 14
       data{3,6} = 'Penetration Depth';
    elseif n== 15
       data{3,7} = str2num(line);
    elseif n== 16
       data{3,8} = 'Collision Time';
    elseif n== 17
       data{3,9} = str2num(line);
    elseif n == 18
       data{4,1} =  'Collision Bodies';
    elseif n == 19
       data{4,2} = str2num(line);
       cg_interval = [19,20+data{4,2}(1)];
       j=3;
    end
        %% Read particle info
    if n>1 && n<6
        data{1,j} = str2num(line);
        j = j+1;
    end
    
    %% Read collision geometry info
    if (n>cg_interval(1)) && (n < cg_interval(2))
        data{4,j} = str2num(line);
        j = j+1;
    end
    
    n = n+1;
end

end