#ifndef HANDMADE_PLATFOR_H
#define HANDMADE_PLATFOR_H

union v2
{
	struct{
		real32 X,Y;
	};
	real32 E[2];
};

v2 V2(real32 X, real32 Y){
	v2 Result = {X,Y};
	return(Result);
};

inline v2
operator*(real32 A, v2 B)
{
	v2 Result;
	Result.X = A*B.X;
	Result.Y = A*B.Y;
	return Result;	
}

inline v2
operator*(v2 B, real32 A)
{
	v2 Result;
	Result = A*B;
	return Result;	
}

inline v2&
operator*=(v2& B, real32 A)
{
	B = A*B;
	return B;
}

inline v2
operator+(v2 A, v2 B)
{
	v2 Result;
	Result.X = A.X + B.X;
	Result.Y = A.Y + B.Y;
	return Result;
}

inline v2&
operator+=(v2& A, v2 B)
{
	A = A+B;
	return A;
}


inline v2
operator-(v2 A, v2 B)
{
	v2 Result;
	Result.X = A.X - B.X;
	Result.Y = A.Y - B.Y;
	return Result;
}

inline v2&
operator-=(v2& A, v2 B)
{
	A = A-B;
	return A;
}

inline real32
DotProduct(v2 A, v2 B)
{
	real32 Result = A.X*B.X + A.Y*B.Y;
	return Result;
}

union v3
{
	struct{
		real32 X, Y, Z;
	};
	real32 E[3];
};

v3 V3(real32 X, real32 Y, real32 Z){
	v3 Result = {X,Y,Z};
	return(Result);
};

inline v3
operator*(real32 A, v3 B)
{
	v3 Result;
	Result.X = A*B.X;
	Result.Y = A*B.Y;
	Result.Z = A*B.Z;
	return Result;	
}

inline v3
operator*(v3 B, real32 A)
{
	v3 Result;
	Result = A*B;
	return Result;	
}

inline v3&
operator*=(v3& B, real32 A)
{
	B = A*B;
	return B;
}

inline v3
operator+(v3 A, v3 B)
{
	v3 Result;
	Result.X = A.X + B.X;
	Result.Y = A.Y + B.Y;
	Result.Z = A.Z + B.Z;
	return Result;
}

inline v3&
operator+=(v3& A, v3 B)
{
	A = A+B;
	return A;
}


inline v3
operator-(v3 A, v3 B)
{
	v3 Result;
	Result.X = A.X - B.X;
	Result.Y = A.Y - B.Y;
	Result.Z = A.Z - B.Z;
	return Result;
}

inline v3&
operator-=(v3& A, v3 B)
{
	A = A-B;
	return A;
}

union v4
{
	struct{
		real32 X, Y, Z, W;

	};
	real32 E[4];
};


v4 V4(real32 X, real32 Y, real32 Z, real32 W){
	v4 Result = {X,Y,Z,W};
	return(Result);
};


inline v4
operator*(real32 A, v4 B)
{
	v4 Result;
	Result.X = A*B.X;
	Result.Y = A*B.Y;
	Result.Z = A*B.Z;
	Result.W = A*B.W;
	return Result;	
}

inline v4
operator*(v4 B, real32 A)
{
	v4 Result;
	Result = A*B;
	return Result;	
}

inline v4&
operator*=(v4& B, real32 A)
{
	B = A*B;
	return B;
}

inline v4
operator+(v4 A, v4 B)
{
	v4 Result;
	Result.X = A.X + B.X;
	Result.Y = A.Y + B.Y;
	Result.Z = A.Z + B.Z;
	Result.Z = A.W + B.W;
	return Result;
}

inline v4&
operator+=(v4& A, v4 B)
{
	A = A+B;
	return A;
}


inline v4
operator-(v4 A, v4 B)
{
	v4 Result;
	Result.X = A.X - B.X;
	Result.Y = A.Y - B.Y;
	Result.Z = A.Z - B.Z;
	Result.W = A.W - B.W;
	return Result;
}

inline v4&
operator-=(v4& A, v4 B)
{
	A = A-B;
	return A;
}


//struct m4{
//	real32 E[16];
//};
//
//uint idx4(uint Row, uint Col)
//{
//	int Result = 4*Row + Col;
//	return Result;
//};
//




#endif