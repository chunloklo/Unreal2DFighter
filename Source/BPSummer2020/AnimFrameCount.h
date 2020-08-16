// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PaperFlipbookComponent.h"
#include "PaperFlipbook.h"
#include "AnimFrameCount.generated.h"

// Class used for animation rollback! Takes complete control over animation for rollback on each tick.
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class BPSUMMER2020_API UAnimFrameCount : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UAnimFrameCount();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame.
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Should never really be changed. No rollback needed.  The component will have full control over animation in the frame.
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
	UPaperFlipbookComponent* flipbookComponent = NULL;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, SaveGame)
	UPaperFlipbook* flipbook = NULL;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, SaveGame)
	int frame = 0;

	UFUNCTION(BlueprintCallable)
	void Tick();

	// Please call every time you set animation!
	UFUNCTION(BlueprintCallable)
	void StartAnim(UPaperFlipbook* flipbookParam, int startFrame = 0);

		
};
