// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ggponet.h"
#include "RollbackManager.h"
#include "GGPOInterface.generated.h"

USTRUCT(BlueprintType)
struct FGGPOInputFrame
{
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
	int leftRight = 0;
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
	int upDown = 0;
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
	bool a1 = false;
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
	bool a2 = false;
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
	bool a3 = false;
};

DECLARE_DYNAMIC_DELEGATE_TwoParams(FAdvanceFrameDelegateType, FGGPOInputFrame, P1Input, FGGPOInputFrame, P2Input);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class BPSUMMER2020_API UGGPOInterface : public UActorComponent
{
	GENERATED_BODY()

public:	

	GGPOPlayer p1, p2;
	GGPOPlayerHandle player_handles[2];
	ARollbackManager* rollbackManager;

	int playerNumber;

	bool sessionStarted = false;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
	FAdvanceFrameDelegateType AdvanceFrameDelegate;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
	bool syncTest = false;

	// Sets default values for this component's properties
	UGGPOInterface();

    UFUNCTION(BlueprintCallable)
    void StartSession();

	UFUNCTION(BlueprintCallable)
	void GameTick(FGGPOInputFrame localInput);

	UFUNCTION(BlueprintCallable)
	void GGPOIdle();

	UFUNCTION(BlueprintCallable)
	void EndSession();

	UFUNCTION(BlueprintCallable)
	void SetPlayerNumber(int num);

	void GameAdvanceFrame(int16 inputs[], int disconnect_flags);

	static FGGPOInputFrame convertInt16ToInputFrame(int16 input);

	static int16 convertInputFrameToInt16(FGGPOInputFrame input);


protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

 //   static bool __cdecl beginGame(const char* game);
 //   static bool __cdecl saveGameState(unsigned char** buffer, int* len, int* checksum, int frame);

	//static bool __cdecl loadGameState(unsigned char* buffer, int len);

	//static void __cdecl freeBuffer(void* buffer);

	//static bool __cdecl advanceFrame(int flags);

	//static bool __cdecl onEvent(GGPOEvent* info);


public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

		
};
