clear all

MiniY = 5.96046448e-08
Vertices  = [ -1, MiniY,-1;
              -1, MiniY, 0;
               0,     1,-1;
               1, MiniY, 0];
Faces = [1,2,3;
         2,4,3;
         3,4,1;
         4,2,1];

PlotFace(Vertices, Faces, -1);


Vertices  = [ -1, MiniY,-1;
              -1, MiniY, 0;
               0,     1,-1;
               1, MiniY, 0;
               1,     2, 1];

Faces = [2,4,3;
         3,4,1;
         4,2,1];

PlotFace(Vertices, Faces, 5);



Sequence = {[1,2,3,4];
            [2,3,4]};
%PlaySequence(Vertices, Faces, Sequence)
function PlaySequence(Vertices, Faces, Sequence)
  nrFrames = size(Sequence,1);
  Arrow = -1;
  nextArrow = 5;
  for i = 1:nrFrames
    FacesThisFrame = Sequence{i}
    NrFaces = size(FacesThisFrame,2)
    F = Faces(FacesThisFrame,:);
    Arrow
    PlotFace(Vertices, F, Arrow);
    pause
    if(Arrow == -1)
      Arrow = nextArrow;
      nextArrow = nextArrow+1;
    end
  end
end

function [N,Center] = GetNormals(Vert, Face)

  nrFaces = size(Face,1);
  N       = zeros(nrFaces,3);
  Center  = zeros(nrFaces,3);
  for idx = 1:nrFaces
    a = Vert(Face(idx,1),:);
    b = Vert(Face(idx,2),:);
    c = Vert(Face(idx,3),:);
    Center(idx,:) = (a+b+c)./3;
    x = cross(b - a, c - a);

    N(idx,:) = x ./ norm(x);
  end
end

function PlotNormals(Vert, Face)

  nrFaces = size(Face,1);
  [N, Center] = GetNormals(Vert, Face);
  for idx = 1:nrFaces
    p = Center(idx,:);
    n =  N(idx,:)./2;
    quiver3(p(1),p(2),p(3),n(1),n(2),n(3), 'linewidth', 2, 'color', 'red')
  end
end


function PlotFace(Vert, Face, Arrow)
  border = 0.4;

  minVal = min(min(Vert))- border;
  maxVal = max(max(Vert))+ border;

  nrFaces = size(Face,1);
  nrVertices = size(Vert,1);

  Patch.Vertices = Vert;
  Patch.Faces = Face;

  CDData = 0.3 * ones(nrFaces,1);
  Patch.FaceVertexCData = CDData;
  Patch.FaceColor = 'flat';
  Patch.EdgeColor = 'red';

  figure()
  camup([0 1 0])
  grid on
  pbaspect([1 1 1])
  rotate3d on
  axis([minVal, maxVal, minVal, maxVal, minVal, maxVal])
  xlabel('X')
  ylabel('Y')
  zlabel('Z')
  patch(Patch);
  hold on
  PlotNormals(Vert, Face);
  if(Arrow > 0)
    quiver3(0, 0, 0, Vert(Arrow,1),Vert(Arrow,2), Vert(Arrow,3), 'linewidth', 2, 'color', 'green')
  end
  hold off
end

%A.FaceVertexCData = [0.31; 0.32; 0.33; 0.34];
%A.FaceColor = 'flat';
%A.EdgeColor = 'red';
%
%%figure();
%axis([-1.1, 1.1, -0.1, 1, -1, 1.1])
%xlabel('X')
%ylabel('Y')
%zlabel('Z')
%patch(A);
%
%{
%% Removing two edges
%pause
B.Vertices  = [ 0, 0.95,-1;
                1,-0.05, 0;
               -1,-0.05, 0;
               -1, 0.95, 0];
B.Faces = [1,2,3;
           3,4,1];
B.FaceVertexCData = [0.31; 0.32; 0.33; 0.34];
B.FaceColor = 'flat';
B.EdgeColor = 'red';

%figure();
axis([-1.1, 1.1, -0.1, 1, -1, 1.1])
xlabel('X')
ylabel('Y')
zlabel('Z')
patch(B);
hold on
plot3(-1, 0.95, 0, 'o')
hold off

%% Adding Four Faces
%pause
%figure();
axis([-1.1, 1.1, -0.1, 1, -1, 1.1])
xlabel('X')
ylabel('Y')
zlabel('Z')
C.Vertices  = [ 0, 0.95,-1;  % 1
                1,-0.05, 0;  % 2
               -1,-0.05, 0;  % 3
               -1, 0.95, 0;  % 4
                0, 0.95, 1]; % 5
               
C.Faces = [1,2,3;
           3,4,1;
           3,2,5;  % 1
           2,1,5;  % 2
           1,4,5;  % 3
           4,3,5]; % 4
C.FaceVertexCData = [0.31; 0.32; 0.33; 0.34; 0.35];
C.FaceColor = 'flat';
C.EdgeColor = 'red';

patch(C);
hold on
plot3(0, 0.95, 1, 'o')
hold off

%% Remove Four Faces
%pause
%figure();
axis([-1.1, 1.1, -0.1, 1, -1, 1.1])
xlabel('X')
ylabel('Y')
zlabel('Z')
D.Vertices  = [ 0, 0.95,-1;  % 1
                1,-0.05, 0;  % 2
               -1,-0.05, 0;  % 3
               -1, 0.95, 0;  % 4
                0, 0.95, 1;  % 5
               -1,-0.05,-1]; % 6
               
D.Faces = [3,4,1;
           3,2,5;  % 1
           2,1,5;  % 2
           1,4,5;  % 3
           4,3,5]; % 4
D.FaceVertexCData = [0.31; 0.32; 0.33; 0.34; 0.35; 0.36];
D.FaceColor = 'flat';
D.EdgeColor = 'red';

patch(D)
hold on
pl
ot3(-1,-0.05,-1, 'o')



%}