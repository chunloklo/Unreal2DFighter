// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RollbackManager.generated.h"

UCLASS()
class BPSUMMER2020_API ARollbackManager : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ARollbackManager();

    // Pointer list to manage lifetimes
    UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
    TMap<AActor *, int> actorMap;
    TMap<AActor*, bool> deserializedMap;

    int GGPOPredictionBarrier = 48;

    UPROPERTY(EditAnywhere)
    float health;

    static const int BUFSIZE = 2048;

    uint8 saveGameBuffer[BUFSIZE];

    uint8 saveGameSlot[BUFSIZE];

    enum DataFlag {
        ACTOR,
        ACTOR_END,
        ACTOR_LOCATION,
        ACTOR_ROTATION,
        COMPONENT,
        COMPONENT_END,
        DATA_BOOL,
        DATA_INT,
        DATA_FLOAT,
        DATA_STRUCT,
        DATA_DELEGATE,
        DATA_BYTE,
        DATA_OBJECT,
        DATA_ARRAY,
        END,

    };

    const bool logSerialization = false;

    uint8* serializeFlag(uint8* bufferHead, DataFlag flag);

    // Check flag equality on top of buffer. Return incremented buffer if check was successful
    uint8* deserializeFlag(uint8* bufferHead, DataFlag flag, bool *success);

    uint8* serializePropName(uint8* bufferHead, FString propName);

    uint8* deserializePropName(uint8* bufferHead, FString* propName);

    /**
     * Serializes bool prop from object
     * @param current buffer head. Should be at the beginning of a bool frame
     * @param object Object whose property value you want to serialize
     * @param prop bool property you are trying to serialize
     * @return buffer head at the end after data is serialized
     */
    uint8* serializeBoolProp(uint8* bufferHead, UObject& object, UBoolProperty *prop);

    /** 
    * Deserializes bool prop from object
    * @param current buffer head. Should be at the beginning of a bool frame
    * @param object Object whose property you want to deserialize into
    * @return buffer head at the end of the read data
    */
    uint8* deserializeBoolProp(uint8* bufferHead, UObject& object, bool *success);

    /**
     * Serializes bool prop from object
     * @param current buffer head. Should be at the beginning of a bool frame
     * @param object Object whose property value you want to serialize
     * @param prop bool property you are trying to serialize
     * @return buffer head at the end after data is serialized
     */
    uint8* serializeIntProp(uint8* bufferHead, UObject& object, UIntProperty *prop);

    /**
    * Deserializes bool prop from object
    * @param current buffer head. Should be at the beginning of a bool frame
    * @param object Object whose property you want to deserialize into
    * @return buffer head at the end of the read data
    */
    uint8* deserializeIntProp(uint8* bufferHead, UObject& object, bool *success);

    uint8* serializeFloatProp(uint8* bufferHead, UObject& object, UFloatProperty *prop);
    uint8* deserializeFloatProp(uint8* bufferHead, UObject& object, bool *success);

    uint8* serializeStructProp(uint8* bufferHead, UObject& object, UStructProperty *prop);
    uint8* deserializeStructProp(uint8* bufferHead, UObject& object, bool *success);

    uint8* serializeDelegateProp(uint8* bufferHead, UObject& object, UDelegateProperty *prop);
    uint8* deserializeDelegateProp(uint8* bufferHead, UObject& object, bool *success);

    uint8* serializeByteProp(uint8* bufferHead, UObject& object, UByteProperty *prop);
    uint8* deserializeByteProp(uint8* bufferHead, UObject& object, bool *success);

    uint8* serializeObjectProp(uint8* bufferHead, UObject& object, UObjectProperty* prop);
    uint8* deserializeObjectProp(uint8* bufferHead, UObject& object, bool* success);

    uint8* serializeArrayProp(uint8* bufferHead, UObject& object, UArrayProperty* prop);
    uint8* deserializeArrayProp(uint8* bufferHead, UObject& object, bool* success);

    uint8* serializeProps(uint8* bufferHead, UObject& object);
    uint8* deserializeProps(uint8* bufferHead, UObject& object);

    uint8* serializeActorComponent(uint8* bufferHead, UActorComponent *component);
    uint8* deserializeActorComponent(uint8* bufferHead, bool *success);

    uint8* serializeActorTransform(uint8* bufferHead, AActor& actor);
    uint8* deserializeActorTransform(uint8* bufferHead, AActor& actor);

    

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

    void DestroyActor(AActor* actor);

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

    UFUNCTION(BlueprintCallable)
    void serializeObjects();

    UFUNCTION(BlueprintCallable)
    void restoreObjects();


    UFUNCTION(BlueprintCallable)
    void saveToSlot();

    UFUNCTION(BlueprintCallable)
    void restoreFromSlot();

    UFUNCTION(BlueprintCallable)
    void HideActor(AActor* actor);

    UFUNCTION(BlueprintCallable)
    void ShowActor(AActor* actor);

    UFUNCTION(BlueprintCallable)
    void AddActor(AActor * actor);

    UFUNCTION(BlueprintCallable)
    void KillActor(AActor* actor);

    UFUNCTION(BlueprintCallable)
    void NetworkTick();
};
