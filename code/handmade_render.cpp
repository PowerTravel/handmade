#ifndef HANDMADE_RENDER
#define HANDMADE_RENDER

internal void
DrawRectangle(game_offscreen_buffer* Buffer, real32 RealMinX, real32 RealMinY, real32 Width, real32 Height, real32 R, real32 G, real32 B)
{
	int32 MinX = RoundReal32ToInt32(RealMinX);
	int32 MinY = RoundReal32ToInt32(RealMinY);
	int32 MaxX = RoundReal32ToInt32(RealMinX+Width);
	int32 MaxY = RoundReal32ToInt32(RealMinY+Height);

	if(MinX < 0)
	{
		MinX = 0;
	}
	if(MinY < 0)
	{
		MinY = 0;
	}
	if(MaxX > Buffer->Width)
	{
		MaxX =  Buffer->Width;
	}
	if(MaxY > Buffer->Height)
	{
		MaxY = Buffer->Height;
	}

	uint8* Row = ( (uint8*) Buffer->Memory  + MinX * Buffer->BytesPerPixel + 
											  MinY * Buffer->Pitch);
	for(int Y = MinY; Y<MaxY; ++Y)
	{
		uint32* Pixel = (uint32*)Row;
		for(int X = MinX; X<MaxX; ++X)
		{
			*Pixel++ = ((TruncateReal32ToInt32(R*255.f) << 16) |
					    (TruncateReal32ToInt32(G*255.f) << 8) 	|
					    (TruncateReal32ToInt32(B*255.f) << 0));
		}
		Row += Buffer->Pitch;
	}
}

void PutPixel(game_offscreen_buffer* Buffer, int32 X, int32 Y, real32 R, real32 G, real32 B)
{
	if( (X < 0) || (Y < 0) || (X >= Buffer->Width) || (Y >= Buffer->Height) )
	{
		return;
	}

	uint8* PixelLocation = ( (uint8*) Buffer->Memory  + X * Buffer->BytesPerPixel + 
											  Y * Buffer->Pitch);
	uint32* Pixel = (uint32*) PixelLocation;  
	*Pixel  = ( ( TruncateReal32ToInt32(R*255.f) << 16) |
			   ( TruncateReal32ToInt32(G*255.f) << 8)  |
			   ( TruncateReal32ToInt32(B*255.f) << 0) );
}

// makes the packing compact
#pragma pack(push, 1)
struct bmp_header{

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
	int32 MaxX = RoundReal32ToInt32(RealMinX+(real32)BitMap.Width);
	int32 MaxY = RoundReal32ToInt32(RealMinY+(real32)BitMap.Height);


	uint32 ClippingOffsetX = 0;
	uint32 ClippingOffsetY = 0;
	if(MinX < 0)
	{
		ClippingOffsetX = - MinX;
		MinX = 0;
	}
	if(MinY < 0)
	{
		ClippingOffsetY = - MinY;
		MinY = 0;
	}
	if(MaxX > Buffer->Width)
	{
		MaxX =  Buffer->Width;
	}
	if(MaxY > Buffer->Height)
	{
		MaxY = Buffer->Height;
	}


	uint32 BytesPerPixel = 4;

	uint8* SourceRow = (uint8*) BitMap.Pixels + (BitMap.Width*ClippingOffsetY + ClippingOffsetX)*BytesPerPixel;
	uint8* DestinationRow = (uint8*) ( (uint8*) Buffer->Memory + MinY* Buffer->Pitch + 
											 MinX * Buffer->BytesPerPixel );


	uint32 BitmapPitch = BitMap.Width * BytesPerPixel;

	for(int Y = MinY; Y < MaxY; ++Y)
	{
		uint32* SourcePixel = (uint32*) SourceRow;
		uint32* DesinationPixel =  (uint32*) DestinationRow; 
	 	for(int X = MinX; X < MaxX; ++X)
		{


			/// Note(Jakob): This loop is SLOW!! Tanking the fps

			uint32 Source = *SourcePixel;

			real32 Alpha = (real32) ((Source & 0xFF000000)>>24)/255.f;

			real32 Red =  (real32) (((Source & 0x00FF0000)>>16))/255.f;
			real32 Green =(real32) (((Source & 0x0000FF00)>>8) )/255.f;
			real32 Blue = (real32) (((Source & 0x000000FF)>>0) )/255.f;

			uint32 DestPix = *DesinationPixel;
			real32 DRed =(real32)    (((DestPix & 0x00FF0000)>>16))/255.f;
			real32 DGreen = (real32) (((DestPix & 0x0000FF00)>>8))/255.f;
			real32 DBlue =(real32)   (((DestPix & 0x000000FF)>>0))/255.f;

			Red = (1-Alpha) *DRed + Alpha*Red;
			Green = (1-Alpha)* DGreen + Alpha*Green;
			Blue = (1-Alpha) *DBlue + Alpha*Blue;

			int32 R = (int) (Red*255+0.5);
			int32 G = (int) (Green*255+0.5);
			int32 B = (int) (Blue*255+0.5);

			Source = (R << 16) | (G << 8) | (B << 0);

			*DesinationPixel = Source;

			DesinationPixel++;
			SourcePixel++;
	
		}
		DestinationRow += Buffer->Pitch;
		SourceRow += BitmapPitch;
	} 
}

void DrawCircle(game_offscreen_buffer* Buffer, real32 RealX0, real32 RealY0, real32 RealRadius)
{
	int32 x0     = RoundReal32ToInt32(RealX0);
	int32 y0     = RoundReal32ToInt32(RealY0);
	int32 radius = RoundReal32ToInt32(RealRadius);
    
	int32 x   = radius-1 ;
	int32 y   = 0;
	int32 dx  = 1;
	int32 dy  = 1;
	int32 err = dx - (radius << 1);


	while (x >= y)
	{
	    PutPixel(Buffer, x0 + x, y0 + y, 1,1,1);
	    PutPixel(Buffer, x0 + y, y0 + x, 1,1,1);
	    PutPixel(Buffer, x0 - y, y0 + x, 1,1,1);
	    PutPixel(Buffer, x0 - x, y0 + y, 1,1,1);
	    PutPixel(Buffer, x0 - x, y0 - y, 1,1,1);
	    PutPixel(Buffer, x0 - y, y0 - x, 1,1,1);
	    PutPixel(Buffer, x0 + y, y0 - x, 1,1,1);
	    PutPixel(Buffer, x0 + x, y0 - y, 1,1,1);

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

void RenderScene( game_offscreen_buffer* Buffer, v4 obj, m4 Camera, m4 OrtoProj, m4 RasterProj )
{
	local_persist real32 t = 0;
	t += (real32) 0.1;
	if( t >= (real32) (2.0*Pi32)){ t = (real32) (2.0*Pi32) - t; }

	DrawRectangle(Buffer, 0,0, (real32) Buffer->Width,   (real32)	 Buffer->Height,1,1,1);
	DrawRectangle(Buffer, 2,2, (real32) Buffer->Width-4, (real32)	 Buffer->Height-4,0,0,0);

	m4 cam = AffineInverse( Camera );
	obj = cam * obj;
	obj = OrtoProj*obj;
	obj = RasterProj*obj;

	real32 Radius = 20+10*Sin(t);
	DrawCircle( Buffer, obj.X, obj.Y, Radius );	
	
	

	//BlitBMP(Buffer, 0.0f, 0.0f, GameState->Map);

	//BlitBMP( Buffer, GameState->World->PlayerPosition.X-GameState->CursorBMP.Width/2, 
	//	             GameState->World->PlayerPosition.Y-GameState->CursorBMP.Width/2, 
	//	             GameState->CursorBMP);

}

#endif HANDMADE_RENDER