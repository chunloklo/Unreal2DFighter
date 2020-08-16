// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TimerComponent.generated.h"

DECLARE_DYNAMIC_DELEGATE(FMyDelegate);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class BPSUMMER2020_API UTimerComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UTimerComponent();

    UPROPERTY(BlueprintReadWrite, VisibleAnywhere, SaveGame)
    FMyDelegate OnMyDelegateCalled;

    UPROPERTY(BlueprintReadWrite, VisibleAnywhere, SaveGame)
    int framesTillExecute = -1;

    UPROPERTY(BlueprintReadWrite, VisibleAnywhere, SaveGame)
    int framesTillEndFreeze = 0;

    UFUNCTION(BlueprintCallable)
    void SetTimer(FMyDelegate delegate, int frames);

    UFUNCTION(BlueprintCallable)
    void SetTimerFreeze(int frames);

    UFUNCTION(BlueprintCallable)
    void Tick();

    UFUNCTION(BlueprintCallable)
    void RunDelegate();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

		
};
