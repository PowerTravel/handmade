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

void fillBottomFlatTriangleFlat(game_offscreen_buffer* Buffer, v2 A, v2 B, v2 C, v4 Color)
{
	//    A
	//   /  \
	//  /    \
	// B----- C

	Assert(A.Y >= B.Y);
	Assert(B.Y == C.Y);

	if( C.X < B.X )
	{
		v2 swp = B;
		B = C;
		C =swp;
	}


	uint32 PixelValue = (TruncateReal32ToInt32(Color.X*255.f) << 16) | 
						(TruncateReal32ToInt32(Color.Y*255.f) << 8 ) | 
						(TruncateReal32ToInt32(Color.Z*255.f) << 0 );

	real32 ABslope = 0;
	real32 ABdy = B.Y - A.Y;
	if (ABdy != 0)
	{
		ABslope = (B.X - A.X) / ABdy;
	}

	real32 ACslope = 0;
	real32 ACdy = C.Y - A.Y;
	if (ACdy != 0)
	{
		ACslope = (C.X - A.X) / ACdy;
	}

	real32 MinX = A.X;
	real32 MaxX = A.X;

	int32 MaxY = RoundReal32ToInt32(A.Y);
	int32 MinY = RoundReal32ToInt32(B.Y);

	real32 Width = C.X - B.X;

	for (int32 Y = MaxY; Y >= MinY; --Y)
	{
		int32 StartX = RoundReal32ToInt32(MinX);
		int32 StopX  = RoundReal32ToInt32(MaxX);
		
		for( int32 X = StartX; X <= StopX; ++X )
		{
			if((X < 0) || (Y < 0) || (X >= Buffer->Width) || (Y >= Buffer->Height))
			{
				continue;
			}
		
			uint8* PixelLocation = ((uint8*)Buffer->Memory + X * Buffer->BytesPerPixel +
				Y * Buffer->Pitch);
			uint32* Pixel = (uint32*) PixelLocation;
			*Pixel = PixelValue;
		
		}

		MinX -= ABslope;
		MaxX -= ACslope;

		if( (MaxX - MinX) > Width )
		{
			MinX = B.X;
			MaxX = C.X;
		}
	}
}

void fillTopFlatTriangleFlat(game_offscreen_buffer* Buffer, v2 A, v2 B, v2 C, v4 Color)
{
	//C---B
	// \ /
	//	A

	Assert(A.Y <= B.Y);
	Assert(B.Y == C.Y);

	if( C.X > B.X )
	{
		v2 swp = B;
		B = C;
		C =swp;
	}

	uint32 PixelValue = (TruncateReal32ToInt32(Color.X*255.f) << 16) | 
						(TruncateReal32ToInt32(Color.Y*255.f) << 8 ) | 
						(TruncateReal32ToInt32(Color.Z*255.f) << 0 );
	real32 ABslope = 0;
	real32 ABdy = B.Y - A.Y;
	if (ABdy != 0)
	{
		ABslope = (B.X - A.X) / ABdy;
	}

	real32 ACslope = 0;
	real32 ACdy = C.Y - A.Y;
	if (ACdy != 0)
	{
		ACslope = (C.X - A.X) / ACdy;
	}

	real32 MinX = A.X;
	real32 MaxX = A.X;


	int32 MaxY = RoundReal32ToInt32(B.Y);
	int32 MinY = RoundReal32ToInt32(A.Y);

	real32 Width = B.X - C.X;

	for (int32 Y = MinY; Y <= MaxY; ++Y)
	{
		int32 StartX = RoundReal32ToInt32(MinX);
		int32 StopX  = RoundReal32ToInt32(MaxX);
		
		for( int32 X = StartX; X <= StopX; ++X )
		{
			if((X < 0) || (Y < 0) || (X >= Buffer->Width) || (Y >= Buffer->Height))
			{
				continue;
			}

			uint8* PixelLocation = ((uint8*)Buffer->Memory + X * Buffer->BytesPerPixel +
				Y * Buffer->Pitch);
			uint32* Pixel = (uint32*) PixelLocation;
			*Pixel = PixelValue;
		
		}

		MaxX += ABslope;
		MinX += ACslope;

		if( (MaxX - MinX) > Width )
		{
			MinX = B.X;
			MaxX = C.X;
		}

	}
}

void DrawLineBresLow(game_offscreen_buffer* Buffer, int32 x0, int32 y0, int32 x1, int32 y1, v4 StartColor, v4 StopColor)
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

	real32 premul = 1 / (real32)(x1 - x0);
	if( dx != 0 )
	{
		premul  = 1 / (real32)(x1 - x0);
	}
	for (int x = x0; x <= x1; ++x)
	{
		real32 t = premul * (x - x0);
		v4 Color = LinearInterpolation(t, StartColor, StopColor);
		PutPixel(Buffer, x, y, Color.X, Color.Y, Color.Z);
		if (D > 0)
		{
			y = y + yi;
			D = D - 2 * dx;
		}
		D = D + 2 * dy;
	}
}


void DrawLineBresHigh(game_offscreen_buffer* Buffer, int32 x0, int32 y0, int32 x1, int32 y1, v4 StartColor, v4 StopColor)
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
	real32 premul = 0;
	if( dy != 0 )
	{
		premul  = 1 / (real32)(y0 - y1);
	}
	for (int y = y0; y <= y1; ++y)
	{
		real32 t = (real32)(y0 - y)* premul;
		v4 Color = LinearInterpolation(t, StartColor, StopColor);
		PutPixel(Buffer, x, y, Color.X, Color.Y, Color.Z);
		if (D > 0)
		{
			x = x + xi;
			D = D - 2 * dy;
		}

		D = D + 2 * dx;
	}
}

void DrawLineBres(game_offscreen_buffer* Buffer, int32 x0, int32 y0, int32 x1, int32 y1, v4 StartColor, v4 StopColor)
{

	if (Abs((real32)(y1 - y0)) < Abs((real32)(x1 - x0)))
	{
		if (x0 > x1) {
			DrawLineBresLow(Buffer, x1, y1, x0, y0, StartColor, StopColor);
		}
		else {
			DrawLineBresLow(Buffer, x0, y0, x1, y1, StartColor, StopColor);
		}
	}
	else {
		if (y0 > y1) {
			DrawLineBresHigh(Buffer, x1, y1, x0, y0, StartColor, StopColor);
		}
		else {
			DrawLineBresHigh(Buffer, x0, y0, x1, y1, StartColor, StopColor);
		}
	}
}


void fillBottomFlatTriangle(game_offscreen_buffer* Buffer, v2 A, v2 B, v2 C, v4 Ac, v4 Bc, v4 Cc)
{
	//     A
	//    /  \
	//   s1   s2
	//  /      \
	// B------- C

	Assert(A.Y >= B.Y);
	Assert(A.Y >= C.Y);

	real32 ABslope = 0;
	real32 ABdy = B.Y - A.Y;
	if (ABdy != 0)
	{
		ABslope = (B.X - A.X) / ABdy;
	}

	real32 ACslope = 0;
	real32 ACdy = C.Y - A.Y;
	if (ACdy != 0)
	{
		ACslope = (C.X - A.X) / ACdy;
	}

	real32 StartX = A.X;
	real32 StopX = A.X;

	real32 startT = 0;

	int32 startLineY = RoundReal32ToInt32(A.Y);
	int32 stopLineY = RoundReal32ToInt32(B.Y);

	real32 premulc = 1 / ((real32)(startLineY - stopLineY));
	for (int32 scanlineY = startLineY; scanlineY >= stopLineY; --scanlineY)
	{
		real32 yt = (real32)(startLineY - scanlineY) * premulc;

		v4 StartColor = LinearInterpolation(yt, Ac, Bc);
		v4 StopColor = LinearInterpolation(yt, Ac, Cc);

		DrawLineBres(Buffer, RoundReal32ToInt32(StartX), scanlineY, RoundReal32ToInt32(StopX), scanlineY, StartColor, StopColor);
		StartX -= ABslope;
		StopX -= ACslope;
		if (Abs(StartX - StopX) > Abs(C.X - B.X))
		{
			StartX = B.X;
			StopX = C.X;
		}
	}
}

void fillTopFlatTriangle(game_offscreen_buffer* Buffer, v2 A, v2 B, v2 C, v4 Ac, v4 Bc, v4 Cc)
{
	//C---B
	// \ /
	//	A

	Assert(A.Y <= B.Y);
	Assert(A.Y <= C.Y);

	real32 ABslope = 0;
	real32 ABdy = B.Y - A.Y;
	if (ABdy != 0)
	{
		ABslope = (B.X - A.X) / ABdy;
	}

	real32 ACslope = 0;
	real32 ACdy = C.Y - A.Y;
	if (ACdy != 0)
	{
		ACslope = (C.X - A.X) / ACdy;
	}

	real32 StartX = A.X;
	real32 StopX = A.X;

	int32 startLineY = RoundReal32ToInt32(A.Y);
	int32 stopLineY = RoundReal32ToInt32(B.Y);

	real32 premulc = 1 / ((real32)(stopLineY - startLineY));
	for (int32 scanlineY = startLineY; scanlineY <= stopLineY; ++scanlineY)
	{
		real32 yt = (real32)(scanlineY - startLineY) * premulc;

		v4 StartColor = LinearInterpolation(yt, Ac, Cc);
		v4 StopColor = LinearInterpolation(yt, Ac, Bc);

		DrawLineBres(Buffer, RoundReal32ToInt32(StartX), scanlineY, RoundReal32ToInt32(StopX), scanlineY, StartColor, StopColor);

		StartX += ABslope;
		StopX += ACslope;
		if (Abs(StopX - StartX) > Abs(C.X - B.X))
		{
			StartX = B.X;
			StopX = C.X;
		}
	}
}


bool SortTriangleVerticesAlongYAxis( v2* PointsIn, v2* PointsOut, v4* ColorIn, v4* ColorOut )
{
	int32 maxIdx = 0;
	int32 minIdx = 0;

	for (int32 i = 0; i < 3; ++i)
	{
		if (PointsIn[i].Y > PointsIn[maxIdx].Y)
		{
			maxIdx = i;
		}

		if (PointsIn[i].Y < PointsIn[minIdx].Y)
		{
			minIdx = i;
		}
	}

	// Points on a line along x axis
	if (maxIdx == minIdx) { return false; }

	for (int32 i = 0; i < 3; ++i)
	{
		if (i == maxIdx)
		{
			PointsOut[0] = PointsIn[i];
			ColorOut[0] = ColorIn[i];
		}else if (i == minIdx)
		{
			PointsOut[2] = PointsIn[i];
			ColorOut[2] = ColorIn[i];
		}else
		{
			PointsOut[1] = PointsIn[i];
			ColorOut[1] = ColorIn[i];
		}
	}

	return true;
}

void FillTriangle(game_offscreen_buffer* Buffer, v2 p0, v2 p1, v2 p2, v4 p1c = V4(1, 0, 0, 0), v4 p2c = V4(0, 1, 0, 0), v4 p3c = V4(0, 0, 1, 0))
{
	v2 P[3] = { p0,p1,p2 };
	v4 C[3] = { p1c, p2c, p3c };

	v2 SP[3] = {};
	v4 SC[3] = {};

	if( ! SortTriangleVerticesAlongYAxis(P,SP, C, SC) )
	{
		return;
	}

	if (SP[0].Y == SP[1].Y)
	{
		fillTopFlatTriangle(Buffer, SP[2], SP[0], SP[1], SC[2], SC[0], SC[1]);
	}
	else if (SP[1].Y == SP[2].Y)
	{
		fillBottomFlatTriangle(Buffer, SP[0], SP[1], SP[2], SC[0], SC[1], SC[2]);
	}
	else {

		v2 p4 = V2(SP[0].X + (SP[1].Y - SP[0].Y) / (SP[2].Y - SP[0].Y) * (SP[2].X - SP[0].X), SP[1].Y);

		real32 len = norm(SP[0] - p4) / norm(SP[0]-SP[2]);
		v4 c4 = LinearInterpolation(len, SC[0], SC[2] );

		if( SP[1].X < p4.X )
		{
			fillBottomFlatTriangle( Buffer, SP[0], SP[1], p4, SC[0], SC[1], c4 );
			fillTopFlatTriangle(    Buffer, SP[2], p4, SP[1],  SC[2], c4, SC[1] );
		}else{
			fillBottomFlatTriangle( Buffer, SP[0], p4, SP[1], SC[0], c4, SC[1] );
			fillTopFlatTriangle(    Buffer, SP[2], SP[1], p4, SC[2], SC[1], c4 );
		}

	}

}

void FillTriangleFlat(game_offscreen_buffer* Buffer, v2 p0, v2 p1, v2 p2, v4 Color)
{

	v2 P[3] = { p0,p1,p2 };
	v4 C[3] = { };

	v2 SP[3] = {};
	v4 SC[3] = {};


	if( ! SortTriangleVerticesAlongYAxis(P, SP, C, SC) )
	{
		return;
	}

	if (SP[0].Y == SP[1].Y)
	{
		fillTopFlatTriangleFlat(Buffer, SP[2], SP[0], SP[1], Color);
	}
	else if (SP[1].Y == SP[2].Y)
	{
		fillBottomFlatTriangleFlat(Buffer, SP[0], SP[1], SP[2], Color);
	}
	else {

		v2 p4 = V2(SP[0].X + (SP[1].Y - SP[0].Y) / (SP[2].Y - SP[0].Y) * (SP[2].X - SP[0].X), SP[1].Y);
		fillBottomFlatTriangleFlat( Buffer, SP[0], SP[1], p4, Color );
		fillTopFlatTriangleFlat(    Buffer, SP[2], p4, SP[1], Color );
	}
}

#if 0
void RenderScene(game_offscreen_buffer* Buffer, memory_arena* MemoryArena, real32 nrVert, v4* vert, v4* vertNorm, int32 nrTriangles, v3* triangleIdx, v4 AmbientProduct, v4 DiffuseProduct, v4 SpecularProduct,
	m4 T, m4 V, m4 P, m4 R)
{
	temporary_memory tmpMem = BeginTemporaryMemory(MemoryArena);
	vertex_data* vd = PushArray(MemoryArena, nrVert, vertex_data);

	m4 ModelView = V*T;
	for (int32 i = 0; i < nrVert; ++i)
	{
		vd[i] = VertexShader(vert[i], vertNorm[i], AmbientProduct, DiffuseProduct, SpecularProduct, T, V, P);
	}

	for (int32 i = 0; i < nrTriangles; ++i)
	{
		int32 vertIndex1 = triangleIdx[i].E[0];
		int32 vertIndex2 = triangleIdx[i].E[1];
		int32 vertIndex3 = triangleIdx[i].E[2];

		Rasterizer(Buffer, R, vd[vertIndex1], vd[vertIndex2], vd[vertIndex3]);
	}


	EndTemporaryMemory(tmpMem);
}
#endif

struct vertex_data
{
	v4 v;
	v4 n;
	v4 c;
};

v4 Flatshading(v4 fCenter, v4 fNormal, v4 LightPosition, m4 T, m4 V, v4 AmbientProduct, v4 DiffuseProduct, v4 SpecularProduct)
{
	// T = Model Matrix;
	// V = View Matrix;
	real32 Shininess = 10;
	real32 flippNormals = 0;

	m4 ModelView = V*T;
	v3 vpos = V3(ModelView*fCenter);
	v3 lpos = V3(V*LightPosition);
	v3 L = normalize(lpos - vpos);
	v3 E = normalize(-vpos);
	v3 H = normalize(L + E);
	v3 N = normalize(V3(ModelView*fNormal));

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


void RenderTriangle(game_offscreen_buffer* Buffer, v4 Vertex1, v4 Vertex2, v4 Vertex3, v4 fNormal, v4 LightPosition, v4 AmbientProduct, v4 DiffuseProduct, v4 SpecularProduct,
	m4 T, m4 V, m4 P, m4 R)
{
	#if 1
	local_persist real32 t = 0;
	v4 triangleCenter = (Vertex1 + Vertex2 +  Vertex3)/3.f;
	m4 ProjectionModelView = R * P * V * T;

	v4 Color = Flatshading(triangleCenter, fNormal, LightPosition, T, V, AmbientProduct, DiffuseProduct, SpecularProduct);
	FillTriangleFlat(Buffer, V2(ProjectionModelView*Vertex1), V2(ProjectionModelView*Vertex2), V2(ProjectionModelView*Vertex3), Color);
	#else
	vertex_data vd1 = BlinnPhong(Vertex1, fNormal, AmbientProduct, DiffuseProduct, SpecularProduct, T, V, P);
	vertex_data vd2 = BlinnPhong(Vertex2, fNormal, AmbientProduct, DiffuseProduct, SpecularProduct, T, V, P);
	vertex_data vd3 = BlinnPhong(Vertex3, fNormal, AmbientProduct, DiffuseProduct, SpecularProduct, T, V, P);

	FillTriangle(Buffer, V2(R*vd1.v), V2(R*vd2.v), V2(R*vd3.v), vd1.c, vd2.c, vd3.c);
	#endif
}


#endif HANDMADE_RENDER