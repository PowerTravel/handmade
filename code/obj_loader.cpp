#include "obj_loader.h"
#include "string.h"
#include "utility_macros.h"
#include "data_containers.h"

v4 ParseNumbers(char* String)
{
	char* Start = str::FindFirstNotOf( " \t", String );
	char WordBuffer[STR_MAX_WORD_LENGTH];
	s32 CoordinateIdx = 0;
	v4 Result = V4(0,0,0,1);
	while( Start )
	{
		Assert(CoordinateIdx < 4);

		char* End = str::FindFirstOf( " \t", Start);

		size_t WordLength = ( End ) ? (End - Start) : str::StringLength(Start);

		Assert(WordLength < STR_MAX_WORD_LENGTH);

		Copy( WordLength, Start, WordBuffer );
		WordBuffer[WordLength] = '\0';

		
		r64 value = str::StringToReal64(WordBuffer);
		Result.E[CoordinateIdx++] = (r32) value;

		Start = (End) ? str::FindFirstNotOf(" \t", End) : End;
	}

	return Result;
}


struct obj_start_string
{
	u32 Enum;
	u32 StringLength;
	char String[16];
};

struct type_string_pair
{
	char* String;
	u32 Enum;
};

enum mtl_data_types
{
	MTL_EMPTY,
	// Material name statement:
	MTL_NEW_MATERIAL, 	// (newmtl) my_mtl
	
	// Material color and illumination statements:
	MTL_KA, 			// (Ka) 0.0435 0.0435 0.0435
	MTL_KD, 			// (Kd)0.1086 0.1086 0.1086
	MTL_KS, 			// (Ks) 0.0000 0.0000 0.0000
	MTL_TF, 			// (Tf) 0.9885 0.9885 0.9885
	MTL_ILLUMINATION, 	// (illum) 6
	MTL_D, 			 	// (d) -halo 0.6600
	MTL_NS, 			// (Ns) 10.0000
	MTL_SHARPNESS, 		// (sharpness) 60
	MTL_NI, 			// (Ni) 1.19713

	// Texture map statements:
	MTL_MAP_KA, 		// (map_Ka) -s 1 1 1 -o 0 0 0 -mm 0 1 chrome.mpc
 	MTL_MAP_KD, 		// (map_Kd) -s 1 1 1 -o 0 0 0 -mm 0 1 chrome.mpc
 	MTL_MAP_KS, 		// (map_Ks) -s 1 1 1 -o 0 0 0 -mm 0 1 chrome.mpc
 	MTL_MAP_NS, 		// (map_Tf) -s 1 1 1 -o 0 0 0 -mm 0 1 wisp.mps
 	MTL_MAP_D,  		// (map_d) -s 1 1 1 -o 0 0 0 -mm 0 1 wisp.mps
 	MTL_DISP,   		// (disp) -s 1 1 .5 wisp.mps
 	MTL_DECAL,  		// (decal) -s 1 1 1 -o 0 0 0 -mm 0 1 sand.mps
 	MTL_BUMP,   		// (bump) -s 1 1 1 -o 0 0 0 -bm 1 sand.mpb
 
 	// Reflection map statement:
 	MTL_REFLECTION_MAP 	// refl -type sphere -mm 0 1 clouds.mpc
};

type_string_pair GetMTLDataType( u32 StringLength, char* String )
{
	local_persist obj_start_string TypeMap[22] =
	{
		{ MTL_EMPTY, 		  1 , "#",		  },
		{ MTL_EMPTY, 		  2 , "\r\n"	  },
		{ MTL_EMPTY, 		  1 , "\n" 		  },
		{ MTL_NEW_MATERIAL,   7 , "newmtl "	  },
		{ MTL_KA, 			  3 , "Ka "		  },
		{ MTL_KD, 			  3 , "Kd "		  },
		{ MTL_KS, 			  3 , "Ks "		  },
		{ MTL_TF, 			  3 , "Tf "		  },
		{ MTL_ILLUMINATION,   6 , "illum "	  },
		{ MTL_D, 			  2 , "d "		  },
		{ MTL_NS, 			  3 , "Ns "		  },
		{ MTL_SHARPNESS, 	  10, "sharpness "},	
		{ MTL_NI, 			  3 , "Ni "		  },
		{ MTL_MAP_KA, 		  7 , "map_Ka "	  },
 		{ MTL_MAP_KD, 		  7 , "map_Kd "	  },
 		{ MTL_MAP_KS, 		  7 , "map_Ks "	  },
 		{ MTL_MAP_NS, 		  7 , "map_Tf "	  },
 		{ MTL_MAP_D,  		  6 , "map_d "	  },
 		{ MTL_DISP,   		  5 , "disp "	  },
 		{ MTL_DECAL,  		  6 , "decal "	  },
 		{ MTL_BUMP,   		  5 , "bump "	  },
 		{ MTL_REFLECTION_MAP, 5 , "refl "     }
    };

	type_string_pair Result = {};
	for( u32 TypeIDX = 0; TypeIDX < ArrayCount(TypeMap); ++TypeIDX )
	{
		obj_start_string& Type = TypeMap[TypeIDX];
		if(str::BeginsWith(Type.StringLength, Type.String, StringLength, String ))
		{	
				Result.Enum = Type.Enum;
				Result.String = str::FindFirstNotOf( " \t", String+Type.StringLength);
				return Result;
		}
	}

	Assert(0);
	return {};
}



enum obj_data_types
{
	// Misc
	OBJ_EMPTY,

	// Vertex data
	OBJ_GEOMETRIC_VERTICES, 		// (v)		1
	OBJ_TEXTURE_VERTICES, 			// (vt)
	OBJ_VERTEX_NORMALS, 			// (vn)
	OBJ_PARAMETER_SPACE_VERTICES, 	// (vp)
		
	// Free-form curve/surface attributes
	OBJ_CURVE_OR_SURFACE_TYPE, 	// (cstype)		5
	OBJ_DEGREE, 				// (deg)
	OBJ_BASIS_MATRIX, 			// (bmat)
	OBJ_STEP_SIZE, 				// (step)

	// Elements
	OBJ_POINT, 		// (p)						9
	OBJ_LINE, 		// (l)
	OBJ_FACE, 		// (f)
	OBJ_CURVE, 		// (curv)
	OBJ_2D_CURVE, 	// (curv2)
	OBJ_SURFACE, 	// (surf)

	// Free-form curve/surface body statements
	OBJ_PARAMETER_VALUES, 		// (parm)		15
	OBJ_OUTER_TRIMMING_LOOP, 	// (trim)
	OBJ_INNER_TRIMMING_LOOP, 	// (hole)
	OBJ_SPECIAL_CURVE, 			// (scrv)
	OBJ_SPECIAL_POINT, 			// (sp)
	OBJ_END_STATEMENT, 			// (end)

	// Connectivity between free-form surfaces
	OBJ_CONNECT, 			// (con)			21

	// Grouping
	OBJ_GROUP_NAME, 		// (g)
	OBJ_SMOOTHING_GROUP, 	// (s)
	OBJ_MERGING_GROUP, 		// (mg)
	OBJ_OBJECT_NAME, 		// (o) 				25

	// Display/render attributes
	OBJ_BEVEL_INTERPOLATION, 				// (bevel)
	OBJ_COLOR_INTERPOLATION , 				// (c_interp)
	OBJ_DISSOLVE_INTERPOLATION, 			// (d_interp)
	OBJ_LEVEL_OF_DETAIL, 					// (lod)
	OBJ_MATERIAL_NAME, 						// (usemtl)
	OBJ_MATERIAL_LIBRARY, 					// (mtllib)
	OBJ_SHADOW_CASTING, 					// (shadow_obj)
	OBJ_RAY_TRACING, 						// (trace_obj)
	OBJ_CURVE_APPROXIMATION_TECHNIQUE, 		// (ctech)
	OBJ_SURFACE_APPROXIMATION_TECHNIQUE, 	// (stech)
};


type_string_pair GetOBJDataType( u32 StringLength, char* String )
{
	local_persist obj_start_string TypeMap[38] =
	{
		{ OBJ_EMPTY, 								1 , "#",			},
		{ OBJ_EMPTY, 								2 , "\r\n"			},
		{ OBJ_EMPTY, 								1 , "\n" 			},
		{ OBJ_GEOMETRIC_VERTICES, 					2 , "v "			},
		{ OBJ_TEXTURE_VERTICES, 					3 , "vt "			},
		{ OBJ_VERTEX_NORMALS, 						3 , "vn "			},
		{ OBJ_PARAMETER_SPACE_VERTICES, 			3 , "vp "			},
		{ OBJ_CURVE_OR_SURFACE_TYPE, 				7 , "cstype "		},
		{ OBJ_DEGREE, 								4 , "deg "			},
		{ OBJ_BASIS_MATRIX, 						5 , "bmat "			},
		{ OBJ_STEP_SIZE, 							5 , "step "			},
		{ OBJ_POINT, 								2 , "p "			},
		{ OBJ_LINE, 								2 , "l "			},
		{ OBJ_FACE, 								2 , "f "			},
		{ OBJ_CURVE, 								5 , "curv "			},
		{ OBJ_2D_CURVE, 							6 , "curv2 "		},
		{ OBJ_SURFACE, 								5 , "surf "			},
		{ OBJ_PARAMETER_VALUES, 					5 , "parm "			},
		{ OBJ_OUTER_TRIMMING_LOOP, 					5 , "trim "			},
		{ OBJ_INNER_TRIMMING_LOOP, 					5 , "hole "			},
		{ OBJ_SPECIAL_CURVE, 						5 , "scrv "			},
		{ OBJ_SPECIAL_POINT, 						3 , "sp "			},
		{ OBJ_END_STATEMENT, 						4 , "end "			},
		{ OBJ_CONNECT, 								4 , "con "			},
		{ OBJ_GROUP_NAME, 							2 , "g "			},
		{ OBJ_SMOOTHING_GROUP, 						2 , "s "			},
		{ OBJ_MERGING_GROUP, 						3 , "mg "			},
		{ OBJ_OBJECT_NAME, 							2 , "o "			},
		{ OBJ_BEVEL_INTERPOLATION, 					6 , "bevel "		},
		{ OBJ_COLOR_INTERPOLATION,   				9 , "c_interp "		},
		{ OBJ_DISSOLVE_INTERPOLATION, 				9 , "d_interp "		},
		{ OBJ_LEVEL_OF_DETAIL, 						4 , "lod "			},
		{ OBJ_MATERIAL_NAME, 						7 , "usemtl "		},
		{ OBJ_MATERIAL_LIBRARY, 					7 , "mtllib "		},
		{ OBJ_SHADOW_CASTING, 						11, "shadow_obj "	},
		{ OBJ_RAY_TRACING, 							10, "trace_obj "	},
		{ OBJ_CURVE_APPROXIMATION_TECHNIQUE, 		6,  "ctech "		},
		{ OBJ_SURFACE_APPROXIMATION_TECHNIQUE, 		6,  "stech "		}
	};

	type_string_pair Result = {};
	for( u32 TypeIDX = 0; TypeIDX < ArrayCount(TypeMap); ++TypeIDX )
	{
		obj_start_string& Type = TypeMap[TypeIDX];
		if(str::BeginsWith(Type.StringLength, Type.String, StringLength, String ))
		{	
				Result.Enum = Type.Enum;
				Result.String = str::FindFirstNotOf( " \t", String+Type.StringLength);
				return Result;
		}
	}


	Assert(0);
	return {};
}

bool GetTrimmedLine( char* SrcLineStart, char* SrcLineEnd, u32* DstLength, char* DstLine )
{
	char* TrimEnd = SrcLineEnd-1;
	while( (TrimEnd > SrcLineStart) && ( (*TrimEnd == '\n') || (*TrimEnd == '\r') ) )
	{
		--TrimEnd;
	}
	++TrimEnd;

	char* TrimStart = SrcLineStart;
	while( (TrimStart < TrimEnd) && ((*TrimStart == ' ') || (*TrimStart == '\t')) )
	{
		++TrimStart;
	}

	SrcLineStart = TrimStart;
	size_t Length = TrimEnd - SrcLineStart;

	Assert(Length < STR_MAX_LINE_LENGTH);

	if(Length < 2)
	{
		return false;
	}
	
	Copy( Length, SrcLineStart, DstLine );
	DstLine[Length] = '\0';
	*DstLength = (u32) Length;
	return true;
}

void TriangulateLine(fifo_queue<u32>* Queue, fifo_queue<u32>* ResultQueue)
{
	// Triangulate:
	// We can assume the original .obj file has right handed 
	// orientation to their faecs.
	//  4--3             3      4--3
	//  |  | becomes    /|  and | /
	//  |  |           / |      |/
	//  1--2          1--2      1
	//  Make sure they're righthanded

	u32 FirstIndex  = Queue->Pop();
	u32 Index = 0;
	while(!Queue->IsEmpty())
	{
		if(Index == 0)
		{
			ResultQueue->Push(FirstIndex);
		}else{
			ResultQueue->Push(Queue->Pop());
		}
		++Index;
		Index = Index % 3;
	}
}

struct group_to_parse
{
	u32 GroupNameLength;
	char* GroupName;

	fifo_queue<u32> vi;
	fifo_queue<u32> ti;
	fifo_queue<u32> ni;

	u32 MaterialNameLength;
	char* MaterialName;
};

void ParseFaceLine(memory_arena* Arena, char* ParsedLine, group_to_parse* ActiveGroup )
{
	u32 NrVerticesInFace = str::GetWordCount( ParsedLine );
	Assert( NrVerticesInFace >= 3);

	fifo_queue<u32> vi = fifo_queue<u32>(Arena);
	fifo_queue<u32> ti = fifo_queue<u32>(Arena);
	fifo_queue<u32> ni = fifo_queue<u32>(Arena);

	char* Start = ParsedLine;
	char WordBuffer[STR_MAX_WORD_LENGTH];
	while( Start )
	{
		char* End = str::FindFirstOf( " \t", Start);

		size_t WordLength = ( End ) ? (End - Start) : str::StringLength(Start);

		Assert(WordLength < STR_MAX_WORD_LENGTH);

		Copy( WordLength, Start, WordBuffer );
		WordBuffer[WordLength] = '\0';


		char* StartNr = WordBuffer;
		char* EndNr   = 0;
		u32 IndexType = 0;

		while(StartNr)
		{
			EndNr 	= str::FindFirstOf("/", StartNr);
			if(EndNr)
		 	{
				*EndNr++ = '\0';
			}

			u32 Number = (u32) str::StringToReal64(StartNr)-1;
			Assert(Number>=0);
			switch(IndexType)
			{
				// Vertex Index
				case 0:
				{	
					vi.Push(Number);
				}break;
				// Texture Index
				case 1:
				{
					ti.Push(Number);
				}break;
				// Normal Index
				case 2:
				{
					ni.Push(Number);
				}break;

				default:
				{
					INVALID_CODE_PATH
				}break;
			}
			
			// A face specified with ____ looks like: "_____"
			//   1: Vertice, Texture Vertice and Normals: "10/11/12"
			//	 2: Only Vertice and Normals:             "10//12"
			//   3: Only Vertice and Texture Vertice:     "10/11"
			//   4: Only Vertice                          "10"
			// Case 1, 3 and 4 is handled by just increasing i sequentially.
			++IndexType;

			// Case 2 requires us to increase i two steps skipping case IndexType==1.
			if( EndNr && *EndNr == '/' ) 
			{
				++IndexType;
			}

			StartNr = str::FindFirstNotOf("/", EndNr);
			
		}

		Start = (End) ? str::FindFirstNotOf(" \t", End) : End;
	}

	// Make sure we have vertex, texture and normal indeces of the right size.
	Assert( vi.GetSize() == NrVerticesInFace);
	Assert((ti.GetSize() == NrVerticesInFace) || (ti.GetSize() == 0) );
	Assert((ni.GetSize() == NrVerticesInFace) || (ni.GetSize() == 0) );	

	TriangulateLine(&vi, &ActiveGroup->vi);

	if(!ti.IsEmpty())
	{
		TriangulateLine(&ti, &ActiveGroup->ti);
	}

	if(!ni.IsEmpty())
	{
		TriangulateLine(&ni, &ActiveGroup->ni);
	}
}

// makes the packing compact
#pragma pack(push, 1)
struct tga_header
{
	/*
	 * ID length:
	 * 0 if image file contains no color map
     * 1 if present
     * 2–127 reserved by Truevision
     * 128–255 available for developer use

     Note Jakob, Seems like if > 0 it specifies the length of the 
     		     image ID Field which comes after the header.
     */
	u8 IDLength; 

	/*
	 * ColorMapType:
     * 0 if image file contains no color map
     * 1 if present
     * 2–127 reserved by Truevision
     * 128–255 available for developer use
     */
	u8 ColorMapType;

	/*
	 * ImageType is enumerated in the lower three bits, with the fourth bit as a flag for RLE. Some possible values are:
     * 0 no image data is present
     * 1 uncompressed color-mapped image
     * 2 uncompressed true-color image
     * 3 uncompressed black-and-white (grayscale) image
     * 9 run-length encoded color-mapped image
     * 10 run-length encoded true-color image
     * 11 run-length encoded black-and-white (grayscale) image
	 */	
	u8 ImageType;

	// ColorMapSpecification
	u16 FirstEntryIndex;  // index of first color map entry that is included in the file
	u16 ColorMapLength;   // number of entries of the color map that are included in the file
	u8  ColorMapEntrySize; // number of bits per pixel

	// ImageSpecification
	u16 XOrigin; 			// absolute coordinate of lower-left corner for displays where origin is at the lower left
	u16 YOrigin; 			// as for X-origin.  0,0 emans first pixel is lower left. 
	                        // Data is stored in BGRA Format.
	u16 Width;	 			// width in pixels
	u16 Height;  			// height in pixels
	u8  PixelDepth; 		// Bits per pixel
	u8 	ImageDescriptor; 	// bits 3-0 give the alpha channel depth, bits 5-4 give direction
};
#pragma pack(pop)

bitmap* LoadTGA( thread_context* Thread, memory_arena* AssetArena,
				 debug_platform_read_entire_file* ReadEntireFile,
				 debug_platfrom_free_file_memory* FreeEntireFile,
				 char* FileName)
{

	debug_read_file_result ReadResult = ReadEntireFile(Thread, FileName);

	if( !ReadResult.ContentSize ) { return {}; }


	u32 HeaderSize = sizeof(tga_header);
	Assert(ReadResult.ContentSize > HeaderSize);

	tga_header& Header = *(tga_header*) ReadResult.Contents;

  	// Note Jakob: Seems like if > 0 it specifies the length of the 
    // 		       image ID Field which comes after the header.
    // 		       But specification says Header.IDLength == 1 means it
    // 			   just exists. So I will treat the field like a length between
    // 			   header and pixel data unless it's 1 and I want to investigate.
	Assert(Header.IDLength !=  1);
	Assert((Header.ImageType == 2) || (Header.ImageType == 3) ); // Uncompressed true color image
	Assert(Header.XOrigin == 0 );
	Assert(Header.YOrigin == 0 );

	u32 BytesPerPixel = Header.PixelDepth/8;
	u32 MinimumFileSize = (HeaderSize + Header.Width*Header.Height * BytesPerPixel );
	Assert(ReadResult.ContentSize >= MinimumFileSize);


	bitmap* Result = (bitmap*) PushStruct( AssetArena, bitmap );
	Result->Handle = 0;
	Result->Pixels = PushArray( AssetArena, Header.Width*Header.Height, u32);


	u8* ScanPtr = ((u8*) ReadResult.Contents) + (sizeof(tga_header) + Header.IDLength);
	u32* Dest = (u32*) Result->Pixels;

	for( s32 i = 0; i < Header.Width*Header.Height; ++i )
	{
		u32 Pixel = 0;
		u8 R = 0;
		u8 G = 0;
		u8 B = 0;
		u8 A = 0;
		if(BytesPerPixel == 4) 
		{
			B = *(ScanPtr + 0);
			G = *(ScanPtr + 1);
			R = *(ScanPtr + 2); 
			A = *(ScanPtr + 3);
		}
		else if(BytesPerPixel == 1){
			R = G = B = *ScanPtr;
			A = 255;
		}else if( BytesPerPixel == 3 ){
			B = *(ScanPtr+0);
			G = *(ScanPtr+1);
			R = *(ScanPtr+2);
			A = 255;
		}else{
			INVALID_CODE_PATH;
		}

		// BGRA Format
		//Pixel = (B << 24) | (G << 16) | (R << 8) | (A << 0);
		// ARGB Format
		Pixel = (A << 24) | (R << 16) | (G << 8) | (B << 0);

		ScanPtr += BytesPerPixel;

		*Dest++ = Pixel;
	}

	Result->Width = Header.Width;
	Result->Height = Header.Height;
	FreeEntireFile(Thread, ReadResult.Contents);

	return Result;
}

fifo_queue<char*> ExtractMTLSettings(memory_arena* Arena, u32 ArgumentCount, char* Setting, char* ArgumentSeparationTokens, char* SourceString, char* LeftOverString )
{
	u32 SettingLength = str::StringLength(Setting);
	u32 SourceStringLength = str::StringLength(SourceString);
	char* SettingStartPtr = 0;
	if( ( SettingStartPtr = str::Contains( SettingLength, Setting, SourceStringLength, SourceString )) == 0 ){ return {}; }
	
	char* SettingPtr = SettingStartPtr + SettingLength;

	fifo_queue<char*> Result = fifo_queue<char*>(Arena);

	for(u32 i = 0; i < ArgumentCount; ++i)
	{
		// Move to argument
		char* Start = str::FindFirstNotOf( ArgumentSeparationTokens, SettingPtr );
		
		// Should be pointing to argument
		Assert(Start);

		// Move to after the argument
		char* End = str::FindFirstOf( ArgumentSeparationTokens, Start );
		End = ( End ) ?  End : (SourceString + SourceStringLength);

		u64 Length = End - Start;
		char* PushSetting = (char*) PushArray(Arena, Length+1, char);
		str::CopyStrings( Length, Start, Length, PushSetting );
		Result.Push(PushSetting);

		SettingPtr = End;
	}

	u64 LengthBefore = SettingStartPtr - SourceString;
	u64 LengthBetween = SettingPtr - SettingStartPtr;
	Assert(SourceStringLength >= (LengthBefore + LengthBetween) );
	u64 LengthAfter = SourceStringLength - (LengthBefore + LengthBetween);
	str::CatStrings( LengthBefore, SourceString,
			LengthAfter, SettingPtr,
			LengthBefore + LengthAfter,	 LeftOverString );

	return Result;
}

void CreateNewFilePath(char* BaseFilePath, char* NewFileName, u32 NewFilePathLength, char* NewFilePath )
{

	char* Pos = str::FindLastOf( "\\", BaseFilePath );
	Assert( Pos )			
	++Pos;
	u32 BaseFolderLength = str::StringLength(BaseFilePath)-str::StringLength(Pos);


	char* Start = str::FindFirstNotOf( " \t", NewFileName);
	char* End = str::FindLastNotOf(" \t" , Start);
	++End;
	Assert(Start < End)
	
	u32 NewFileNameLength =(u32) (End - Start);
	Assert( (BaseFolderLength + NewFileNameLength) < STR_MAX_LINE_LENGTH );

	str::CatStrings(	BaseFolderLength,    BaseFilePath,
						NewFileNameLength,   Start,
						NewFilePathLength,   NewFilePath );
}



obj_mtl_data* ReadMTLFile(thread_context* Thread, game_state* aGameState,
				 debug_platform_read_entire_file* ReadEntireFile,
				 debug_platfrom_free_file_memory* FreeEntireFile,
				 char* FileName)
{
	debug_read_file_result ReadResult = ReadEntireFile(Thread, FileName);

	if( !ReadResult.ContentSize ) { return 0; }

	char LineBuffer[STR_MAX_LINE_LENGTH];

	memory_arena* AssetArena = &aGameState->AssetArena;
	memory_arena* TemporaryArena = &aGameState->TemporaryArena;

	temporary_memory TempMem = BeginTemporaryMemory(TemporaryArena);


	char* ScanPtr = ( char* ) ReadResult.Contents;
	char* FileEnd =  ( char* ) ReadResult.Contents + ReadResult.ContentSize;

	fifo_queue<mtl_material*> ParsedMaterialQueue = fifo_queue<mtl_material*>(TemporaryArena);
	mtl_material* ActieveMaterial = 0;

	while( ScanPtr < FileEnd )
	{
		char* ThisLine = ScanPtr;
		ScanPtr = str::FindFirstOf('\n', ThisLine);
		ScanPtr = (ScanPtr) ? (ScanPtr + 1) : FileEnd;

		u32 Length = 0;
		if( !GetTrimmedLine( ThisLine, ScanPtr, &Length, LineBuffer ))
		{
			continue;
		}

		type_string_pair DataType = GetMTLDataType( Length, LineBuffer );
		
		switch(DataType.Enum)
		{
			case MTL_EMPTY:
			{

			}break;
			case MTL_NEW_MATERIAL:
			{
				ActieveMaterial = (mtl_material*) PushStruct(AssetArena, mtl_material);
				ActieveMaterial->NameLength = str::StringLength( DataType.String );
				ActieveMaterial->Name = (char*) PushArray(AssetArena, ActieveMaterial->NameLength+1, char);
				str::CopyStrings( ActieveMaterial->NameLength, DataType.String, ActieveMaterial->NameLength+1, ActieveMaterial->Name );
				ParsedMaterialQueue.Push(ActieveMaterial);

			}break;
			case MTL_KA:
			{
				Assert( ! str::Contains( 8, "spectral", str::StringLength( DataType.String ), DataType.String ) );
				Assert( ! str::Contains( 3, "xyz", str::StringLength( DataType.String ), DataType.String ) );
				ActieveMaterial->Ka = (v4*) PushStruct( AssetArena, v4 );
				*ActieveMaterial->Ka = ParseNumbers(DataType.String);
				ActieveMaterial->Ka->W = 0;
			}break;
			case MTL_KD:
			{
				Assert( ! str::Contains( 8, "spectral", str::StringLength( DataType.String ), DataType.String ) );
				Assert( ! str::Contains( 3, "xyz", str::StringLength( DataType.String ), DataType.String ) );
				ActieveMaterial->Kd = (v4*) PushStruct( AssetArena, v4 );
				*ActieveMaterial->Kd = ParseNumbers(DataType.String);
				ActieveMaterial->Kd->W = 0;
			}break;
			case MTL_KS:
			{
				Assert( ! str::Contains( 8, "spectral", str::StringLength( DataType.String ), DataType.String ) );
				Assert( ! str::Contains( 3, "xyz", str::StringLength( DataType.String ), DataType.String ) );
				ActieveMaterial->Ks = (v4*) PushStruct( AssetArena, v4 );
				*ActieveMaterial->Ks = ParseNumbers(DataType.String);
				ActieveMaterial->Ks->W = 0;
			}break;
			case MTL_TF:
			{
				Assert( ! str::Contains( 8, "spectral", str::StringLength( DataType.String ), DataType.String ) );
				Assert( ! str::Contains( 3, "xyz", str::StringLength( DataType.String ), DataType.String ) );
				ActieveMaterial->Tf = (v4*) PushStruct( AssetArena, v4 );
				*ActieveMaterial->Tf = ParseNumbers(DataType.String);
				ActieveMaterial->Tf->W = 0;
			}break;
			case MTL_ILLUMINATION:
			{
				// Unimplemented
			}break;
			case MTL_D:
			{
				Assert(0);
			}break;
			case MTL_NS: //Specifies the specular exponent for the current material.  This defines the focus of the specular highlight.
			{
				ActieveMaterial->Ns = (r32*) PushStruct( AssetArena, r32 );
				*ActieveMaterial->Ns = (r32) str::StringToReal64(DataType.String);
			}break;
			case MTL_SHARPNESS:
			{
				Assert(0);
			}break;
			case MTL_NI: //Specifies the optical density for the surface. This is also known as index of refraction.
			{
				ActieveMaterial->Ni = (r32*) PushStruct( AssetArena, r32 );
				*ActieveMaterial->Ni = (r32) str::StringToReal64(DataType.String);
			}break;
			case MTL_MAP_KA:
			{
				Assert(0);
			}break;
			case MTL_MAP_KD:
			{
				// Unimplemented Settings 
				u32 SettingLength = str::StringLength( DataType.String );

				// -blendu on | off			
 				Assert( ! str::Contains( 8, "-blendu ", SettingLength, DataType.String ) );

 				// -blendv on | off			
 				Assert( ! str::Contains( 8, "-blendv ", SettingLength, DataType.String ) );

 				// -cc on | off				
 				Assert( ! str::Contains( 4, "-cc ",     SettingLength, DataType.String ) );

 				// -clamp on | off			
 				Assert( ! str::Contains( 7, "-clamp ",  SettingLength, DataType.String ) );
 				
 				// -mm base gain		
 				Assert( ! str::Contains( 4, "-mm ",     SettingLength, DataType.String ) );

 				// -o u v w
 				Assert( ! str::Contains( 3, "-o ",      SettingLength, DataType.String ) );

 				// -s u v w
 				Assert( ! str::Contains( 3, "-s ",      SettingLength, DataType.String ) );
 				
 				// -t u v w
				Assert( ! str::Contains( 3, "-t ",      SettingLength, DataType.String ) );

 				// -texres value
 				Assert( ! str::Contains( 8, "-texres ", SettingLength, DataType.String ) );



				char TGAFilePath[STR_MAX_LINE_LENGTH] = {};
				CreateNewFilePath( FileName, DataType.String, sizeof(TGAFilePath), TGAFilePath );
				
				ActieveMaterial->MapKd = LoadTGA( Thread, AssetArena,
													 ReadEntireFile, FreeEntireFile,
		 											 TGAFilePath);
			}break;
			case MTL_MAP_KS:
			{
				// Unimplemented Settings
				u32 SettingLength = str::StringLength( DataType.String );
				
				// -blendu on | off			
 				Assert( ! str::Contains( 8, "-blendu ", SettingLength, DataType.String ) );

 				// -blendv on | off			
 				Assert( ! str::Contains( 8, "-blendv ", SettingLength, DataType.String ) );

 				// -cc on | off				
 				Assert( ! str::Contains( 4, "-cc ",     SettingLength, DataType.String ) );

 				// -clamp on | off			
 				Assert( ! str::Contains( 7, "-clamp ",  SettingLength, DataType.String ) );
 				
 				// -mm base gain		
 				Assert( ! str::Contains( 4, "-mm ",     SettingLength, DataType.String ) );

 				// -o u v w
 				Assert( ! str::Contains( 3, "-o ",      SettingLength, DataType.String ) );

 				// -s u v w
 				Assert( ! str::Contains( 3, "-s ",      SettingLength, DataType.String ) );
 				
 				// -t u v w
				Assert( ! str::Contains( 3, "-t ",      SettingLength, DataType.String ) );

 				// -texres value
 				Assert( ! str::Contains( 8, "-texres ", SettingLength, DataType.String ) );


				char TGAFilePath[STR_MAX_LINE_LENGTH] = {};
				CreateNewFilePath( FileName, DataType.String, sizeof(TGAFilePath), TGAFilePath );
				
				ActieveMaterial->MapKs = LoadTGA( Thread, AssetArena,
													 ReadEntireFile, FreeEntireFile,
		 											 TGAFilePath);
			}break;
			case MTL_MAP_NS:
			{
				Assert(0);
			}break;
			case MTL_MAP_D:
			{
				Assert(0);
			}break;
			case MTL_DISP:
			{
				Assert(0);
			}break;
			case MTL_DECAL:
			{
				Assert(0);
			}break;
			case MTL_BUMP:
			{
				// Unimplemented Settings 
					
				u32 SettingLength = str::StringLength( DataType.String );

 				// -imfchan r | g | b | m | l | z
 				Assert( ! str::Contains( 9, "-imfchan ", SettingLength, DataType.String ) );

				// -blendu on | off			
 				Assert( ! str::Contains( 8, "-blendu ",  SettingLength, DataType.String ) );

 				// -blendv on | off			
 				Assert( ! str::Contains( 8, "-blendv ",  SettingLength, DataType.String ) );

 				// -cc on | off				
 				Assert( ! str::Contains( 4, "-cc ",      SettingLength, DataType.String ) );

 				// -clamp on | off			
 				Assert( ! str::Contains( 7, "-clamp ",   SettingLength, DataType.String ) );
 				
 				// -mm base gain		
 				Assert( ! str::Contains( 4, "-mm ",      SettingLength, DataType.String ) );

 				// -o u v w
 				Assert( ! str::Contains( 3, "-o ",       SettingLength, DataType.String ) );

 				// -s u v w
 				Assert( ! str::Contains( 3, "-s ",       SettingLength, DataType.String ) );
 				
 				// -t u v w
				Assert( ! str::Contains( 3, "-t ",       SettingLength, DataType.String ) );

 				// -texres value
 				Assert( ! str::Contains( 8, "-texres ",  SettingLength, DataType.String ) );

 				char* SettingStartPtr = 0;
 				// -bm mult, mult: [0,1]

				char LeftoverString[STR_MAX_LINE_LENGTH] = {};

				fifo_queue<char*> SettingQueue = ExtractMTLSettings( TemporaryArena, 1, "-bm ",  " \t",  DataType.String,  LeftoverString );
				Assert(SettingQueue.GetSize() == 1);
				
				ActieveMaterial->BumpMapBM = (r32) str::StringToReal64( SettingQueue.Pop() );

				
				char TGAFilePath[STR_MAX_LINE_LENGTH] = {};
				CreateNewFilePath( FileName, LeftoverString, sizeof(TGAFilePath), TGAFilePath );
				
				ActieveMaterial->BumpMap = LoadTGA( Thread, AssetArena,
													 ReadEntireFile, FreeEntireFile,
		 											 TGAFilePath);
			}break;
			case MTL_REFLECTION_MAP:
			{
				Assert(0);
			}break;
			default:
			{
				Assert(0);
			}
		}


	}

	mtl_material* Material = 0;
	obj_mtl_data* Result = (obj_mtl_data*) PushStruct(AssetArena, obj_mtl_data);
	Result->MaterialCount = ParsedMaterialQueue.GetSize();
	Result->Materials = (mtl_material*) PushArray(AssetArena, Result->MaterialCount, mtl_material);
	mtl_material* MaterialPosition = Result->Materials;
	while( (Material = ParsedMaterialQueue.Pop()) != 0 )
	{
		*MaterialPosition = *Material;
		++MaterialPosition;
	}

	EndTemporaryMemory(TempMem);
	FreeEntireFile(Thread, ReadResult.Contents);

	return Result;
}

void SetCenterOfMassAndBoundingBoxToOBJFile( obj_loaded_file* OBJFile, memory_arena* TemporaryArena )
{
	mesh_data* MeshData = OBJFile->MeshData;
	for( u32 GroupIndex = 0; GroupIndex < OBJFile->ObjectCount; ++GroupIndex )
	{
		temporary_memory TempMem = BeginTemporaryMemory(TemporaryArena);
		b32* IsVertexCounted = (b32*) PushArray(TemporaryArena,  MeshData->nv, u32 );

		obj_group& OBJGroup = OBJFile->Objects[GroupIndex];

		v3 CenterOfMass = V3(0,0,0); 
		r32 VertexCount = 0;
		mesh_indeces* Indeces = &OBJGroup.Indeces;

		u32 FirstVertexIndex = Indeces->vi[0];
		v3  FirstVertexOfGroup = MeshData->v[FirstVertexIndex];
		v3  Max = FirstVertexOfGroup;
		v3  Min = FirstVertexOfGroup;

		for( u32 Index=0; Index < Indeces->Count; ++Index )
		{
			u32 VertexIndex = Indeces->vi[Index];
			
			if( ! IsVertexCounted[VertexIndex] )
			{
				IsVertexCounted[VertexIndex] = true;
				
				v3 Vertex = MeshData->v[VertexIndex];

				Max.X = Max.X > Vertex.X ? Max.X : Vertex.X;
				Max.Y = Max.Y > Vertex.Y ? Max.Y : Vertex.Y;
				Max.Z = Max.Z > Vertex.Z ? Max.Z : Vertex.Z;

				Min.X = Min.X < Vertex.X ? Min.X : Vertex.X;
				Min.Y = Min.Y < Vertex.Y ? Min.Y : Vertex.Y;
				Min.Z = Min.Z < Vertex.Z ? Min.Z : Vertex.Z;

				CenterOfMass += Vertex;
				++VertexCount;
			}
		}

		OBJGroup.CenterOfMass = CenterOfMass / VertexCount;
		OBJGroup.BoundingBoxMin = Min - CenterOfMass;
		OBJGroup.BoundingBoxMax = Max - CenterOfMass;

		EndTemporaryMemory(TempMem);	
	}
}

void ScaleObjectToUnitCube( obj_loaded_file* Obj )
{
	mesh_data* MeshData = Obj->MeshData;

	v3 MaxAxis = V3(0,0,0);
	r32 MaxDistance = 0;
	for( u32 i = 0; i < MeshData->nv; ++i )
	{
		v3 Point = MeshData->v[i];

		r32 Distance = Point.X;
		Distance = GetAbsoluteMax( Distance, Point.Y );
		Distance = GetAbsoluteMax( Distance, Point.Z );

		if( Distance > MaxDistance )
		{
			MaxAxis = Point;
			MaxDistance = Distance;
		}
	}

	if(MaxDistance < 1)
	{
		MaxDistance = 1/MaxDistance;
	}

	for( u32 i = 0; i < MeshData->nv; ++i )
	{
		MeshData->v[i] = MeshData->v[i]*MaxDistance;
	}
}

obj_loaded_file* ReadOBJFile(thread_context* Thread, game_state* aGameState, 
			   debug_platform_read_entire_file* ReadEntireFile,
			   debug_platfrom_free_file_memory* FreeEntireFile,
			   char* FileName)
{
	debug_read_file_result ReadResult = ReadEntireFile(Thread, FileName);

	char LineBuffer[STR_MAX_LINE_LENGTH];
	
	if( !ReadResult.ContentSize ){ return {}; }

	memory_arena* TempArena = &aGameState->TemporaryArena;
	temporary_memory TempMem = BeginTemporaryMemory(TempArena);


	fifo_queue<group_to_parse*> GroupToParse = fifo_queue<group_to_parse*>(TempArena);
	group_to_parse* DefaultGroup = (group_to_parse*) PushStruct(TempArena, group_to_parse);
	DefaultGroup->vi = fifo_queue<u32>(TempArena);
	DefaultGroup->ti = fifo_queue<u32>(TempArena);
	DefaultGroup->ni = fifo_queue<u32>(TempArena);
	
	group_to_parse* ActiveGroup = DefaultGroup;

	fifo_queue<v3> VerticeBuffer 		= fifo_queue<v3>( TempArena );
	fifo_queue<v3> VerticeNormalBuffer 	= fifo_queue<v3>( TempArena );
	fifo_queue<v2> TextureVerticeBuffer = fifo_queue<v2>( TempArena );

	obj_mtl_data* MaterialFile = 0;

	char* ScanPtr = ( char* ) ReadResult.Contents;
	char* FileEnd =  ( char* ) ReadResult.Contents + ReadResult.ContentSize;

	while( ScanPtr < FileEnd )
	{
		char* ThisLine = ScanPtr;
		ScanPtr = str::FindFirstOf('\n', ThisLine);
		ScanPtr = (ScanPtr) ? (ScanPtr + 1) : FileEnd;

		u32 Length = 0;
		if( !GetTrimmedLine( ThisLine, ScanPtr, &Length, LineBuffer ))
		{
			continue;
		}

		type_string_pair DataType = GetOBJDataType( Length, LineBuffer );
		
		switch( DataType.Enum )
		{
			case obj_data_types::OBJ_EMPTY:
			{
				continue;
			}break;
		
			// Vertices: v x y z
			case obj_data_types::OBJ_GEOMETRIC_VERTICES:
			{
				v3 Vert = V3(ParseNumbers(DataType.String));
				VerticeBuffer.Push(Vert);

			}break;

			// Vertex Normals: vn i j k
			case obj_data_types::OBJ_VERTEX_NORMALS:
			{
				v3 VertNorm = V3(ParseNumbers(DataType.String));
				VerticeNormalBuffer.Push(VertNorm);
			}break;

			// Vertex Textures: vt u v
			case obj_data_types::OBJ_TEXTURE_VERTICES:
			{
				v2 TextureVertice = V2( ParseNumbers(DataType.String) );
				TextureVerticeBuffer.Push( TextureVertice );
			}break;

			// Faces: f v1[/vt1][/vn1] v2[/vt2][/vn2] v3[/vt3][/vn3] ...
			case obj_data_types::OBJ_FACE:
			{
				ParseFaceLine( TempArena, DataType.String, ActiveGroup );
			}break;

			// Group: name1 name2 ....
			case obj_data_types::OBJ_GROUP_NAME:
			{
				u32 ObjectNameLength = str::StringLength( DataType.String );
				if( !str::Contains(7, "default", ObjectNameLength, DataType.String ) )
				{
					ActiveGroup = (group_to_parse*) PushStruct(TempArena, group_to_parse);
					
					ActiveGroup->GroupNameLength = ObjectNameLength;
					ActiveGroup->GroupName = (char*) PushArray(TempArena, ObjectNameLength+1, char );
					str::CopyStrings(ObjectNameLength,  DataType.String, ObjectNameLength, ActiveGroup->GroupName );

					ActiveGroup->vi = fifo_queue<u32>(TempArena);
					ActiveGroup->ti = fifo_queue<u32>(TempArena);
					ActiveGroup->ni = fifo_queue<u32>(TempArena);

					GroupToParse.Push( ActiveGroup );
				}
			}break;

			case obj_data_types::OBJ_MATERIAL_LIBRARY:
			{
				char MTLFileName[STR_MAX_LINE_LENGTH] = {};

				CreateNewFilePath( FileName, DataType.String, sizeof(MTLFileName), MTLFileName );
				
				if(!MaterialFile)
				{
					// Stores materials to Asset Arena
					MaterialFile =  ReadMTLFile(Thread, aGameState, ReadEntireFile, FreeEntireFile, MTLFileName);
				}


			} break;
			case obj_data_types::OBJ_OBJECT_NAME:
			{
				// Unimplemented
				char* kek = DataType.String;
			} break;
			case obj_data_types::OBJ_MATERIAL_NAME:
			{
				u32 MaterialNameLength = str::StringLength( DataType.String );

				ActiveGroup->MaterialNameLength = MaterialNameLength;
				ActiveGroup->MaterialName = (char*) PushArray(TempArena, MaterialNameLength+1, char );
				str::CopyStrings(MaterialNameLength,  DataType.String, MaterialNameLength, ActiveGroup->MaterialName );
				
			} break;
			case obj_data_types::OBJ_SMOOTHING_GROUP:
			{
				// Unimplemented
			} break;
			
			default:
			{
				Assert(0);

			} break;
		}
		
	}

	if(ActiveGroup == DefaultGroup)
	{
		GroupToParse.Push( DefaultGroup );
	}

	FreeEntireFile(Thread, ReadResult.Contents);

	memory_arena* AssetArena = &aGameState->AssetArena;

	obj_loaded_file* Result = (obj_loaded_file*) PushStruct( AssetArena, obj_loaded_file );

	mesh_data* MeshData = (mesh_data*) PushStruct( AssetArena, mesh_data );

	MeshData->nv = VerticeBuffer.GetSize();
	MeshData->v = (v3*) PushArray(AssetArena, MeshData->nv, v3);		
	for( u32 Index = 0; Index < MeshData->nv; ++Index )
	{
		MeshData->v[Index] = VerticeBuffer.Pop();
	}
	Assert(VerticeBuffer.IsEmpty());


	MeshData->nvn = VerticeNormalBuffer.GetSize();
	MeshData->vn = (v3*) PushArray(AssetArena, MeshData->nvn, v3);
	for( u32 Index = 0; Index < MeshData->nvn; ++Index )
	{
		MeshData->vn[Index] = VerticeNormalBuffer.Pop();
	}
	Assert(VerticeNormalBuffer.IsEmpty());


	MeshData->nvt = TextureVerticeBuffer.GetSize();
	MeshData->vt = (v2*) PushArray(AssetArena, MeshData->nvt, v2);
	for( u32 Index = 0; Index < MeshData->nvt; ++Index )
	{
		MeshData->vt[Index] = TextureVerticeBuffer.Pop();
	}
	Assert(TextureVerticeBuffer.IsEmpty());


	Result->ObjectCount = GroupToParse.GetSize();
	Result->Objects = (obj_group*) PushArray(AssetArena, Result->ObjectCount, obj_group);
	Result->MeshData    = MeshData;

	obj_group* NewGroup = Result->Objects;
	while( !GroupToParse.IsEmpty() )
	{
		group_to_parse* ParsedGroup = GroupToParse.Pop();

		NewGroup->GroupNameLength = ParsedGroup->GroupNameLength;
		NewGroup->GroupName = (char*) PushArray( AssetArena, ParsedGroup->GroupNameLength+1, char );
		str::CopyStrings( ParsedGroup->GroupNameLength, ParsedGroup->GroupName, NewGroup->GroupNameLength, NewGroup->GroupName );

		if(MaterialFile && ParsedGroup->MaterialName )
		{
			for( u32 MaterialIndex = 0; MaterialIndex < MaterialFile->MaterialCount; ++MaterialIndex)
			{
				mtl_material* mtl =  &MaterialFile->Materials[MaterialIndex];
				if( str::Equals(ParsedGroup->MaterialName, mtl->Name ) )
				{
					NewGroup->Material = mtl;
				}
			}
		}

		mesh_indeces* Indeces = &NewGroup->Indeces;
		Indeces->Count = ParsedGroup->vi.GetSize();

		Indeces->vi    = (u32*) PushArray( AssetArena, Indeces->Count, u32 );
		for( u32 i = 0; i < Indeces->Count; ++i )
		{
			Indeces->vi[i] = ParsedGroup->vi.Pop();
		}

		if(ParsedGroup->ti.GetSize())
		{
			Indeces->ti    = (u32*) PushArray( AssetArena, Indeces->Count, u32 );
			for( u32 i = 0; i < Indeces->Count; ++i )
			{
				Indeces->ti[i] = ParsedGroup->ti.Pop();
			}
		}

		if(ParsedGroup->ni.GetSize())
		{
			Indeces->ni    = (u32*) PushArray( AssetArena, Indeces->Count, u32 );
			for( u32 i = 0; i < Indeces->Count; ++i )
			{
				Indeces->ni[i] = ParsedGroup->ni.Pop();	
			}
		}

		NewGroup++;
	}

	Assert( GroupToParse.IsEmpty() );

	SetCenterOfMassAndBoundingBoxToOBJFile(Result, TempArena);

	EndTemporaryMemory(TempMem);

	CheckArena(TempArena);

	return Result;
}

entity* CreateEntityFromOBJGroup( world* World, obj_group* OBJGrp, mesh_data* MeshData )
{
	entity* Entity = NewEntity( World );
	NewComponents( World, Entity,  COMPONENT_TYPE_MESH | COMPONENT_TYPE_SPATIAL );

	Entity->MeshComponent->Indeces = OBJGrp->Indeces;
	Entity->MeshComponent->Data = MeshData;

	v3 Size = OBJGrp->BoundingBoxMax - OBJGrp->BoundingBoxMin;
	Entity->SpatialComponent->Position = OBJGrp->CenterOfMass;
	Entity->SpatialComponent->Velocity = V3(0,0,0);
	Entity->SpatialComponent->RotationAngle = 0;
	Entity->SpatialComponent->RotationAxis = V3(0,0,1);
	Entity->SpatialComponent->IsDynamic = true;
	Entity->SpatialComponent->Width  = Size.X;
	Entity->SpatialComponent->Height = Size.Y;
	Entity->SpatialComponent->Depth  = Size.Z;

	if( OBJGrp->Material )
	{
		NewComponents( World, Entity,  COMPONENT_TYPE_SURFACE );
		component_surface* Surface = Entity->SurfaceComponent;
		Surface->Material = PushStruct( &World->Arena, material);

		if( OBJGrp->Material->Ka || OBJGrp->Material->Kd || OBJGrp->Material->Ks || OBJGrp->Material->Ns )
		{
			if(OBJGrp->Material->Ka)
			{
				Surface->Material->AmbientColor  = *OBJGrp->Material->Ka;
			}else{
				Surface->Material->AmbientColor = V4(1,1,1,1);
			}

			if(OBJGrp->Material->Kd)
			{
				Surface->Material->DiffuseColor  = *OBJGrp->Material->Kd;
			}else{
				Surface->Material->DiffuseColor = V4(1,1,1,1);
			}

			if(OBJGrp->Material->Ks)
			{
				Surface->Material->SpecularColor  = *OBJGrp->Material->Ks;
			}else{
				Surface->Material->SpecularColor = V4(1,1,1,1);
			}

			if( OBJGrp->Material->Ns )
			{
				Surface->Material->Shininess  	 = *OBJGrp->Material->Ns;
			}else{
				Surface->Material->Shininess 	 = 1;
			}
		}

		Surface->Material->DiffuseMap  = OBJGrp->Material->MapKd;
	}

	return Entity;
}

void CreateEntitiesFromOBJFile( world* World, obj_loaded_file* ObjFile )
{
	for( u32  ObjectIndex = 0; ObjectIndex < ObjFile->ObjectCount; ++ObjectIndex )
	{
		obj_group* Grp = &ObjFile->Objects[ObjectIndex];
		CreateEntityFromOBJGroup(World, Grp, ObjFile->MeshData );
	}
}

#if 0
#include "mesh.h"


ordered_mesh* CreateOrderedMesh( memory_arena* Arena, obj_loaded_file*  LoadedObject )
{
	ordered_mesh* Mesh = PushArray(Arena, LoadedObject->ObjectCount, ordered_mesh );
	return 0;
}

#endif