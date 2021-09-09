// Fill out your copyright notice in the Description page of Project Settings.


#include "HomeGenerator.h"


// Sets default values
AHomeGenerator::AHomeGenerator()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void AHomeGenerator::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AHomeGenerator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

