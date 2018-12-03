#include "render.h"

internal void
DrawRectangle( game_offscreen_buffer* Buffer, r32 RealMinX, r32 RealMinY, r32 Width, r32 Height, r32 R, r32 G, r32 B )
{
	s32 MinX = RoundReal32ToInt32( RealMinX );
	s32 MinY = RoundReal32ToInt32( RealMinY );
	s32 MaxX = RoundReal32ToInt32( RealMinX + Width);
	s32 MaxY = RoundReal32ToInt32( RealMinY + Height);

	if( MinX < 0 )
	{
		MinX = 0;
	}
	if( MinY < 0 )
	{
		MinY = 0;
	}
	if( MaxX > Buffer->Width )
	{
		MaxX = Buffer->Width;
	}
	if( MaxY > Buffer->Height )
	{
		MaxY = Buffer->Height;
	}

	u8* Row = ( (u8*) Buffer->Memory + MinX*Buffer->BytesPerPixel + MinY*Buffer->Pitch );
	for( s32 Y = MinY; Y < MaxY; ++Y )
	{
		u32* Pixel = (u32*) Row;
		for( s32 X = MinX; X < MaxX; ++X )
		{
			*Pixel++ = ( (TruncateReal32ToInt32( R*255.f ) << 16 ) |
						 (TruncateReal32ToInt32( G*255.f ) << 8  ) |
						 (TruncateReal32ToInt32( B*255.f ) << 0  ) );
		}
		Row += Buffer->Pitch;
	}
}

void PutPixel( game_offscreen_buffer* Buffer, s32 X, s32 Y, r32 R, r32 G, r32 B )
{
	if( ( X < 0 ) || ( Y < 0 ) || ( X >= Buffer->Width ) || ( Y >= Buffer->Height ) )
	{
		return;
	} 

	u8*  PixelLocation = ( (u8*) Buffer->Memory + X*Buffer->BytesPerPixel + Y*Buffer->Pitch );
	u32* Pixel = (u32*) PixelLocation;
	*Pixel = ( (TruncateReal32ToInt32( R*255.f ) << 16 ) |
			   (TruncateReal32ToInt32( G*255.f ) << 8  ) |
			   (TruncateReal32ToInt32( B*255.f ) << 0));
}


void DrawCircle( game_offscreen_buffer* Buffer, r32 RealX0, r32 RealY0, r32 RealRadius, r32 R = 1, r32 G = 1, r32 B = 1 )
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


// makes the packing compact
#pragma pack(push, 1)
struct bmp_header
{
	u16 FileType;     /* File type, always 4D42h ("BM") */
	u32 FileSize;     /* Size of the file in bytes */
	u16 Reserved1;    /* Always 0 */
	u16 Reserved2;    /* Always 0 */
	u32 BitmapOffset; /* Starting position of image data in bytes */

	u32 HeaderSize;       /* Size of this header in bytes */
	s32 Width;           /* Image width in pixels */
	s32 Height;          /* Image height in pixels */
	u16 Planes;          /* Number of color planes */
	u16 BitsPerPixel;    /* Number of bits per pixel */
	u32 Compression;     /* Compression methods used */
	u32 SizeOfBitmap;    /* Size of bitmap in bytes */
	s32 HorzResolution;  /* Horizontal resolution in pixels per meter */
	s32 VertResolution;  /* Vertical resolution in pixels per meter */
	u32 ColorsUsed;      /* Number of colors in the image */
	u32 ColorsImportant; /* Minimum number of important colors */
	/* Fields added for Windows 4.x follow this line */

	u32 RedMask;       /* Mask identifying bits of red component */
	u32 GreenMask;     /* Mask identifying bits of green component */
	u32 BlueMask;      /* Mask identifying bits of blue component */
	u32 AlphaMask;     /* Mask identifying bits of alpha component */
	u32 CSType;        /* Color space type */
	s32 RedX;          /* X coordinate of red endpoint */
	s32 RedY;          /* Y coordinate of red endpoint */
	s32 RedZ;          /* Z coordinate of red endpoint */
	s32 GreenX;        /* X coordinate of green endpoint */
	s32 GreenY;        /* Y coordinate of green endpoint */
	s32 GreenZ;        /* Z coordinate of green endpoint */
	s32 BlueX;         /* X coordinate of blue endpoint */
	s32 BlueY;         /* Y coordinate of blue endpoint */
	s32 BlueZ;         /* Z coordinate of blue endpoint */
	u32 GammaRed;      /* Gamma red coordinate scale value */
	u32 GammaGreen;    /* Gamma green coordinate scale value */
	u32 GammaBlue;     /* Gamma blue coordinate scale value */
};
#pragma pack(pop)

internal void
BlitBMP( game_offscreen_buffer* Buffer, r32 RealMinX, r32 RealMinY, loaded_bitmap BitMap )
{
	s32 MinX = RoundReal32ToInt32( RealMinX );
	s32 MinY = RoundReal32ToInt32( RealMinY );
	s32 MaxX = RoundReal32ToInt32( RealMinX + (r32) BitMap.Width  );
	s32 MaxY = RoundReal32ToInt32( RealMinY + (r32) BitMap.Height );


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
	if( MaxX > Buffer->Width )
	{
		MaxX = Buffer->Width;
	}
	if( MaxY > Buffer->Height )
	{
		MaxY = Buffer->Height;
	}


	u32 BytesPerPixel = 4;

	u8* SourceRow = (u8*) BitMap.Pixels + ( BitMap.Width*ClippingOffsetY + ClippingOffsetX )*BytesPerPixel;
	u8* DestinationRow = (u8*) ( (u8*) Buffer->Memory + MinY*Buffer->Pitch + MinX*Buffer->BytesPerPixel );


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

		DestinationRow += Buffer->Pitch;
		SourceRow += BitmapPitch;

	}
}

void DrawLineBresLow( game_offscreen_buffer* Buffer, s32 x0, s32 y0, s32 x1, s32 y1, v4 Color )
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


void DrawLineBresHigh( game_offscreen_buffer* Buffer, s32 x0, s32 y0, s32 x1, s32 y1, v4 Color )
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

void DrawLineBres( game_offscreen_buffer* Buffer, s32 x0, s32 y0, s32 x1, s32 y1, v4 Color )
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

// Inputs are in world coordinate space
v4 CalculateColor( v4 Vertice, v4 VerticeNormal, v4 CameraPosition, v4 LightPosition, v4 AmbientProduct, v4 DiffuseProduct, v4 SpecularProduct, r32 Shininess )
{
	local_persist r32 t = 0;
	t += 0.000006;
	if(t>=10*Pi32)
	{
		t-= 10*Pi32;
	}

	v4 N = Normalize(VerticeNormal);
	v4 L = Normalize(LightPosition  - Vertice);
	v4 V = Normalize(CameraPosition - Vertice);
	v4 H = Normalize(L+V);

	Shininess = 10;

	v4 ambient = AmbientProduct;

	r32 Kd = Maximum(L*N, 0.0f);
	v4 diffuse = Kd*DiffuseProduct;

	r32 Ks = Pow(Maximum(H*N, 0.0f), Shininess);
	v4 specular = Ks * SpecularProduct;

	v4 Result;
	Result = ambient + diffuse + specular;
	return Result;
}

struct aabb2d
{
	v2 min;
	v2 max;
};


void PushLight( render_push_buffer* PushBuffer, component_light* Light )
{
	Push<component_light>( PushBuffer->Lights, Light );
}

void PushRenderGroup( render_push_buffer* PushBuffer, component_mesh* Mesh, component_material* Material )
{
	render_group* NewRenderGroup = (render_group*) PushStruct( PushBuffer->Arena, render_group );
	NewRenderGroup->Mesh = Mesh;
	NewRenderGroup->Material = Material;
	Push<render_group>( PushBuffer->RenderGroups, NewRenderGroup );
}

void InitiatePushBuffer(render_push_buffer* PushBuffer, game_offscreen_buffer* aOffscreenBuffer, depth_buffer* DepthBuffer, memory_arena* aArena)
{	
	*PushBuffer = {};
	PushBuffer->Arena = aArena;
	PushBuffer->DepthBuffer = DepthBuffer;
	PushBuffer->TemporaryMemory = BeginTemporaryMemory(PushBuffer->Arena);
	PushBuffer->OffscreenBuffer = aOffscreenBuffer;
	PushBuffer->RenderGroups = CreateFiloBuffer( PushBuffer->Arena );
	PushBuffer->Lights = CreateFiloBuffer( PushBuffer->Arena );
}

void ClearPushBuffer(render_push_buffer* Buffer)
{
	Assert(Buffer->Arena);
	Assert(Buffer->OffscreenBuffer);
	EndTemporaryMemory(Buffer->TemporaryMemory);
	*Buffer = {};
}

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

r32 EdgeFunction( v2 a, v2 b, v2 p )
{
	r32 Result = (a.X-p.X) * (b.Y-a.Y) - (b.X-a.X) * (a.Y-p.Y);
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


void GouradShading(game_offscreen_buffer* OffscreenBuffer, depth_buffer* DepthBuffer, 
					v4* CameraPosition, aabb2d* RasterBB, v2* RasterPoints, 
					v4* Vertices, v4* VertexNormals, v4* ViewPoints, 
					u32 NrFragmentColors, fragment_color* PerLightColors )
{
	r32 Area2 = EdgeFunction( RasterPoints[0], RasterPoints[1], RasterPoints[2] );
	if( Area2 <= 0 )
	{
		return;
	}
	
	r32 near_clipping_plane = -1;
	r32 PremulArea2 = 1/Area2;

	v4 Color[3] = {};
	for( u32 LightIndex = 0; LightIndex < NrFragmentColors; ++LightIndex )
	{
		for( u32 TriangleIndex = 0; TriangleIndex < 3; ++TriangleIndex )
		{
			fragment_color& Fragment = PerLightColors[LightIndex];
			v4& VerticeColor = Color[TriangleIndex];
			VerticeColor += CalculateColor( Vertices[TriangleIndex], 
												 VertexNormals[TriangleIndex], 
												 *CameraPosition, 
												 Fragment.LightPosition, 
									     		 Fragment.AmbientColor, 
									     		 Fragment.DiffuseColor, 
									     		 Fragment.SpecularColor, 
									     		 Fragment.Shininess );

			VerticeColor.X = (VerticeColor.X > 1) ? 1 : VerticeColor.X;
			VerticeColor.Y = (VerticeColor.Y > 1) ? 1 : VerticeColor.Y;
			VerticeColor.Z = (VerticeColor.Z > 1) ? 1 : VerticeColor.Z;
			VerticeColor.W = 1;
		}
	}

	for(s32 i = RoundReal32ToInt32( RasterBB->min.X ); i < RasterBB->max.X; ++i)
	{
		for(s32 j = RoundReal32ToInt32( RasterBB->min.Y ); j < RasterBB->max.Y; ++j)
		{
			v2 p = V2( (r32) i, (r32) j);
			r32 PixelInRange0 = EdgeFunction( RasterPoints[0], RasterPoints[1], p);
			r32 PixelInRange1 = EdgeFunction( RasterPoints[1], RasterPoints[2], p);
			r32 PixelInRange2 = EdgeFunction( RasterPoints[2], RasterPoints[0], p);

			if( ( PixelInRange0 >= 0 ) && 
				( PixelInRange1 >= 0) &&
				( PixelInRange2 >= 0) )
			{

				// Barycentric Coordinates
				r32 Lambda0 = PixelInRange1 * PremulArea2;
				r32 Lambda1 = PixelInRange2 * PremulArea2;
				r32 Lambda2 = PixelInRange0 * PremulArea2;

				r32 PixelDepthValue = Lambda0 * ViewPoints[0].Z + Lambda1 * ViewPoints[1].Z + Lambda2 * ViewPoints[2].Z;

				r32& BufferDepthValue = DepthBuffer->Buffer[ j * DepthBuffer->Width  + i];

				if(( PixelDepthValue > BufferDepthValue)  && ( PixelDepthValue < near_clipping_plane ))
				{
					BufferDepthValue = PixelDepthValue;
					u8* PixelLocation = ((u8*)OffscreenBuffer->Memory + i * OffscreenBuffer->BytesPerPixel +
										j * OffscreenBuffer->Pitch);

					v4 InterpolatedColor = Lambda0 * Color[0] + Lambda1 * Color[1] + Lambda2 * Color[2];

					r32 Red   = InterpolatedColor.X;
					r32 Green = InterpolatedColor.Y;
					r32 Blue  = InterpolatedColor.Z;

					u32* Pixel = (u32*)PixelLocation;
					*Pixel = ((TruncateReal32ToInt32(Red*255.f) << 16) |
							  (TruncateReal32ToInt32(Green*255.f) << 8)  |
							  (TruncateReal32ToInt32(Blue*255.f) << 0));
				}
			}
		}
	}

}

void PhongShading(game_offscreen_buffer* OffscreenBuffer, depth_buffer* DepthBuffer, 
						v4* CameraPosition, aabb2d* RasterBB, v2* RasterPoints, 
						v4* Vertices, v4* VertexNormals, v4* ViewPoints, 
						u32 NrFragmentColors, fragment_color* PerLightColors )
{
	r32 Area2 = EdgeFunction( RasterPoints[0], RasterPoints[1], RasterPoints[2] );
	if( Area2 <= 0 )
	{
		return;
	}
	
	r32 near_clipping_plane = -1;
	r32 PremulArea2 = 1/Area2;

	for(s32 i = RoundReal32ToInt32( RasterBB->min.X ); i < RasterBB->max.X; ++i)
	{
		for(s32 j = RoundReal32ToInt32( RasterBB->min.Y ); j < RasterBB->max.Y; ++j)
		{
			v2 p = V2( (r32) i, (r32) j);
			r32 PixelInRange0 = EdgeFunction( RasterPoints[0], RasterPoints[1], p);
			r32 PixelInRange1 = EdgeFunction( RasterPoints[1], RasterPoints[2], p);
			r32 PixelInRange2 = EdgeFunction( RasterPoints[2], RasterPoints[0], p);

			if( ( PixelInRange0 >= 0 ) && 
				( PixelInRange1 >= 0) &&
				( PixelInRange2 >= 0) )
			{

				// Barycentric Coordinates
				r32 Lambda0 = PixelInRange1 * PremulArea2;
				r32 Lambda1 = PixelInRange2 * PremulArea2;
				r32 Lambda2 = PixelInRange0 * PremulArea2;

				r32 PixelDepthValue = Lambda0 * ViewPoints[0].Z + Lambda1 * ViewPoints[1].Z + Lambda2 * ViewPoints[2].Z;

				r32& BufferDepthValue = DepthBuffer->Buffer[ j * DepthBuffer->Width  + i];

				if(( PixelDepthValue > BufferDepthValue)  && ( PixelDepthValue < near_clipping_plane ))
				{
					BufferDepthValue = PixelDepthValue;
					u8* PixelLocation = ((u8*)OffscreenBuffer->Memory + i * OffscreenBuffer->BytesPerPixel +
										j * OffscreenBuffer->Pitch);

					v4 No = Lambda0 * VertexNormals[0] + Lambda1 * VertexNormals[1] + Lambda2 * VertexNormals[2];
					v4 Ve = Lambda0 *  Vertices[0] + Lambda1 *  Vertices[1] + Lambda2 *  Vertices[2];


					v4 Color = V4(0,0,0,0);
					for( u32 LightIndex = 0; LightIndex < NrFragmentColors; ++LightIndex )
					{
						fragment_color& Fragment = PerLightColors[LightIndex];
						Color += CalculateColor( Ve, No, 	*CameraPosition, 
														Fragment.LightPosition, 
												     	Fragment.AmbientColor, 
												     	Fragment.DiffuseColor, 
												     	Fragment.SpecularColor, 
												     	Fragment.Shininess );
						
						Color.X = (Color.X > 1) ? 1 : Color.X;
						Color.Y = (Color.Y > 1) ? 1 : Color.Y;
						Color.Z = (Color.Z > 1) ? 1 : Color.Z;
						Color.W = 1;
					}
					r32 Red   = Color.X;
					r32 Green = Color.Y;
					r32 Blue  = Color.Z;

					u32* Pixel = (u32*)PixelLocation;
					*Pixel = ((TruncateReal32ToInt32(Red*255.f) << 16) |
							  (TruncateReal32ToInt32(Green*255.f) << 8)  |
							  (TruncateReal32ToInt32(Blue*255.f) << 0));
				}
			}
		}
	}
}

void DrawTriangles( render_push_buffer* PushBuffer )
{
	// Get camera matrices
	m4& V =  PushBuffer->Camera->V; 	// ViewMatrix
	m4& P = PushBuffer->Camera->P;		// ProjectionMatrix
	m4& R = PushBuffer->Camera->R;		// RasterizationMatrix
	m4 	RPV= R*P*V;						// All in one

	v4 CameraPosition = Transpose( RigidInverse( V ) ).r3;

	// 'Reset' DepthBuffer
	game_offscreen_buffer* OffscreenBuffer =  PushBuffer->OffscreenBuffer;
	depth_buffer* DepthBuffer = PushBuffer->DepthBuffer;
	for( u32 i = 0; i<DepthBuffer->Width*DepthBuffer->Height; ++i)
	{
		DepthBuffer->Buffer[i] = -10E10;
	}

	r32 DiffsePremul = 1/Pi32;
	r32 SpecularPremul = 1/(8*Pi32);

	// For each render group
	render_group* RenderGroup = 0;
	while( ( RenderGroup = Pop<render_group>(PushBuffer->RenderGroups) ) )
	{
		// Transform Matrix for points
		m4& T = RenderGroup->Mesh->T;
		// Transform Matrix for normals
		m4 NT = Transpose( RigidInverse(T) );

		temporary_memory LightMemory = BeginTemporaryMemory( PushBuffer->Arena );
		fragment_color* PerLightColors = (fragment_color*) PushArray( PushBuffer->Arena, PushBuffer->Lights->Size, fragment_color );
		component_material* Material = RenderGroup->Material;
		
		filo_buffer_entry* LightEntry = PushBuffer->Lights->First;
		u32 LightIndex = 0;
		do
		{
			component_light* Light = (component_light*) LightEntry->Data;

			fragment_color& Fragment = PerLightColors[LightIndex];
			Fragment.LightPosition  = Light->Position;

			v4 LightColor  = Light->Color;
			Fragment.AmbientColor  = V4(	LightColor.X * Material->AmbientColor.X,  
											LightColor.Y * Material->AmbientColor.Y,  
											LightColor.Z * Material->AmbientColor.Z,  
											LightColor.W * Material->AmbientColor.W );
			Fragment.DiffuseColor  = V4(	LightColor.X * Material->DiffuseColor.X,  
											LightColor.Y * Material->DiffuseColor.Y,  
											LightColor.Z * Material->DiffuseColor.Z,  
											LightColor.W * Material->DiffuseColor.W );
			Fragment.AmbientColor = DiffsePremul*Fragment.AmbientColor;

			Fragment.SpecularColor = V4(	LightColor.X * Material->SpecularColor.X, 
											LightColor.Y * Material->SpecularColor.Y,
								 			LightColor.Z * Material->SpecularColor.Z, 
			 								LightColor.W * Material->SpecularColor.W );

			Fragment.Shininess = Material->Shininess;

			Fragment.SpecularColor = (SpecularPremul * (Fragment.Shininess + 8) ) * Fragment.SpecularColor;
			LightIndex++;
		}while( (LightEntry = LightEntry->Next) );

		

		obj_geometry* Object = RenderGroup->Mesh->Object;
		u32 NrTrianglesInMesh = Object->nt;
		for( u32 TriangleIndex = 0; TriangleIndex < NrTrianglesInMesh; ++TriangleIndex) 
		{
			triangle Triangle = Object->t[TriangleIndex];
			
			v4 v[3]  = {};
			v4 vn[3] = {};

			v[0] = T*Object->v[ Triangle.vi[0] ];
			v[1] = T*Object->v[ Triangle.vi[1] ];
			v[2] = T*Object->v[ Triangle.vi[2] ];

			if(Triangle.HasVerticeNormals)
			{
				vn[0] = NT*Object->vn[ Triangle.vni[0] ];
				vn[1] = NT*Object->vn[ Triangle.vni[1] ];
				vn[2] = NT*Object->vn[ Triangle.vni[2] ];
			}else{
				vn[0] = vn[1] = vn[2] = NT*Triangle.n;
			}

			// Get Bounding Box in raster space
			v2 RasterPoints[3] = { V2( PointMultiply(RPV, v[0]) ), V2( PointMultiply(RPV, v[1]) ), V2( PointMultiply(RPV, v[2]) ) };
			aabb2d RasterBB = getBoundingBox( RasterPoints[0], RasterPoints[1], RasterPoints[2] );

			// OffScreenCulling
			if( (RasterBB.max.X < 0)  || 
				(RasterBB.max.Y < 0 ) ||
				(RasterBB.min.X >= OffscreenBuffer->Width) || 
				(RasterBB.min.Y >= OffscreenBuffer->Height))
			{
				continue;
			}

			RasterBB.min.X = Maximum(0, RasterBB.min.X);
			RasterBB.max.X = Minimum((r32) OffscreenBuffer->Width, RasterBB.max.X);
			RasterBB.min.Y = Maximum(0, RasterBB.min.Y);
			RasterBB.max.Y = Minimum((r32) OffscreenBuffer->Height, RasterBB.max.Y);

			v4 ViewPoints[3] ={ V*v[0] ,V*v[1] , V*v[2] };
			#if 1
			GouradShading(OffscreenBuffer, DepthBuffer, &CameraPosition, &RasterBB, RasterPoints, v, vn, ViewPoints,  PushBuffer->Lights->Size, PerLightColors);
			#else
			PhongShading(OffscreenBuffer, DepthBuffer, &CameraPosition, &RasterBB, RasterPoints, v, vn, ViewPoints,  PushBuffer->Lights->Size, PerLightColors);
			#endif

		}
		EndTemporaryMemory( LightMemory );
	}
}