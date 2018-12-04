

struct mtl_material
{
	u32 NameLength;
	char* Name;

	v4 *Kd;
	v4 *Ka;
	v4 *Tf;
	v4 *Ks; 
	r32 *Ni;
	r32 *Ns;

	r32 BumpMapBM;
	bitmap* BumpMap;
	bitmap* MapKd;
	bitmap* MapKs;
};

struct obj_mtl_data
{
	u32 MaterialCount;
	mtl_material* Materials;
};

struct obj_group
{
	u32 GroupNameLength;
	char* GroupName;

	u32   FaceCount;  // Nr Faces
	face* Faces; 	  // Faces

	mtl_material* Material;
};


v4 ParseNumbers(char* String)
{
	char* Start = str::FindFirstNotOf( " \t", String );
	char WordBuffer[OBJ_MAX_WORD_LENGTH];
	s32 CoordinateIdx = 0;
	v4 Result = V4(0,0,0,1);
	while( Start )
	{
		Assert(CoordinateIdx < 4);

		char* End = str::FindFirstOf( " \t", Start);

		size_t WordLength = ( End ) ? (End - Start) : str::StringLength(Start);

		Assert(WordLength < OBJ_MAX_WORD_LENGTH);

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

	Assert(Length < OBJ_MAX_LINE_LENGTH);

	if(Length < 2)
	{
		return false;
	}
	
	Copy( Length, SrcLineStart, DstLine );
	DstLine[Length] = '\0';
	*DstLength = (u32) Length;
	return true;
}


face* ParseFaceLine(memory_arena* Arena, char* ParsedLine )
{
	face* Result = (face*) PushStruct(Arena, face);
	Result->nv = str::GetWordCount( ParsedLine );
	Assert( Result->nv >= 3);

	u32 VertIdx = 0;
	char* Start = ParsedLine;
	char WordBuffer[OBJ_MAX_WORD_LENGTH];
	while( Start )
	{
		char* End = str::FindFirstOf( " \t", Start);

		size_t WordLength = ( End ) ? (End - Start) : str::StringLength(Start);

		Assert(WordLength < OBJ_MAX_WORD_LENGTH);

		Copy( WordLength, Start, WordBuffer );
		WordBuffer[WordLength] = '\0';


		char* StartNr = WordBuffer;
		char* EndNr = 0;
		u32 i =0;
		while( StartNr )
		{
			EndNr 	= str::FindFirstOf("/", StartNr);
			if( EndNr )
		 	{
				*EndNr++ = '\0';
			}

			u32 nr = (u32) str::StringToReal64(StartNr)-1;
			Assert(nr>=0);
			Assert(VertIdx < Result->nv);
			switch(i)
			{
				case 0:
				{	
					if( !Result->vi )
					{
						Result->vi = (u32*) PushArray(Arena, Result->nv, u32); 
					}

					Result->vi[VertIdx] = nr;
				}break;

				case 1:
				{
					if( !Result->ti )
					{
						Result->ti = (u32*) PushArray(Arena, Result->nv, u32); 
					}

					Result->ti[VertIdx] = nr;
				}break;

				case 2:
				{
					if( !Result->ni )
					{
						Result->ni  = (u32*) PushArray(Arena, Result->nv, u32); 
					}
					Result->ni[VertIdx] = nr;
				}break;

			}

			++i;

			StartNr = str::FindFirstNotOf("/", EndNr);
			
		}

		++VertIdx;

		Start = (End) ? str::FindFirstNotOf(" \t", End) : End;
	}
	return Result;
}

bitmap* LoadTGA( thread_context* Thread, memory_arena* AssetArena,
				 debug_platform_read_entire_file* ReadEntireFile,
				 debug_platfrom_free_file_memory* FreeEntireFile,
				 char* FileName)
{

	debug_read_file_result ReadResult = ReadEntireFile(Thread, FileName);

	if( !ReadResult.ContentSize ) { return {}; }

	Assert(ReadResult.ContentSize > 18);

	u8* ScanPtr = (u8*) ReadResult.Contents;

	u8 ImageIdFieldSize = *ScanPtr++;
	Assert(ImageIdFieldSize == 0);  // No Image ID Field
	Assert(*ScanPtr++ == 0); 		// No Color Map included
	Assert(*ScanPtr++ == 2); 		// Uncompressed true color image

	for( u32 i = 0; i < 5; ++i )
	{
		Assert(*ScanPtr++ == 0); // Color Map specification
	}

	u32 XOrigin = *( (u16*) ScanPtr);
	ScanPtr += 2;
	u32 YOrigin = *( (u16*) ScanPtr);
	ScanPtr += 2;
	u32 Width = *( (u16*) ScanPtr);
	ScanPtr += 2;
	u32 Height = *( (u16*) ScanPtr);
	ScanPtr += 2;

	u8 PixelDepth = *ScanPtr++;

	u8 ImageDescriptor = *ScanPtr++;

	Assert( (ImageDescriptor & 0x30) == 0x00 );

	Assert(ReadResult.ContentSize > ( 18 + Width*Height*( PixelDepth/8 )));

	bitmap* Result = (bitmap*) PushStruct( AssetArena, bitmap );
	Result->Pixels = PushArray( AssetArena, Width*Height, u32);

	u32* Dest = (u32*) Result->Pixels;

	u32 BytesPerPixel = PixelDepth/8;
	for( u32 i = 0; i < Width*Height; ++i )
	{
		u32 Pixel = 0;
		for(u32 j = 0; j<BytesPerPixel; ++j)
		{
			u8 ColorValue = *(ScanPtr+j);
			Pixel =  Pixel | ( ColorValue << (8*j) );
		}
		if(BytesPerPixel != 4)
		{
			// Set Alpha to 255;
			Pixel =  Pixel | ( 0xff << 24 );	
		}

		ScanPtr += BytesPerPixel;

		*Dest++ = Pixel;
	}

	Result->Width = Width;
	Result->Height = Height;
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
	Assert( (BaseFolderLength + NewFileNameLength) < OBJ_MAX_LINE_LENGTH );

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

	char LineBuffer[OBJ_MAX_LINE_LENGTH];

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



				char TGAFilePath[OBJ_MAX_LINE_LENGTH] = {};
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


				char TGAFilePath[OBJ_MAX_LINE_LENGTH] = {};
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

				char LeftoverString[OBJ_MAX_LINE_LENGTH] = {};

				fifo_queue<char*> SettingQueue = ExtractMTLSettings( TemporaryArena, 1, "-bm ",  " \t",  DataType.String,  LeftoverString );
				Assert(SettingQueue.GetSize() == 1);
				
				ActieveMaterial->BumpMapBM = (r32) str::StringToReal64( SettingQueue.Pop() );

				
				char TGAFilePath[OBJ_MAX_LINE_LENGTH] = {};
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

struct group_to_parse
{
	u32 GroupNameLength;
	char* GroupName;

	fifo_queue<face*> Faces;
	u32 TriangleCount;

	u32 MaterialNameLength;
	char* MaterialName;
};


void CenterObjectAtOrigin( loaded_obj_file* Obj )
{
	v4 CenterOfMass = V4(0,0,0,0);
	mesh_data* MeshData = Obj->MeshData;
	for(u32 i = 0; i < MeshData->nv; ++i)
	{
		CenterOfMass += MeshData->v[i];	
	}
	
	CenterOfMass = CenterOfMass/(r32) Obj->MeshData->nv;
	CenterOfMass.W = 0;

	for(u32 i = 0; i < MeshData->nv; ++i)
	{
		MeshData->v[i] -= CenterOfMass;	
	}
}

void ScaleObjectToUnitQube( loaded_obj_file* Obj )
{
	mesh_data* MeshData = Obj->MeshData;

	v4 MaxAxis = V4(0,0,0,0);
	r32 MaxDistance = 0;
	for( u32 i = 0; i < MeshData->nv; ++i )
	{
		r32 Distance = Norm( V3(MeshData->v[i]) ); 
		if( Distance > MaxDistance )
		{
			MaxAxis = MeshData->v[i];
			MaxDistance = Distance;
		}
	}

	if(MaxDistance > 1)
	{
		MaxDistance = 1/MaxDistance;
	}

	for( u32 i = 0; i < MeshData->nv; ++i )
	{
		MeshData->v[i] = MeshData->v[i]*MaxDistance; 
		MeshData->v[i].W = 1;
	}
}

loaded_obj_file* ReadOBJFile(thread_context* Thread, game_state* aGameState, 
			   debug_platform_read_entire_file* ReadEntireFile,
			   debug_platfrom_free_file_memory* FreeEntireFile,
			   char* FileName)
{
	debug_read_file_result ReadResult = ReadEntireFile(Thread, FileName);

	char LineBuffer[OBJ_MAX_LINE_LENGTH];
	
	if( !ReadResult.ContentSize ){ return {}; }

	memory_arena* TempArena = &aGameState->TemporaryArena;
	temporary_memory TempMem = BeginTemporaryMemory(TempArena);


	fifo_queue<group_to_parse*> GroupToParse = fifo_queue<group_to_parse*>(TempArena);
	group_to_parse* DefaultGroup = (group_to_parse*) PushStruct(TempArena, group_to_parse);
	DefaultGroup->Faces = fifo_queue<face*>(TempArena);
	
	group_to_parse* ActiveGroup = DefaultGroup;

	fifo_queue<v4> VerticeBuffer 		= fifo_queue<v4>( TempArena );
	fifo_queue<v4> VerticeNormalBuffer 	= fifo_queue<v4>( TempArena );
	fifo_queue<v3> TextureVerticeBuffer = fifo_queue<v3>( TempArena );

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
		
			// Vertices: v x y z [w=1]
			case obj_data_types::OBJ_GEOMETRIC_VERTICES:
			{
				v4 Vert = ParseNumbers(DataType.String);
				Vert.W = 1;
				VerticeBuffer.Push(Vert);

			}break;

			// Vertex Normals: vn i j k
			case obj_data_types::OBJ_VERTEX_NORMALS:
			{
				v4 VertNorm = ParseNumbers(DataType.String);
				VertNorm.W = 0;
				VerticeNormalBuffer.Push(VertNorm);
			}break;

			// Vertex Textures: vt u v [w=0]
			case obj_data_types::OBJ_TEXTURE_VERTICES:
			{
				v3 TextureVertice = V3( ParseNumbers(DataType.String) );
				TextureVerticeBuffer.Push( TextureVertice );
			}break;

			// Faces: f v1[/vt1][/vn1] v2[/vt2][/vn2] v3[/vt3][/vn3] ...
			case obj_data_types::OBJ_FACE:
			{
				face* Face = ParseFaceLine( TempArena, DataType.String );
				ActiveGroup->Faces.Push(Face);
				Assert( Face->nv >= 3 );
				ActiveGroup->TriangleCount += Face->nv - 2;

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

					ActiveGroup->Faces = fifo_queue<face*>(TempArena);

					GroupToParse.Push( ActiveGroup );
				}
			}break;

			case obj_data_types::OBJ_MATERIAL_LIBRARY:
			{
				char MTLFileName[OBJ_MAX_LINE_LENGTH] = {};

				CreateNewFilePath( FileName, DataType.String, sizeof(MTLFileName), MTLFileName );
				
				if(!MaterialFile)
				{
					// Stores materials to Asset Arena
					MaterialFile = ReadMTLFile( Thread, aGameState,
												ReadEntireFile, FreeEntireFile,
		 										MTLFileName);
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

	loaded_obj_file* Result = (loaded_obj_file*) PushStruct( AssetArena, loaded_obj_file );

	mesh_data* MeshData = (mesh_data*) PushStruct( AssetArena, mesh_data );

	MeshData->nv = VerticeBuffer.GetSize();
	MeshData->v = (v4*) PushArray(AssetArena, MeshData->nv, v4);		
	for( u32 Index = 0; Index < MeshData->nv; ++Index )
	{
		MeshData->v[Index] = VerticeBuffer.Pop();
	}
	Assert(VerticeBuffer.IsEmpty());


	MeshData->nvn = VerticeNormalBuffer.GetSize();
	MeshData->vn = (v4*) PushArray(AssetArena, MeshData->nvn, v4);
	for( u32 Index = 0; Index < MeshData->nvn; ++Index )
	{
		MeshData->vn[Index] = VerticeNormalBuffer.Pop();
	}
	Assert(VerticeNormalBuffer.IsEmpty());


	MeshData->nvt = TextureVerticeBuffer.GetSize();
	MeshData->vt = (v3*) PushArray(AssetArena, MeshData->nvt, v3);
	for( u32 Index = 0; Index < MeshData->nvt; ++Index )
	{
		MeshData->vt[Index] = TextureVerticeBuffer.Pop();
	}
	Assert(TextureVerticeBuffer.IsEmpty());


	u32 ObjectCount = GroupToParse.GetSize();
	obj_group* Objects = (obj_group*) PushArray(AssetArena, ObjectCount, obj_group);
	for( u32 GroupIndex = 0; GroupIndex < ObjectCount; ++GroupIndex )
	{
		group_to_parse* ParsedGroup = GroupToParse.Pop();
		obj_group* NewGroup = &Objects[GroupIndex];

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

		NewGroup->FaceCount = ParsedGroup->TriangleCount;
		NewGroup->Faces = (face*) PushArray( AssetArena, ParsedGroup->TriangleCount, face );

		fifo_queue<face*> ParsedFaceBuffer = ParsedGroup->Faces;
		u32 FaceCount = ParsedFaceBuffer.GetSize();
		u32 TriangleIndex = 0;
		for( u32 FaceLoopCount = 0; FaceLoopCount < FaceCount; ++FaceLoopCount )
		{
			face* ParsedFace = ParsedFaceBuffer.Pop();
			
			for( u32 j = 0; j < ParsedFace->nv-2; ++j)
			{
				face* NewFace = &NewGroup->Faces[TriangleIndex++];

				Assert(ParsedFace->vi);

				NewFace->nv = 3;
				NewFace->vi = (u32*)PushArray(AssetArena, 3, u32);
				NewFace->vi[0] = ParsedFace->vi[0];
				NewFace->vi[1] = ParsedFace->vi[1+j];
				NewFace->vi[2] = ParsedFace->vi[2+j];

				if(ParsedFace->ni)
				{
					NewFace->ni = (u32*) PushArray( AssetArena, 3, u32);
					NewFace->ni[0] = ParsedFace->ni[0];
					NewFace->ni[1] = ParsedFace->ni[1 + j];
					NewFace->ni[2] = ParsedFace->ni[2 + j];
				}
		
				if(ParsedFace->ti)
				{
					NewFace->ti = (u32*) PushArray( AssetArena, 3, u32);
					NewFace->ti[0] = ParsedFace->ti[0];
					NewFace->ti[1] = ParsedFace->ti[1 + j];
					NewFace->ti[2] = ParsedFace->ti[2 + j];
				}

			}
		}
		Assert( ParsedFaceBuffer.IsEmpty() );
	}

	Assert( GroupToParse.IsEmpty() );

	EndTemporaryMemory( TempMem );

	Result->MeshData  = MeshData;
	Result->ObjectCount = ObjectCount;
	Result->Objects   = Objects;

	CenterObjectAtOrigin( Result );
	//ScaleObjectToUnitQube( Result );

	return Result;
}

entity* SetMeshAndMaterialComponentFromObjGroup( world* World, obj_group* Grp, mesh_data* MeshData )
{
	entity* Entity = NewEntity( World );
	NewComponents( World, Entity,  COMPONENT_TYPE_RENDER_MESH );

	Entity->RenderMeshComponent->TriangleCount = Grp->FaceCount;
	Entity->RenderMeshComponent->Triangles = Grp->Faces;
	Entity->RenderMeshComponent->Data = MeshData;
	Entity->RenderMeshComponent->T = M4Identity();

	if( Grp->Material )
	{
		surface_property* SurfaceProperty = &Entity->RenderMeshComponent->SurfaceProperty;

		if( Grp->Material->Ka || Grp->Material->Kd || Grp->Material->Ks || Grp->Material->Ns )
		{
			SurfaceProperty->Material = PushStruct( &World->Arena, material);
			if(Grp->Material->Ka)
			{
				SurfaceProperty->Material->AmbientColor  = *Grp->Material->Ka;
			}else{
				SurfaceProperty->Material->AmbientColor = V4(1,1,1,1);
			}

			if(Grp->Material->Kd)
			{
				SurfaceProperty->Material->DiffuseColor  = *Grp->Material->Kd;
			}else{
				SurfaceProperty->Material->DiffuseColor = V4(1,1,1,1);
			}

			if(Grp->Material->Ks)
			{
				SurfaceProperty->Material->SpecularColor  = *Grp->Material->Ks;
			}else{
				SurfaceProperty->Material->SpecularColor = V4(1,1,1,1);
			}

			if( Grp->Material->Ns )
			{
				SurfaceProperty->Material->Shininess  	 = *Grp->Material->Ns;
			}else{
				SurfaceProperty->Material->Shininess 	 = 1;
			}
		}

		SurfaceProperty->DiffuseMap  = Grp->Material->MapKd;
	}

	return Entity;
}

void SetMeshAndMaterialComponentFromObjFile( world* World, loaded_obj_file* ObjFile )
{
	for( u32  ObjectIndex = 0; ObjectIndex < ObjFile->ObjectCount; ++ObjectIndex )
	{
		obj_group* Grp = &ObjFile->Objects[ObjectIndex];
		SetMeshAndMaterialComponentFromObjGroup(World, Grp, ObjFile->MeshData );
	}
}


#include "mesh.h"


ordered_mesh* CreateOrderedMesh( memory_arena* Arena, loaded_obj_file*  LoadedObject )
{
	ordered_mesh* Mesh = PushArray(Arena, LoadedObject->ObjectCount, ordered_mesh );
	return 0;
}