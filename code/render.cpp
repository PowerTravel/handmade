
#include "platform.h"
#include "math/affine_transformations.h"

internal void
DrawRectangle( bitmap* Buffer, r32 RealMinX, r32 RealMinY, r32 Width, r32 Height, r32 R, r32 G, r32 B )
{
	s32 MinX = RoundReal32ToUInt32( RealMinX );
	s32 MinY = RoundReal32ToUInt32( RealMinY );
	s32 MaxX = RoundReal32ToUInt32( RealMinX + Width);
	s32 MaxY = RoundReal32ToUInt32( RealMinY + Height);

	if( MinX < 0 )
	{
		MinX = 0;
	}
	if( MinY < 0 )
	{
		MinY = 0;
	}
	if( MaxX > (s32) Buffer->Width )
	{
		MaxX = Buffer->Width;
	}
	if( MaxY > (s32) Buffer->Height )
	{
		MaxY = Buffer->Height;
	}

	// Note (Jakob) Move this to bitmap?
	u32 BytesPerPixel = 4;
	u32 Pitch =  Buffer->Width * BytesPerPixel;

	u8* Row = ( (u8*) Buffer->Pixels + MinX*BytesPerPixel + MinY*Pitch );
	for( u32 Y = MinY; Y < (u32) MaxY; ++Y )
	{
		u32* Pixel = (u32*) Row;
		for( u32 X = MinX; X < (u32) MaxX; ++X )
		{
			*Pixel++ = ( (TruncateReal32ToInt32( R*255.f ) << 16 ) |
						 (TruncateReal32ToInt32( G*255.f ) << 8  ) |
						 (TruncateReal32ToInt32( B*255.f ) << 0  ) );
		}
		Row += Pitch;
	}
}

void PutPixel( bitmap* Buffer, u32 X, u32 Y, r32 R, r32 G, r32 B )
{
	if( ( X < 0 ) || ( Y < 0 ) || ( X >= Buffer->Width ) || ( Y >= Buffer->Height ) )
	{
		return;
	} 

	u32 BytesPerPixel = 4;
	u32 Pitch =  Buffer->Width * BytesPerPixel;

	u8*  PixelLocation = ( (u8*) Buffer->Pixels + X*BytesPerPixel + Y*Pitch );
	u32* Pixel = (u32*) PixelLocation;
	*Pixel = ( (TruncateReal32ToInt32( R*255.f ) << 16 ) |
			   (TruncateReal32ToInt32( G*255.f ) << 8  ) |
			   (TruncateReal32ToInt32( B*255.f ) << 0));
}


void DrawCircle( bitmap* Buffer, r32 RealX0, r32 RealY0, r32 RealRadius, r32 R = 1, r32 G = 1, r32 B = 1 )
{
	s32 x0 = RoundReal32ToInt32( RealX0 );
	s32 y0 = RoundReal32ToInt32( RealY0 );
	s32 r  = RoundReal32ToInt32( RealRadius );

	s32 x   = r - 1;
	s32 y   = 0;
	s32 dx  = 1;
	s32 dy  = 1;
	s32 err = dx - ( r << 1 );


	while( x >= y )
	{
		PutPixel( Buffer, x0 + x, y0 + y, R, G, B );
		PutPixel( Buffer, x0 + y, y0 + x, R, G, B );
		PutPixel( Buffer, x0 - y, y0 + x, R, G, B );
		PutPixel( Buffer, x0 - x, y0 + y, R, G, B );
		PutPixel( Buffer, x0 - x, y0 - y, R, G, B );
		PutPixel( Buffer, x0 - y, y0 - x, R, G, B );
		PutPixel( Buffer, x0 + y, y0 - x, R, G, B );
		PutPixel( Buffer, x0 + x, y0 - y, R, G, B );

		//for(int xi = x-1; xi>=0; --xi)
		//{
		//	PutPixel(Buffer, x0 + xi, y0 + y,  1,1,1);
		//	PutPixel(Buffer, x0 + y,  y0 + xi, 1,1,1);
		//	PutPixel(Buffer, x0 - y,  y0 + xi, 1,1,1);
		//	PutPixel(Buffer, x0 - xi, y0 + y,  1,1,1);
		//	PutPixel(Buffer, x0 - xi, y0 - y,  1,1,1);
		//	PutPixel(Buffer, x0 - y,  y0 - xi, 1,1,1);
		//	PutPixel(Buffer, x0 + y,  y0 - xi, 1,1,1);
		//	PutPixel(Buffer, x0 + xi, y0 - y,  1,1,1);
		//}

		if( err <= 0 )
		{
			y++;
			err += dy;
			dy += 2;
		}

		if( err > 0 )
		{
			x--;
			dx += 2;
			err += dx - ( r << 1 );
		}
	}

}

internal void
BlitBMP( bitmap* Buffer, r32 RealMinX, r32 RealMinY, bitmap BitMap )
{
	s32 MinX =(u32) RoundReal32ToInt32( RealMinX );
	s32 MinY =(u32) RoundReal32ToInt32( RealMinY );
	s32 MaxX =(u32) RoundReal32ToInt32( RealMinX + (r32) BitMap.Width  );
	s32 MaxY =(u32) RoundReal32ToInt32( RealMinY + (r32) BitMap.Height );


	u32 ClippingOffsetX = 0;
	u32 ClippingOffsetY = 0;
	if( MinX < 0 )
	{
		ClippingOffsetX = -MinX;
		MinX = 0;
	}
	if( MinY < 0 )
	{
		ClippingOffsetY = -MinY;
		MinY = 0;
	}
	if( (u32) MaxX > Buffer->Width )
	{
		MaxX = Buffer->Width;
	}
	if( (u32) MaxY > Buffer->Height )
	{
		MaxY = Buffer->Height;
	}


	u32 BytesPerPixel = 4;
	u32 BufferPitch =  Buffer->Width * BytesPerPixel;

	u8* SourceRow = (u8*) BitMap.Pixels + ( BitMap.Width*ClippingOffsetY + ClippingOffsetX )*BytesPerPixel;
	u8* DestinationRow = (u8*) ( (u8*) Buffer->Pixels + MinY*BufferPitch + MinX*BytesPerPixel );


	u32 BitmapPitch = BitMap.Width*BytesPerPixel;

	for( s32 Y = MinY; Y < MaxY; ++Y )
	{
		u32* SourcePixel = (u32*)SourceRow;
		u32* DesinationPixel = (u32*)DestinationRow;
		for( s32 X = MinX; X < MaxX; ++X )
		{

			/// Note(Jakob): This loop is SLOW!! Tanking the fps

			u32 Source  = *SourcePixel;

			r32 Alpha   = (r32)  ( (Source & 0xFF000000)  >> 24 )  / 255.f;

			r32 Red     = (r32) (( (Source & 0x00FF0000)  >> 16 )) / 255.f;
			r32 Green   = (r32) (( (Source & 0x0000FF00)  >> 8  )) / 255.f;
			r32 Blue    = (r32) (( (Source & 0x000000FF)  >> 0  )) / 255.f;

			u32 DestPix = *DesinationPixel;
			r32 DRed    = (r32) (( (DestPix & 0x00FF0000) >> 16 )) / 255.f;
			r32 DGreen  = (r32) (( (DestPix & 0x0000FF00) >> 8  )) / 255.f;
			r32 DBlue   = (r32) (( (DestPix & 0x000000FF) >> 0  )) / 255.f;

			Red   = (1 - Alpha)*DRed   + Alpha*Red;
			Green = (1 - Alpha)*DGreen + Alpha*Green;
			Blue  = (1 - Alpha)*DBlue  + Alpha*Blue;

			s32 R = (s32)(Red   * 255 + 0.5);
			s32 G = (s32)(Green * 255 + 0.5);
			s32 B = (s32)(Blue  * 255 + 0.5);

			Source = (R << 16) | (G << 8) | (B << 0);

			*DesinationPixel = Source;

			++DesinationPixel;
			++SourcePixel;

		}

		DestinationRow += BufferPitch;
		SourceRow += BitmapPitch;

	}
}

void DrawLineBresLow( bitmap* Buffer, s32 x0, s32 y0, s32 x1, s32 y1, v4 Color )
{
	s32 dx = x1 - x0;
	s32 dy = y1 - y0;
	s32 yi = 1;
	if (dy < 0)
	{
		yi = -1;
		dy = -dy;
	}
	s32 D = 2 * dy - dx;
	s32 y = y0;

	for (s32 x = x0; x <= x1; ++x)
	{
		PutPixel( Buffer, x, y, Color.X, Color.Y, Color.Z );
		if (D > 0)
		{
			y = y + yi;
			D = D - 2 * dx;
		}
		D = D + 2 * dy;
	}
}


void DrawLineBresHigh( bitmap* Buffer, s32 x0, s32 y0, s32 x1, s32 y1, v4 Color )
{
	s32 dx = x1 - x0;
	s32 dy = y1 - y0;
	s32 xi = 1;
	if (dx < 0)
	{
		xi = -1;
		dx = -dx;
	}
	s32 D = 2 * dx - dy;
	s32 x = x0;
	for (s32 y = y0; y <= y1; ++y)
	{
		PutPixel( Buffer, x, y, Color.X, Color.Y, Color.Z );
		if (D > 0)
		{
			x = x + xi;
			D = D - 2 * dy;
		}

		D = D + 2 * dx;
	}
}

void DrawLineBres( bitmap* Buffer, s32 x0, s32 y0, s32 x1, s32 y1, v4 Color )
{

	if( Abs((r32) (y1 - y0)) < Abs((r32) (x1 - x0)) )
	{
		if(x0 > x1) 
		{
			DrawLineBresLow(Buffer, x1, y1, x0, y0, Color );
		}else{
			DrawLineBresLow(Buffer, x0, y0, x1, y1, Color );
		}
	}else{
		if(y0 > y1) 
		{
			DrawLineBresHigh(Buffer, x1, y1, x0, y0, Color );
		}else{
			DrawLineBresHigh(Buffer, x0, y0, x1, y1, Color );
		}
	}
}

m4 GetRasterMatrix( r32 ScreenWidth, r32 ScreenHeight )
{
	m4 Result = M4( ScreenWidth/2.f,                0, 0,  ScreenWidth/2.f, 
				    			  0, ScreenHeight/2.f, 0, ScreenHeight/2.f, 
				    			  0,                0, 1,                0,
				    			  0,                0, 0,                1);
	return Result;
}

struct calculated_color
{
	v4 ambient;
	v4 diffuse;
	v4 specular;
};
// Inputs are in world coordinate space
calculated_color CalculateColor( v4 Vertice, v4 VerticeNormal, v4 CameraPosition, v4 LightPosition, v4 AmbientProduct, v4 DiffuseProduct, v4 SpecularProduct, r32 Shininess )
{
	v4 N = Normalize(VerticeNormal);
	v4 L = Normalize(LightPosition  - Vertice);
	v4 V = Normalize(CameraPosition - Vertice);
	v4 H = Normalize(L+V);

	Shininess = 10;

	calculated_color Result = {};
	Result.ambient = AmbientProduct;

	r32 Kd = Maximum(L*N, 0.0f);
	Result.diffuse = Kd*DiffuseProduct;

	r32 Ks = Pow(Maximum(H*N, 0.0f), Shininess);
	Result.specular = Ks * SpecularProduct;

	return Result;
}

struct aabb2d
{
	v2 min;
	v2 max;
};

aabb2d getBoundingBox(v2 a, v2 b, v2 c)
{
	v2 p[3] = {a,b,c};
	aabb2d Result = {};
	Result.min.X = Minimum(a.X, Minimum(b.X,c.X));
	Result.min.Y = Minimum(a.Y, Minimum(b.Y,c.Y));
	Result.max.X = Maximum(a.X, Maximum(b.X,c.X));
	Result.max.Y = Maximum(a.Y, Maximum(b.Y,c.Y));
	return Result;
}

r32 EdgeFunction( v2* a, v2* b, v2* p )
{
	r32 Result = (a->X-p->X) * (b->Y-a->Y) - (b->X-a->X) * (a->Y-p->Y);
	return(Result);
}


struct fragment_color
{
	v4 LightPosition;
	v4 AmbientColor;
	v4 DiffuseColor;
	v4 SpecularColor;
	r32 Shininess;
};


void DrawTriangle(  bitmap* OffscreenBuffer, bitmap* DepthBuffer, 
					aabb2d* RasterBB, v2* PointsInScreenSpace, 
					v4* PointInCameraSpace, v2* TextureCoordinates, bitmap* TextureMap, r32 PremulArea2, calculated_color* Color)
{
	u32 BytesPerPixel = 4;
	u32 Pitch =  OffscreenBuffer->Width * BytesPerPixel;
	r32 near_clipping_plane = -1;
	for(s32 Y = RoundReal32ToInt32( RasterBB->min.Y ); Y < RasterBB->max.Y; ++Y)
	{
		for(s32 X = RoundReal32ToInt32( RasterBB->min.X ); X < RasterBB->max.X; ++X)
		{
			v2 p = V2( (r32) X, (r32) Y);
			r32 PixelInRange0 = EdgeFunction( &PointsInScreenSpace[0], &PointsInScreenSpace[1], &p);
			r32 PixelInRange1 = EdgeFunction( &PointsInScreenSpace[1], &PointsInScreenSpace[2], &p);
			r32 PixelInRange2 = EdgeFunction( &PointsInScreenSpace[2], &PointsInScreenSpace[0], &p);

			b32 ShouldFillPixel = ( ( PixelInRange0 >= 0 ) && 
								    ( PixelInRange1 >= 0 ) &&
								    ( PixelInRange2 >= 0 ) );

			if( ShouldFillPixel )
			{
				// Barycentric Coordinates
				r32 Lambda0 = PixelInRange1 * PremulArea2;
				r32 Lambda1 = PixelInRange2 * PremulArea2;
				r32 Lambda2 = PixelInRange0 * PremulArea2;

				r32 PixelDepthValue = Lambda0 * PointInCameraSpace[0].Z + Lambda1 * PointInCameraSpace[1].Z + Lambda2 * PointInCameraSpace[2].Z;

				r32* DepthBufferPixels = (r32*) DepthBuffer->Pixels;
				r32* BufferDepthValue =  &DepthBufferPixels[ Y * DepthBuffer->Width  + X];

				if( ( PixelDepthValue > *BufferDepthValue) && ( PixelDepthValue < near_clipping_plane ) )
				{
					*BufferDepthValue = PixelDepthValue;
					u8* PixelLocation = ((u8*)OffscreenBuffer->Pixels + X * BytesPerPixel +
										Y * Pitch);

					v4 InterpolatedAmbientColor  = Lambda0 * Color[0].ambient  + Lambda1 * Color[1].ambient  + Lambda2 * Color[2].ambient;
					v4 InterpolatedDiffuseColor  = Lambda0 * Color[0].diffuse  + Lambda1 * Color[1].diffuse  + Lambda2 * Color[2].diffuse;
					v4 InterpolatedSpecularColor = Lambda0 * Color[0].specular + Lambda1 * Color[1].specular + Lambda2 * Color[2].specular;

					v2 InterpolatedTextureCoord = Lambda0 * TextureCoordinates[0] + Lambda1 * TextureCoordinates[1] + Lambda2 * TextureCoordinates[2];

					r32 TextureRed   = 1;
					r32 TextureGreen = 1;
					r32 TextureBlue  = 1;
					if( TextureMap  )
					{
						u32* TextureValue = ( (u32*) TextureMap->Pixels + TextureMap->Width *  TruncateReal32ToInt32( InterpolatedTextureCoord.Y ) +
																							   TruncateReal32ToInt32( InterpolatedTextureCoord.X ));

						TextureRed   = ( ((*TextureValue) & 0x00FF0000) >> 16)/255.f;
						TextureGreen = ( ((*TextureValue) & 0x0000FF00) >> 8)/255.f;
						TextureBlue  = ( ((*TextureValue) & 0x000000FF) >> 0)/255.f;

					}
#if 1
					r32 TextureBlendedRed   = InterpolatedAmbientColor.X * TextureRed + 
											  InterpolatedDiffuseColor.X * TextureRed + 
											  InterpolatedSpecularColor.X;
					TextureBlendedRed = (TextureBlendedRed > 1) ? 1 : TextureBlendedRed;	 

					r32 TextureBlendedGreen = InterpolatedAmbientColor.Y * TextureGreen + 
											  InterpolatedDiffuseColor.Y * TextureGreen + 
											  InterpolatedSpecularColor.Y;
					TextureBlendedGreen = (TextureBlendedGreen > 1) ? 1 : TextureBlendedGreen;

					r32 TextureBlendedBlue  = InterpolatedAmbientColor.Z * TextureBlue + 
											  InterpolatedDiffuseColor.Z * TextureBlue + 
											  InterpolatedSpecularColor.Z;
					TextureBlendedBlue = (TextureBlendedBlue > 1) ? 1 : TextureBlendedBlue;	 

#else
					r32 TextureBlendedRed   =  TextureRed;
					r32 TextureBlendedGreen =  TextureGreen;
					r32 TextureBlendedBlue  =  TextureBlue;
#endif
					u32* Pixel = (u32*)PixelLocation;

					*Pixel = ((TruncateReal32ToInt32(TextureBlendedRed  * 255.f) << 16) |
							  (TruncateReal32ToInt32(TextureBlendedGreen* 255.f) << 8)  |
							  (TruncateReal32ToInt32(TextureBlendedBlue * 255.f) << 0));
				}
			}
		}
	}
}

void GouradShading(bitmap* OffscreenBuffer, bitmap* DepthBuffer, 
					v4* CameraPosition, aabb2d* RasterBB, v2* PointsInScreenSpace, 
					v4* Vertices, v4* VertexNormals, v4* PointInCameraSpace, 
					u32 NrFragmentColors, fragment_color* PerLightColors, v2* TextureCoordinates, bitmap* TextureMap )
{
	r32 Area2 = EdgeFunction( &PointsInScreenSpace[0], &PointsInScreenSpace[1], &PointsInScreenSpace[2] );
	if( Area2 <= 0 )
	{
		return;
	}
	
	
	r32 PremulArea2 = 1/Area2;
	calculated_color Color[3] = {};
	for( u32 LightIndex = 0; LightIndex < NrFragmentColors; ++LightIndex )
	{
		for( u32 TriangleIndex = 0; TriangleIndex < 3; ++TriangleIndex )
		{
			fragment_color& Fragment = PerLightColors[LightIndex];
			
			calculated_color VC = CalculateColor( Vertices[TriangleIndex], 
												  VertexNormals[TriangleIndex], 
												  *CameraPosition, 
												  Fragment.LightPosition, 
									     		  Fragment.AmbientColor, 
									     		  Fragment.DiffuseColor, 
									     		  Fragment.SpecularColor, 
									     		  Fragment.Shininess );

			calculated_color& VerticeColor = Color[TriangleIndex];
			VerticeColor.ambient += VC.ambient;
			VerticeColor.diffuse += VC.diffuse;
			VerticeColor.specular += VC.specular;
		}
	}
	
	DrawTriangle( OffscreenBuffer, DepthBuffer, RasterBB, PointsInScreenSpace, 
				  PointInCameraSpace, TextureCoordinates, TextureMap, PremulArea2, Color);

}

v4 Blend(v4* A, v4* B)
{
	v4 Result =  V4( A->X *B->X,  
				     A->Y *B->Y,  
				     A->Z *B->Z,  
				     A->W *B->W );
	return Result;
}

void ClampToOne( v4* A )
{
	A->X = (A->X > 1) ? 1 : A->X; 
	A->Y = (A->Y > 1) ? 1 : A->Y; 
	A->Z = (A->Z > 1) ? 1 : A->Z; 
	A->W = (A->W > 1) ? 1 : A->W; 
}

void DrawTriangles( game_render_commands* RenderCommands, bitmap* OutputBitMap  )
{
#if 1
	render_push_buffer* PushBuffer = (render_push_buffer*)  RenderCommands->PushBuffer;

	// Get camera matrices
	m4& V =  PushBuffer->Camera->V; 	// ViewMatrix
	m4& P =  PushBuffer->Camera->P;		// ProjectionMatrix
	m4 R =  GetRasterMatrix( (r32) OutputBitMap->Width, (r32) OutputBitMap->Height);		// RasterizationMatrix
	m4 	RPV= R*P*V;						// All in one

	v4 CameraPosition = Transpose( RigidInverse( V ) ).r3;

	// 'Reset' DepthBuffer
	depth_buffer* DepthBuffer = PushBuffer->DepthBuffer;
	for( u32 i = 0; i<DepthBuffer->Width*DepthBuffer->Height; ++i)
	{
		DepthBuffer->Buffer[i] = -10E10;
	}

	r32 DiffsePremul = 1/Pi32;
	r32 SpecularPremul = 1/(8*Pi32);

	// For each render group
	filo_queue<render_group*>& RenderGroups = PushBuffer->RenderGroups;
	while( !PushBuffer->RenderGroups.IsEmpty()  )
	{
		render_group* RenderGroup = RenderGroups.Pop();

		temporary_memory LightMemory = BeginTemporaryMemory( PushBuffer->Arena );
		fragment_color* PerLightColors = (fragment_color*) PushArray( PushBuffer->Arena, PushBuffer->Lights.GetSize(), fragment_color );
	
		surface_property SurfaceProperty = RenderGroup->Mesh->SurfaceProperty;
		material* Material = SurfaceProperty.Material;
		bitmap* DiffuseMap = SurfaceProperty.DiffuseMap;
		
		list<component_light*>& LightQueue = PushBuffer->Lights;
		LightQueue.First();
		u32 LightIndex = 0;
		while( ! LightQueue.IsEnd() )
		{
			component_light* Light = LightQueue.Get();

			fragment_color& Fragment = PerLightColors[LightIndex];
			Fragment.LightPosition  = Light->Position;

			v4 LightColor  = Light->Color;
			
			if(Material)
			{
				Fragment.AmbientColor   = Blend(&Light->Color, &Material->AmbientColor);
				Fragment.DiffuseColor   = Blend(&Light->Color, &Material->DiffuseColor);
				Fragment.SpecularColor  = Blend(&Light->Color, &Material->SpecularColor);
				Fragment.Shininess 		= Material->Shininess;
			}else{

				Fragment.AmbientColor  = V4(3,3,3,3);
				Fragment.DiffuseColor  = V4(0,0,0,0);
				Fragment.SpecularColor = V4(0,0,0,0);

				Fragment.Shininess = 1;
			}

			Fragment.AmbientColor  = DiffsePremul * Fragment.AmbientColor;
			Fragment.SpecularColor = (SpecularPremul * (Fragment.Shininess + 8) ) * Fragment.SpecularColor;

			LightQueue.Next();
			LightIndex++;
		};

		// Transform Matrix for points
		m4& T = RenderGroup->Mesh->T;
		// Transform Matrix for normals
		m4 NT = Transpose( RigidInverse(T) );

		component_render_mesh*  Mesh = RenderGroup->Mesh;
		u32 NrTrianglesInMesh = Mesh->TriangleCount;
		mesh_data* MeshData   = Mesh->Data;

		for( u32 TriangleIndex = 0; TriangleIndex < NrTrianglesInMesh; ++TriangleIndex) 
		{
			face* Triangle = &Mesh->Triangles[TriangleIndex];
			Assert(Triangle->nv == 3);

			v4 v[3]  = {};
			v4 vn[3] = {};
			v2 tc[3] = {}; // TextureCoordinate

			v[0] = T*MeshData->v[ Triangle->vi[0] ];
			v[1] = T*MeshData->v[ Triangle->vi[1] ];
			v[2] = T*MeshData->v[ Triangle->vi[2] ];

			if(Triangle->ni)
			{
				vn[0] = NT*MeshData->vn[ Triangle->ni[0] ];
				vn[1] = NT*MeshData->vn[ Triangle->ni[1] ];
				vn[2] = NT*MeshData->vn[ Triangle->ni[2] ];
			}else{
				v3 A = V3(v[0]-v[1]);
				v3 B = V3(v[0]-v[2]);
				v4 FaceNormal =  V4( Normalize( CrossProduct(A,B) ),0);

				vn[0] = vn[1] = vn[2] = NT*FaceNormal;
			}

			if( Triangle->ti && DiffuseMap )
			{
				tc[0].X = DiffuseMap->Width  * MeshData->vt[ Triangle->ti[0] ].X;
				tc[0].Y = DiffuseMap->Height * MeshData->vt[ Triangle->ti[0] ].Y;
				tc[1].X = DiffuseMap->Width  * MeshData->vt[ Triangle->ti[1] ].X;
				tc[1].Y = DiffuseMap->Height * MeshData->vt[ Triangle->ti[1] ].Y;
				tc[2].X = DiffuseMap->Width  * MeshData->vt[ Triangle->ti[2] ].X;
				tc[2].Y = DiffuseMap->Height * MeshData->vt[ Triangle->ti[2] ].Y;
			}

			// Get Bounding Box in raster space
			v2 PointsInScreenSpace[3] = { V2( PointMultiply(RPV, v[0]) ), V2( PointMultiply(RPV, v[1]) ), V2( PointMultiply(RPV, v[2]) ) };
			aabb2d ScreenBoundingBox = getBoundingBox( PointsInScreenSpace[0], PointsInScreenSpace[1], PointsInScreenSpace[2] );

			// OffScreenCulling
			if( (ScreenBoundingBox.max.X < 0)  || 
				(ScreenBoundingBox.max.Y < 0 ) ||
				(ScreenBoundingBox.min.X >= OutputBitMap->Width) || 
				(ScreenBoundingBox.min.Y >= OutputBitMap->Height))
			{
				continue;
			}

			ScreenBoundingBox.min.X = Maximum(0, ScreenBoundingBox.min.X);
			ScreenBoundingBox.max.X = Minimum((r32) OutputBitMap->Width, ScreenBoundingBox.max.X);
			ScreenBoundingBox.min.Y = Maximum(0, ScreenBoundingBox.min.Y);
			ScreenBoundingBox.max.Y = Minimum((r32) OutputBitMap->Height, ScreenBoundingBox.max.Y);

			v4 PointInCameraSpace[3] ={ V*v[0] ,V*v[1] , V*v[2] };

			GouradShading(OutputBitMap, DepthBuffer, &CameraPosition, &ScreenBoundingBox, PointsInScreenSpace, v, vn, PointInCameraSpace,  PushBuffer->Lights.GetSize(), PerLightColors, tc, DiffuseMap);

		}
		EndTemporaryMemory( LightMemory );
	}
	#endif
}
