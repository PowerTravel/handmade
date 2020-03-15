function [N,Center] =  get_normals(Vert, Face)

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