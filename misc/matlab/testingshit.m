clear all
pd = 0.0698043;
rad = 1.41421;
vel = [0,1,0];
cn =  [0.724999 0.688749       -0];

a = rad;
c = rad - pd;
cosA = (cn * vel')/(norm(rad)*norm(vel));
p = -2*c * cosA;
q = c*c - a*a;

ph = (p/2)

p2h = (p/2)*(p/2);

p2h - q