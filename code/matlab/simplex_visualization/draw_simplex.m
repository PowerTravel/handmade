function [] = draw_simplex(Vert, Face, Arrow)

  nrFaces = size(Face,1);
  nrVertices = size(Vert,1);

  Patch.Vertices = Vert;
  Patch.Faces = Face;

  CDData = 0.3 * ones(nrFaces,1);
  Patch.FaceVertexCData = CDData;
  Patch.FaceColor = 'flat';
  Patch.EdgeColor = 'red';
  
  patch(Patch);
end