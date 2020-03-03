#ifndef COMPONENT_SURFACE_H
#define COMPONENT_SURFACE_H
struct bitmap;

enum MATERIAL_TYPE
{
	MATERIAL_WHITE,
	MATERIAL_RED,
	MATERIAL_GREEN,
	MATERIAL_BLUE,
	MATERIAL_EMERALD,
	MATERIAL_JADE,
	MATERIAL_OBSIDIAN,
	MATERIAL_PEARL,
	MATERIAL_RUBY,
	MATERIAL_TURQUOISE,
	MATERIAL_BRASS,
	MATERIAL_BRONZE,
	MATERIAL_CHROME,
	MATERIAL_COMPPER,
	MATERIAL_GOLD,
	MATERIAL_SILVER,
	MATERIAL_BLACK_PLASTIC,
	MATERIAL_CYAN_PLASTIC,
	MATERIAL_GREEN_PLASTIC,
	MATERIAL_RED_PLASTIC,
	MATERIAL_BLUE_PLASTIC,
	MATERIAL_WHITE_PLASTIC,
	MATERIAL_YELLOW_PLASTIC,
	MATERIAL_BLACK_RUBBER,
	MATERIAL_CYAN_RUBBER,
	MATERIAL_GREEN_RUBBER,
	MATERIAL_RED_RUBBER,
	MATERIAL_BLUE_RUBBER,
	MATERIAL_WHITE_RUBBER,
	MATERIAL_YELLOW_RUBBER
};

struct material
{
	v4 AmbientColor;
	v4 DiffuseColor;
	v4 SpecularColor;
	r32 Shininess;
	bitmap* DiffuseMap;
};

struct component_surface
{
	material* Material;
};

void SetMaterial(material* Material, u32 MaterialType)
{
	local_persist material mtl[30] =
	{
		{ V4(1,        1,        1,        1), V4( 1,        1,          1,          1), V4( 1,          1,          1,          1), 1          }, // WHITE
		{ V4(1,        0,        0,        1), V4( 1,        0,          0,          1), V4( 1,          0,          0,          1), 1          }, // RED
		{ V4(0,        1,        0,        1), V4( 0,        1,          0,          1), V4( 0,          1,          0,          1), 1          }, // GREEN
		{ V4(0,        0,        1,        1), V4( 0,        0,          1,          1), V4( 0,          0,          1,          1), 1          }, // Blue
	  { V4(0.0215,   0.1745,   0.0215,   1), V4( 0.07568,  0.61424,    0.07568,    1), V4( 0.633,      0.727811,   0.633,      1), 0.6        }, // emerald
	  { V4(0.135,    0.2225,   0.1575,   1), V4( 0.54,     0.89,       0.63,       1), V4( 0.316228,   0.316228,   0.316228,   1), 0.1        }, // jade
	  { V4(0.05375,  0.05,     0.06625,  1), V4( 0.18275,  0.17,       0.22525,    1), V4( 0.332741,   0.328634,   0.346435,   1), 0.3        }, // obsidian
	  { V4(0.25,     0.20725,  0.20725,  1), V4( 1.0,      0.829,      0.829,      1), V4( 0.296648,   0.296648,   0.296648,   1), 0.088      }, // pearl
	  { V4(0.1745,   0.01175,  0.01175,  1), V4( 0.61424,  0.04136,    0.04136,    1), V4( 0.727811,   0.626959,   0.626959,   1), 0.6        }, // ruby
	  { V4(0.1,      0.18725,  0.1745,   1), V4( 0.396,    0.74151,    0.69102,    1), V4( 0.297254,   0.30829,    0.306678,   1), 0.1        }, // turquoise
	  { V4(0.329412, 0.223529, 0.027451, 1), V4( 0.780392, 0.568627,   0.113725,   1), V4( 0.992157,   0.941176,   0.807843,   1), 0.21794872 }, // brass
	  { V4(0.2125,   0.1275,   0.054,    1), V4( 0.714,    0.4284,     0.18144,    1), V4( 0.393548,   0.271906,   0.166721,   1), 0.2        }, // bronze
	  { V4(0.25,     0.25,     0.25,     1), V4( 0.4,      0.4,        0.4,        1), V4( 0.774597,   0.774597,   0.774597,   1), 0.6        }, // chrome
	  { V4(0.19125,  0.0735,   0.0225,   1), V4( 0.7038,   0.27048,    0.0828,     1), V4( 0.256777,   0.137622,   0.086014,   1), 0.1        }, // copper
	  { V4(0.24725,  0.1995,   0.0745,   1), V4( 0.75164,  0.60648,    0.22648,    1), V4( 0.628281,   0.555802,   0.366065,   1), 0.4        }, // gold
	  { V4(0.19225,  0.19225,  0.19225,  1), V4( 0.50754,  0.50754,    0.50754,    1), V4( 0.508273,   0.508273,   0.508273,   1), 0.4        }, // silver
	  { V4(0.0,      0.0,      0.0,      1), V4( 0.01,     0.01,       0.01,       1), V4( 0.50,       0.50,       0.50,       1), 0.25       }, // black plastic 	
	  { V4(0.0,      0.1,      0.06,     1), V4( 0.0,      0.50980392, 0.50980392, 1), V4( 0.50196078, 0.50196078, 0.50196078, 1), 0.25       }, // cyan plastic 	
	  { V4(0.0,      0.0,      0.0,      1), V4( 0.1,      0.35,       0.1,        1), V4( 0.45,       0.55,       0.45,       1), 0.25       }, // green plastic 	
	  { V4(0.0,      0.0,      0.0,      1), V4( 0.5,      0.0,        0.0,        1), V4( 0.7,        0.6,        0.6,        1), 0.25       }, // red plastic 
	  { V4(0.0,      0.0,      0.0,      1), V4( 0.0,      0.0,        0.5,        1), V4( 0.6,        0.6,        0.7,        1), 0.25       }, // blue plastic  		
	  { V4(0.0,      0.0,      0.0,      1), V4( 0.55,     0.55,       0.55,       1), V4( 0.70,       0.70,       0.70,       1), 0.25       }, // white plastic 	
	  { V4(0.0,      0.0,      0.0,      1), V4( 0.5,      0.5,        0.0,        1), V4( 0.60,       0.60,       0.50,       1), 0.25       }, // yellow plastic
	  { V4(0.02,     0.02,     0.02,     1), V4( 0.01,     0.01,       0.01,       1), V4( 0.4,        0.4,        0.4,        1), 0.078125   }, // black rubber 	
	  { V4(0.0,      0.05,     0.05,     1), V4( 0.4,      0.5,        0.5,        1), V4( 0.04,       0.7,        0.7,        1), 0.078125   }, // cyan rubber 	
	  { V4(0.0,      0.05,     0.0,      1), V4( 0.4,      0.5,        0.4,        1), V4( 0.04,       0.7,        0.04,       1), 0.078125   }, // green rubber 	
	  { V4(0.05,     0.0,      0.0,      1), V4( 0.5,      0.4,        0.4,        1), V4( 0.7,        0.04,       0.04,       1), 0.078125   }, // red rubber 	    
	  { V4(0.00,     0.0,      0.05,     1), V4( 0.4,      0.4,        0.5,        1), V4( 0.04,       0.04,       0.7,        1), 0.078125   }, // blue rubber 
	  { V4(0.05,     0.05,     0.05,     1), V4( 0.5,      0.5,        0.5,        1), V4( 0.7,        0.7,        0.7,        1), 0.078125   }, // white rubber 	
	  { V4(0.05,     0.05,     0.0,      1), V4( 0.5,      0.5,        0.4,        1), V4( 0.7,        0.7,        0.04,       1), 0.078125   } // yellow rubber 		    
	};

	switch(MaterialType)
	{
		case MATERIAL_WHITE:          *Material = mtl[0 ]; break;
		case MATERIAL_RED:            *Material = mtl[1 ]; break;
		case MATERIAL_GREEN:          *Material = mtl[2 ]; break;
		case MATERIAL_BLUE:           *Material = mtl[3 ]; break;
		case MATERIAL_EMERALD:        *Material = mtl[4 ]; break;
		case MATERIAL_JADE:           *Material = mtl[5 ]; break;
		case MATERIAL_OBSIDIAN:       *Material = mtl[6 ]; break;
		case MATERIAL_PEARL:          *Material = mtl[7 ]; break;
		case MATERIAL_RUBY:           *Material = mtl[8 ]; break;
		case MATERIAL_TURQUOISE:      *Material = mtl[9 ]; break;
		case MATERIAL_BRASS:          *Material = mtl[10]; break;
		case MATERIAL_BRONZE:         *Material = mtl[11]; break;
		case MATERIAL_CHROME:         *Material = mtl[12]; break;
		case MATERIAL_COMPPER:        *Material = mtl[13]; break;
		case MATERIAL_GOLD:           *Material = mtl[14]; break;
		case MATERIAL_SILVER:         *Material = mtl[15]; break;
		case MATERIAL_BLACK_PLASTIC:  *Material = mtl[16]; break;
		case MATERIAL_CYAN_PLASTIC:   *Material = mtl[17]; break;
		case MATERIAL_GREEN_PLASTIC:  *Material = mtl[18]; break;
		case MATERIAL_RED_PLASTIC:    *Material = mtl[19]; break;
		case MATERIAL_BLUE_PLASTIC:   *Material = mtl[20]; break;
		case MATERIAL_WHITE_PLASTIC:  *Material = mtl[21]; break;
		case MATERIAL_YELLOW_PLASTIC: *Material = mtl[22]; break;
		case MATERIAL_BLACK_RUBBER:   *Material = mtl[23]; break;
		case MATERIAL_CYAN_RUBBER:    *Material = mtl[24]; break;
		case MATERIAL_GREEN_RUBBER:   *Material = mtl[25]; break;
		case MATERIAL_RED_RUBBER:     *Material = mtl[26]; break;
		case MATERIAL_BLUE_RUBBER:    *Material = mtl[27]; break;
		case MATERIAL_WHITE_RUBBER:   *Material = mtl[28]; break;
		case MATERIAL_YELLOW_RUBBER:  *Material = mtl[29]; break;
		default: Assert(0) break;
	}
}

#endif // COMPONENT_SURFACE_PROPERTY_H