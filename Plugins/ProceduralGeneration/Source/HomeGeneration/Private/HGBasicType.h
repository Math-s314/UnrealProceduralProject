//Fill with copyright

#pragma once

#include "CoreMinimal.h"

struct FVectorGrid
{
	//By default generate a vector null : (0, 0)
	FVectorGrid();
	FVectorGrid(int _X, int _Y);
	
	int X;
	int Y;

	//Operators and maths
	FVectorGrid operator+(const FVectorGrid &In) const;
	FVectorGrid operator-(const FVectorGrid &In) const;
	FVectorGrid operator*(const float Lambda) const;
	FVectorGrid &operator+=(const FVectorGrid &In);
	FVectorGrid &operator-=(const FVectorGrid &In);
	
	static FVectorGrid Min(const FVectorGrid &a, const FVectorGrid &b);
	static FVectorGrid Max(const FVectorGrid &a, const FVectorGrid &b);
	static FVectorGrid Abs(const FVectorGrid &a);
	static FVectorGrid Random(const FVectorGrid &min, const FVectorGrid &max);//Inclusive, by coordinates
	
	static const FVectorGrid Zero;
	static const FVectorGrid Unit;
	static const FVectorGrid IVector;
	static const FVectorGrid JVector;
	
	//Export
	FVector ToVector(float GridSnapLength) const;
};