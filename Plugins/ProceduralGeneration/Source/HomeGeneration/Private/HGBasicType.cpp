//Fill

#include "HGBasicType.h"

const FVectorGrid FVectorGrid::Zero(0,0);
const FVectorGrid FVectorGrid::Unit(1, 1);
const FVectorGrid FVectorGrid::IVector(1, 0);
const FVectorGrid FVectorGrid::JVector(0, 1);

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
	X += In.X;
	Y += In.Y;
	return *this;
}

FVectorGrid& FVectorGrid::operator-=(const FVectorGrid& In)
{
	X -= In.X;
	Y -= In.Y;
	return *this;
}

FVectorGrid FVectorGrid::Random(const FVectorGrid& min, const FVectorGrid& max)
{
	return FVectorGrid(FMath::RandRange(min.X, max.X), FMath::RandRange(min.Y, max.Y));
}

FVectorGrid FVectorGrid::Min(const FVectorGrid& a, const FVectorGrid& b)
{
	return FVectorGrid(FMath::Min(a.X, b.X), FMath::Min(a.Y, b.Y));
}

FVectorGrid FVectorGrid::Max(const FVectorGrid& a, const FVectorGrid& b)
{
	return FVectorGrid(FMath::Max(a.X, b.X), FMath::Max(a.Y, b.Y));
}

FVectorGrid FVectorGrid::Abs(const FVectorGrid& a)
{
	return FVectorGrid(FMath::Abs(a.X), FMath::Abs(a.Y));
}

FVector FVectorGrid::ToVector(float GridSnapLength) const
{
	return FVector(X * GridSnapLength, Y * GridSnapLength, 0.f);
}
