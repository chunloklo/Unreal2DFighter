// Fill out your copyright notice in the Description page of Project Settings.


#include "TimerComponent.h"

// Sets default values for this component's properties
UTimerComponent::UTimerComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UTimerComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UTimerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UTimerComponent::SetTimer(FMyDelegate delegate, int frames)
{
    OnMyDelegateCalled = delegate;
    framesTillExecute = frames;

    if (framesTillExecute == 0) {
        RunDelegate();
    }
}

void UTimerComponent::SetTimerFreeze(int frames)
{
    if (frames < 0) {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, "Setting Timer Freeze to negative. Setting freeze time to 0 instead");
        frames = 0;
    }
    framesTillEndFreeze = frames;
}

void UTimerComponent::Tick()
{
    // Timer is not active. Don't decrement any more.
    if (framesTillExecute < 0) {
        return;
    }

    if (framesTillEndFreeze > 0) {
        framesTillEndFreeze -= 1;
        return;
    }

    framesTillExecute -= 1;

    if (framesTillExecute == 0) {
        RunDelegate();
    }
}

void UTimerComponent::RunDelegate() {
    bool bound = OnMyDelegateCalled.IsBound();
    if (bound) {
        OnMyDelegateCalled.Execute();
    }
    else {
        UE_LOG(LogTemp, Warning, TEXT("DELEGATE UNBOUND"));
    }
}
