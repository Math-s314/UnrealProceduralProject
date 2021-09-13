//Fill

#include "HGBasicType.h"

FVectorGrid::FVectorGrid() : X(0), Y(0) {}

FVectorGrid::FVectorGrid(int _X, int _Y) : X(_X), Y(_Y) {}

FVectorGrid FVectorGrid::operator+(const FVectorGrid& In) const
{
	return FVectorGrid(X + In.X, Y + In.Y);
}

FVectorGrid FVectorGrid::operator-(const FVectorGrid& In) const
{
	return FVectorGrid(X - In.X, Y - In.Y);
}

FVectorGrid FVectorGrid::operator*(const float Lambda) const
{
	return FVectorGrid((int) X * Lambda, (int) Y * Lambda);
}

FVectorGrid &FVectorGrid::operator+=(const FVectorGrid& In)
{
	X += In.X; Y += In.Y;
	return *this;
}