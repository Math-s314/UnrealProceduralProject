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

	//Operators
	FVectorGrid operator+(const FVectorGrid &In) const;
	FVectorGrid operator-(const FVectorGrid &In) const;
	FVectorGrid operator*(const float Lambda) const;
	
	FVectorGrid &operator+=(const FVectorGrid &In);
};