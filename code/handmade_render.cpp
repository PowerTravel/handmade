#ifndef HANDMADE_RENDER
#define HANDMADE_RENDER

// Extremely Fast Line Algorithm Var E (Addition Fixed Point PreCalc)
// Copyright 2001-2, By Po-Han Lin


// Freely useable in non-commercial applications as long as credits 
// to Po-Han Lin and link to http://www.edepot.com is provided in 
// source code and can been seen in compiled executable.  
// Commercial applications please inquire about licensing the algorithms.
//
// Lastest version at http://www.edepot.com/phl.html
// This version is for standard displays (up to 65536x65536)
// For small display version (256x256) visit http://www.edepot.com/lineex.html

struct SURFACE;

// used by myLine
void myPixel(SURFACE* surface, int x,int y) {
	// PLOT x,y point on surface

}

// THE EXTREMELY FAST LINE ALGORITHM Variation E (Addition Fixed Point PreCalc)
void myLine(SURFACE* surface, int x, int y, int x2, int y2) {
   	bool yLonger=false;
	int shortLen=y2-y;
	int longLen=x2-x;
	if (abs(shortLen)>abs(longLen)) {
		int swap=shortLen;
		shortLen=longLen;
		longLen=swap;				
		yLonger=true;
	}
	int decInc;
	if (longLen==0) decInc=0;
	else decInc = (shortLen << 16) / longLen;

	if (yLonger) {
		if (longLen>0) {
			longLen+=y;
			for (int j=0x8000+(x<<16);y<=longLen;++y) {
				myPixel(surface,j >> 16,y);	
				j+=decInc;
			}
			return;
		}
		longLen+=y;
		for (int j=0x8000+(x<<16);y>=longLen;--y) {
			myPixel(surface,j >> 16,y);	
			j-=decInc;
		}
		return;	
	}

	if (longLen>0) {
		longLen+=x;
		for (int j=0x8000+(y<<16);x<=longLen;++x) {
			myPixel(surface,x,j >> 16);
			j+=decInc;
		}
		return;
	}
	longLen+=x;
	for (int j=0x8000+(y<<16);x>=longLen;--x) {
		myPixel(surface,x,j >> 16);
		j-=decInc;
	}

}

void mySquare(SURFACE* surface,int x, int y, int x2, int y2) {
	myLine(surface,x,y,x2,y2);
	myLine(surface,x2,y2,x2+(y-y2),y2+(x2-x));
	myLine(surface,x,y,x+(y-y2),y+(x2-x));
	myLine(surface,x+(y-y2),y+(x2-x),x2+(y-y2),y2+(x2-x));
}


void myRect(SURFACE* surface, int x, int y, int x2, int y2) {
	myLine(surface,x,y,x2,y);
	myLine(surface,x2,y,x2,y2);
	myLine(surface,x2,y2,x,y2);
	myLine(surface,x,y2,x,y);
}


internal void
DrawRectangle(game_offscreen_buffer* Buffer, real32 RealMinX, real32 RealMinY, real32 Width, real32 Height, real32 R, real32 G, real32 B)
{
	int32 MinX = RoundReal32ToInt32(RealMinX);
	int32 MinY = RoundReal32ToInt32(RealMinY);
	int32 MaxX = RoundReal32ToInt32(RealMinX + Width);
	int32 MaxY = RoundReal32ToInt32(RealMinY + Height);

	if (MinX < 0)
	{
		MinX = 0;
	}
	if (MinY < 0)
	{
		MinY = 0;
	}
	if (MaxX > Buffer->Width)
	{
		MaxX = Buffer->Width;
	}
	if (MaxY > Buffer->Height)
	{
		MaxY = Buffer->Height;
	}

	uint8* Row = ((uint8*)Buffer->Memory + MinX * Buffer->BytesPerPixel +
		MinY * Buffer->Pitch);
	for (int Y = MinY; Y < MaxY; ++Y)
	{
		uint32* Pixel = (uint32*)Row;
		for (int X = MinX; X < MaxX; ++X)
		{
			*Pixel++ = ((TruncateReal32ToInt32(R*255.f) << 16) |
				(TruncateReal32ToInt32(G*255.f) << 8) |
				(TruncateReal32ToInt32(B*255.f) << 0));
		}
		Row += Buffer->Pitch;
	}
}

void PutPixel(game_offscreen_buffer* Buffer, int32 X, int32 Y, real32 R, real32 G, real32 B)
{
	if((X < 0) || (Y < 0) || (X >= Buffer->Width) || (Y >= Buffer->Height))
	{
		return;
	} 
	uint8* PixelLocation = ((uint8*)Buffer->Memory + X * Buffer->BytesPerPixel +
		Y * Buffer->Pitch);
	uint32* Pixel = (uint32*)PixelLocation;
	*Pixel = ((TruncateReal32ToInt32(R*255.f) << 16) |
		(TruncateReal32ToInt32(G*255.f) << 8) |
		(TruncateReal32ToInt32(B*255.f) << 0));
}


void DrawCircle(game_offscreen_buffer* Buffer, real32 RealX0, real32 RealY0, real32 RealRadius, real32 R = 1, real32 G = 1, real32 B = 1)
{
	int32 x0 = RoundReal32ToInt32(RealX0);
	int32 y0 = RoundReal32ToInt32(RealY0);
	int32 radius = RoundReal32ToInt32(RealRadius);

	int32 x = radius - 1;
	int32 y = 0;
	int32 dx = 1;
	int32 dy = 1;
	int32 err = dx - (radius << 1);


	while (x >= y)
	{
		PutPixel(Buffer, x0 + x, y0 + y, R, G, B);
		PutPixel(Buffer, x0 + y, y0 + x, R, G, B);
		PutPixel(Buffer, x0 - y, y0 + x, R, G, B);
		PutPixel(Buffer, x0 - x, y0 + y, R, G, B);
		PutPixel(Buffer, x0 - x, y0 - y, R, G, B);
		PutPixel(Buffer, x0 - y, y0 - x, R, G, B);
		PutPixel(Buffer, x0 + y, y0 - x, R, G, B);
		PutPixel(Buffer, x0 + x, y0 - y, R, G, B);

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

		if (err <= 0)
		{
			y++;
			err += dy;
			dy += 2;
		}

		if (err > 0)
		{
			x--;
			dx += 2;
			err += dx - (radius << 1);
		}
	}

}


// makes the packing compact
#pragma pack(push, 1)
struct bmp_header {

	uint16   FileType;     /* File type, always 4D42h ("BM") */
	uint32  FileSize;     /* Size of the file in bytes */
	uint16   Reserved1;    /* Always 0 */
	uint16   Reserved2;    /* Always 0 */
	uint32  BitmapOffset; /* Starting position of image data in bytes */

	uint32 HeaderSize;       /* Size of this header in bytes */
	int32  Width;           /* Image width in pixels */
	int32  Height;          /* Image height in pixels */
	uint16  Planes;          /* Number of color planes */
	uint16  BitsPerPixel;    /* Number of bits per pixel */
	uint32 Compression;     /* Compression methods used */
	uint32 SizeOfBitmap;    /* Size of bitmap in bytes */
	int32  HorzResolution;  /* Horizontal resolution in pixels per meter */
	int32  VertResolution;  /* Vertical resolution in pixels per meter */
	uint32 ColorsUsed;      /* Number of colors in the image */
	uint32 ColorsImportant; /* Minimum number of important colors */
	/* Fields added for Windows 4.x follow this line */

	uint32 RedMask;       /* Mask identifying bits of red component */
	uint32 GreenMask;     /* Mask identifying bits of green component */
	uint32 BlueMask;      /* Mask identifying bits of blue component */
	uint32 AlphaMask;     /* Mask identifying bits of alpha component */
	uint32 CSType;        /* Color space type */
	int32  RedX;          /* X coordinate of red endpoint */
	int32  RedY;          /* Y coordinate of red endpoint */
	int32  RedZ;          /* Z coordinate of red endpoint */
	int32  GreenX;        /* X coordinate of green endpoint */
	int32  GreenY;        /* Y coordinate of green endpoint */
	int32  GreenZ;        /* Z coordinate of green endpoint */
	int32  BlueX;         /* X coordinate of blue endpoint */
	int32  BlueY;         /* Y coordinate of blue endpoint */
	int32  BlueZ;         /* Z coordinate of blue endpoint */
	uint32 GammaRed;      /* Gamma red coordinate scale value */
	uint32 GammaGreen;    /* Gamma green coordinate scale value */
	uint32 GammaBlue;     /* Gamma blue coordinate scale value */
};
#pragma pack(pop)

internal void
BlitBMP(game_offscreen_buffer* Buffer, real32 RealMinX, real32 RealMinY, loaded_bitmap BitMap)
{
	int32 MinX = RoundReal32ToInt32(RealMinX);
	int32 MinY = RoundReal32ToInt32(RealMinY);
	int32 MaxX = RoundReal32ToInt32(RealMinX + (real32)BitMap.Width);
	int32 MaxY = RoundReal32ToInt32(RealMinY + (real32)BitMap.Height);


	uint32 ClippingOffsetX = 0;
	uint32 ClippingOffsetY = 0;
	if (MinX < 0)
	{
		ClippingOffsetX = -MinX;
		MinX = 0;
	}
	if (MinY < 0)
	{
		ClippingOffsetY = -MinY;
		MinY = 0;
	}
	if (MaxX > Buffer->Width)
	{
		MaxX = Buffer->Width;
	}
	if (MaxY > Buffer->Height)
	{
		MaxY = Buffer->Height;
	}


	uint32 BytesPerPixel = 4;

	uint8* SourceRow = (uint8*)BitMap.Pixels + (BitMap.Width*ClippingOffsetY + ClippingOffsetX)*BytesPerPixel;
	uint8* DestinationRow = (uint8*)((uint8*)Buffer->Memory + MinY* Buffer->Pitch +
		MinX * Buffer->BytesPerPixel);


	uint32 BitmapPitch = BitMap.Width * BytesPerPixel;

	for (int Y = MinY; Y < MaxY; ++Y)
	{
		uint32* SourcePixel = (uint32*)SourceRow;
		uint32* DesinationPixel = (uint32*)DestinationRow;
		for (int X = MinX; X < MaxX; ++X)
		{

			/// Note(Jakob): This loop is SLOW!! Tanking the fps

			uint32 Source = *SourcePixel;

			real32 Alpha = (real32)((Source & 0xFF000000) >> 24) / 255.f;

			real32 Red = (real32)(((Source & 0x00FF0000) >> 16)) / 255.f;
			real32 Green = (real32)(((Source & 0x0000FF00) >> 8)) / 255.f;
			real32 Blue = (real32)(((Source & 0x000000FF) >> 0)) / 255.f;

			uint32 DestPix = *DesinationPixel;
			real32 DRed = (real32)(((DestPix & 0x00FF0000) >> 16)) / 255.f;
			real32 DGreen = (real32)(((DestPix & 0x0000FF00) >> 8)) / 255.f;
			real32 DBlue = (real32)(((DestPix & 0x000000FF) >> 0)) / 255.f;

			Red = (1 - Alpha) *DRed + Alpha*Red;
			Green = (1 - Alpha)* DGreen + Alpha*Green;
			Blue = (1 - Alpha) *DBlue + Alpha*Blue;

			int32 R = (int)(Red * 255 + 0.5);
			int32 G = (int)(Green * 255 + 0.5);
			int32 B = (int)(Blue * 255 + 0.5);

			Source = (R << 16) | (G << 8) | (B << 0);

			*DesinationPixel = Source;

			DesinationPixel++;
			SourcePixel++;

		}
		DestinationRow += Buffer->Pitch;
		SourceRow += BitmapPitch;

	}
}

void DrawLineBresLow( game_offscreen_buffer* Buffer, int32 x0, int32 y0, int32 x1, int32 y1, v4 Color )
{
	int32 dx = x1 - x0;
	int32 dy = y1 - y0;
	int32 yi = 1;
	if (dy < 0)
	{
		yi = -1;
		dy = -dy;
	}
	int32 D = 2 * dy - dx;
	int32 y = y0;

	for (int x = x0; x <= x1; ++x)
	{
		PutPixel(Buffer, x, y, Color.X, Color.Y, Color.Z);
		if (D > 0)
		{
			y = y + yi;
			D = D - 2 * dx;
		}
		D = D + 2 * dy;
	}
}


void DrawLineBresHigh(game_offscreen_buffer* Buffer, int32 x0, int32 y0, int32 x1, int32 y1, v4 Color)
{
	int32 dx = x1 - x0;
	int32 dy = y1 - y0;
	int32 xi = 1;
	if (dx < 0)
	{
		xi = -1;
		dx = -dx;
	}
	int32 D = 2 * dx - dy;
	int32 x = x0;
	for (int y = y0; y <= y1; ++y)
	{
		PutPixel(Buffer, x, y, Color.X, Color.Y, Color.Z);
		if (D > 0)
		{
			x = x + xi;
			D = D - 2 * dy;
		}

		D = D + 2 * dx;
	}
}

void DrawLineBres(game_offscreen_buffer* Buffer, int32 x0, int32 y0, int32 x1, int32 y1, v4 Color)
{

	if (Abs((real32)(y1 - y0)) < Abs((real32)(x1 - x0)))
	{
		if (x0 > x1) {
			DrawLineBresLow(Buffer, x1, y1, x0, y0, Color );
		}
		else {
			DrawLineBresLow(Buffer, x0, y0, x1, y1, Color );
		}
	}
	else {
		if (y0 > y1) {
			DrawLineBresHigh(Buffer, x1, y1, x0, y0, Color );
		}
		else {
			DrawLineBresHigh(Buffer, x0, y0, x1, y1, Color );
		}
	}
}


v4 Flatshading(v4 fCenter, v4 fNormal, v4 LightPosition, m4 V, v4 AmbientProduct, v4 DiffuseProduct, v4 SpecularProduct)
{
	// T = Model Matrix;
	// V = View Matrix;
	real32 Shininess = 10;
	real32 flippNormals = 0;

	v3 vpos = V3(V*fCenter);
	v3 lpos = V3(V*LightPosition);
	v3 L = normalize(lpos - vpos);
	v3 E = normalize(-vpos);
	v3 H = normalize(L + E);
	v3 N = normalize(V3(V*fNormal));

	v4 ambient = AmbientProduct;

	real32 Kd = Maximum(L*N, 0.0f);
	v4 diffuse = Kd*DiffuseProduct;

	if (L*N < 0.0)
	{
		diffuse = V4(0.0, 0.0, 0.0, 0.0);
	}

	real32 Ks = Pow(Maximum(N*H, 0.0f), Shininess);
	v4 specular = Ks * SpecularProduct;

	if (L*N < 0.0)
	{
		specular = V4(0.0, 0.0, 0.0, 0.0);
	}

	v4 Result;
	Result = ambient + diffuse + specular;
	Result.X = (Result.X > 1) ? 1 : Result.X;
	Result.Y = (Result.Y > 1) ? 1 : Result.Y;
	Result.Z = (Result.Z > 1) ? 1 : Result.Z;
	Result.W = 1;

	return Result;
}


struct vertex_data
{
	v4 v;
	v4 n;
	v4 c;
};

vertex_data BlinnPhong(v4 vPosition, v4 vNormal, v4 AmbientProduct, v4 DiffuseProduct, v4 SpecularProduct, m4 T, m4 V, m4 P)
{
	// T = Model Matrix;
	// V = View Matrix;
	// P = Projection Matrix;
	local_persist real32 dt = 0;
	v4 LightPosition = V4(0, 0, 0, 1);
	dt += Pi32 / 600.f;
	real32 Shininess = 0.1;
	real32 flippNormals = 0;
	/////////////////////////////

	v4 norm = -vNormal;
	if (flippNormals == 1)
	{
		norm = -norm;
	}

	m4 ModelView = V*T;
	v3 pos = V3(ModelView*vPosition);

	v3 L = normalize(V3(V*LightPosition) - pos);
	v3 E = normalize(-pos);
	v3 H = normalize(L + E);
	v3 N = normalize(V3(ModelView*norm));

	v4 ambient = AmbientProduct;

	real32 Kd = Maximum(L*N, 0.0f);
	v4 diffuse = Kd*DiffuseProduct;

	if (L*N < 0.0)
	{
		diffuse = V4(0.0, 0.0, 0.0, 1.0);
	}

	real32 Ks = Pow(Maximum(N*H, 0.0f), Shininess);
	v4 specular = Ks * SpecularProduct;

	if (L*N < 0.0)
	{
	//	specular = V4(0.0, 0.0, 0.0, 1.0);
	}

	vertex_data Result;
	Result.v = P*ModelView*vPosition;
	Result.n = ModelView*norm;
	Result.c = ambient + diffuse + specular;
	//Result.c = diffuse;// + specular;
	Result.c.W = 0;

	Result.c.X = (Result.c.X > 1) ? 1 : Result.c.X;
	Result.c.Y = (Result.c.Y > 1) ? 1 : Result.c.Y;
	Result.c.Z = (Result.c.Z > 1) ? 1 : Result.c.Z;
	Result.c.W = 1;
	if (Result.c.X > 1)
	{
		Assert(false)
	}
	return Result;
}

struct aabb2d
{
	v2 min;
	v2 max;
};

struct render_entity
{
	v4 vertices[3];
	v4 verticeNormals[3];
	v4 normal;
	v4 AmbientProduct;
	v4 DiffuseProduct;
	v4 SpecularProduct;
	v4 LightPosition;
	render_entity* Next;
};

struct render_push_buffer
{
	memory_arena* Arena;
	temporary_memory TemporaryMemory;

	m4 R; // Rasterization Matrix
	m4 V; // View Matrix
	m4 P; // Projection Matrix
	game_offscreen_buffer* OffscreenBuffer;
	//game_z_buffer* zBuffer;
	render_entity* First;
	render_entity* Last;
};

void InitiatePushBuffer(render_push_buffer* PushBuffer, game_offscreen_buffer* aOffscreenBuffer, memory_arena* aArena)
{	
	*PushBuffer = {};
	PushBuffer->Arena = aArena;
	PushBuffer->TemporaryMemory = BeginTemporaryMemory(PushBuffer->Arena);
	PushBuffer->OffscreenBuffer = aOffscreenBuffer;
}

void ClearPushBuffer(render_push_buffer* Buffer)
{
	Assert(Buffer->Arena);
	Assert(Buffer->OffscreenBuffer);
	EndTemporaryMemory(Buffer->TemporaryMemory);
	*Buffer = {};
}

void PushBuffer(render_push_buffer* Buffer, render_entity Entity )
{
	render_entity* EntityCopy = (render_entity*) PushStruct(Buffer->Arena, render_entity);
	*EntityCopy = Entity;
	if( ! Buffer->First )
	{
		Assert( EntityCopy->Next == 0);
		Buffer->First = EntityCopy;
		Buffer->Last  = EntityCopy;
		return;
	}

	Assert( Buffer->Last->Next == 0 );
	Buffer->Last->Next = EntityCopy;
	Buffer->Last = EntityCopy;
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

real32 EdgeFunction( v2 a, v2 b, v2 p )
{
	real32 Result = (a.X - p.X) * ( b.Y-a.Y ) - (b.X-a.X)*( a.Y-p.Y );
	return(Result);
}

void DrawTriangles( render_push_buffer* PushBuffer )
{
	local_persist  real32 dt = 0;
	dt =  (dt<10*Pi32) ? (dt-10*Pi32) : (dt+ 0.01f);

	m4 RasterizerProjectionViewMatrix = PushBuffer->R * PushBuffer->P * PushBuffer->V;
	game_offscreen_buffer* OffscreenBuffer =  PushBuffer->OffscreenBuffer;
	for(render_entity* Triangle = PushBuffer->First; 
					   Triangle != 0; 
					   Triangle = Triangle->Next)
	{
		v4* v = Triangle->vertices;
		v2 p2[3] = {};

		for( int i = 0; i<3; ++i)
		{
			v[i] = RasterizerProjectionViewMatrix * v[i];
			p2[i] = V2(v[i]);
		}

		aabb2d Box = getBoundingBox( p2[0], p2[1], p2[2] );
		if( (Box.max.X < 0)  || 
			(Box.max.Y < 0 ) ||
			(Box.min.X >= OffscreenBuffer->Width) || 
			(Box.min.Y >= OffscreenBuffer->Height))
		{
			continue;
		}

		v4 Color = V4(0.7,0.2,0.4,0); // 
		v4 triangleCenter = (v[0] + v[1] + v[2])/3;
		Color = Flatshading(triangleCenter, Triangle->normal, Triangle->LightPosition,  PushBuffer->V, Triangle->AmbientProduct, 
				Triangle->DiffuseProduct, Triangle->SpecularProduct );

		Box.min.X = Maximum(0, Box.min.X);
		Box.max.X = Minimum((real32) OffscreenBuffer->Width, Box.max.X);
		Box.min.Y = Maximum(0, Box.min.Y);
		Box.max.Y = Minimum((real32) OffscreenBuffer->Height, Box.max.Y);


		real32 Area2 = EdgeFunction( p2[0], p2[1], p2[2] );
		if( Area2 < 0 )
		{
			continue;
		}
		for(int32 i = RoundReal32ToInt32( Box.min.X ); i < Box.max.X; ++i)
		{
			for(int32 j = RoundReal32ToInt32( Box.min.Y ); j < Box.max.Y; ++j)
			{
				v2 p = V2( (real32) i, (real32) j);
				real32 PixelInRange1 = EdgeFunction( p2[0], p2[1], p);
				real32 PixelInRange2 = EdgeFunction( p2[1], p2[2], p);
				real32 PixelInRange3 = EdgeFunction( p2[2], p2[0], p);

				if( ( PixelInRange1 >= 0 ) && 
					( PixelInRange2 >= 0 ) &&
					( PixelInRange3 >= 0 ) )
				{
					uint8* PixelLocation = ((uint8*)OffscreenBuffer->Memory + i * OffscreenBuffer->BytesPerPixel +
													j * OffscreenBuffer->Pitch);
					uint32* Pixel = (uint32*)PixelLocation;
					*Pixel = ((TruncateReal32ToInt32(Color.X*255.f) << 16) |
							  (TruncateReal32ToInt32(Color.Y*255.f) << 8) |
							  (TruncateReal32ToInt32(Color.Z*255.f) << 0));
				}
			}
		}
	}
}

#endif HANDMADE_RENDER