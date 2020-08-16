// Fill out your copyright notice in the Description page of Project Settings.


#include "AnimFrameCount.h"

namespace AnimFrameCount {
	bool debugLog = true;
}

using namespace AnimFrameCount;

// Sets default values for this component's properties
UAnimFrameCount::UAnimFrameCount()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UAnimFrameCount::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UAnimFrameCount::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}


void UAnimFrameCount::Tick()
{
	if (!flipbookComponent) {
		UE_LOG(LogTemp, Warning, TEXT("Flipbook Not set in AnimFrameCount. Please set me!"));
		return;
	}

	if (flipbook != flipbookComponent->GetFlipbook())
	{
		flipbookComponent->SetFlipbook(flipbook);
		if (debugLog) UE_LOG(LogTemp, Warning, TEXT("Setting new flipbook"));
	}

	float totalLength = flipbookComponent->GetFlipbookLength() * 60;
	if (frame >= totalLength - 1)
	{
		if (flipbookComponent->IsLooping()) {
			frame = 0;
		}
	}
	else {
		frame += 1;
	}

	flipbookComponent->SetPlaybackPosition(frame / 60.0, true);
}

void UAnimFrameCount::StartAnim(UPaperFlipbook* flipbookParam, int startFrame)
{
	flipbook = flipbookParam;
	if (!flipbookComponent) {
		UE_LOG(LogTemp, Warning, TEXT("Flipbook Not set in AnimFrameCount. Please set me!"));
		return;
	}

	frame = startFrame % flipbookComponent->GetFlipbookLengthInFrames();
}
