//Fill with copyright

#pragma once

#include "CoreMinimal.h"

struct FVectorGrid
{
	//By default generate a vector null : (0, 0)
	FVectorGrid();
	FVectorGrid(int _X, int _Y);
	FVectorGrid(const FVector2D &Vector);
	
	int X;
	int Y;

	//Operators to Grid
	FVectorGrid operator+(const FVectorGrid &In) const;
	FVectorGrid operator-(const FVectorGrid &In) const;
	FVectorGrid &operator+=(const FVectorGrid &In);
	FVectorGrid &operator-=(const FVectorGrid &In);
	
	//Operators to Vector2D
	FVector2D operator*(const float Lambda) const;

	//Special calculations
	int Area() const;
	int MinSide() const;
	int MaxSide() const;
	float Norm() const;

	//Static maths
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