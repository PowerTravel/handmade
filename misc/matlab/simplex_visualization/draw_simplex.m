function [] = draw_simplex(Vert, Face, faceColor  )

  nrFaces = size(Face,1);
  nrVertices = size(Vert,1);

  Patch.Vertices = Vert;
  Patch.Faces = Face;

  %CDData = 0.3 * ones(nrFaces,1);
  %Patch.FaceVertexCData = CDData;
  Patch.FaceColor = faceColor;
  Patch.EdgeColor = 'black';

  patch(Patch);
end