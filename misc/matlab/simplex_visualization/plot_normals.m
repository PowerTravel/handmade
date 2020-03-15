function plot_normals(Vert, Face)

  nrFaces = size(Face,1);
  [N, Center] = get_normals(Vert, Face);
  for idx = 1:nrFaces
    p = Center(idx,:);
    n =  N(idx,:)./2;
    quiver3(p(1),p(2),p(3),n(1),n(2),n(3), 'linewidth', 2, 'color', 'red')
  end
end
