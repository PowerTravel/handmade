clear all
hold off
t = load("../../../Tree.m");

s = t(:,1);
n = t(:,2:7);

border = 0.4;

minVal = min(min(n))- border;
maxVal = max(max(n))+ border;

camup([0 1 0])
grid on
pbaspect([1 1 1])
rotate3d on
axis([minVal, maxVal, minVal, maxVal, minVal, maxVal])
xlabel('X')
ylabel('Y')
zlabel('Z')

hold on

len = size(t,1);
figure(1)
% parse the file
for i=1:len
  if(s(i) >= 0)
    draw_rect(n(i,1:3), n(i,4:6)-n(i,1:3), 'black', 1);
  else
    pause;
  end
end