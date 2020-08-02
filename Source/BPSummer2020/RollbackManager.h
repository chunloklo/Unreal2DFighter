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
    TSet<UObject *> objectArray;

    UPROPERTY(EditAnywhere)
    float health;

    static const int BUFSIZE = 2048;

    uint8 saveGameBuffer[BUFSIZE];

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
        END,

    };

    uint8* serializeFlag(uint8* bufferHead, DataFlag flag);

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
    uint8* deserializeBoolProp(uint8* bufferHead, UObject& object);

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
    uint8* deserializeIntProp(uint8* bufferHead, UObject& object);

    uint8* serializeFloatProp(uint8* bufferHead, UObject& object, UFloatProperty *prop);
    uint8* deserializeFloatProp(uint8* bufferHead, UObject& object);

    uint8* serializeStructProp(uint8* bufferHead, UObject& object, UStructProperty *prop);
    uint8* deserializeStructProp(uint8* bufferHead, UObject& object);

    //uint8* serializeVectorProp(uint8* bufferHead, UObject& object, UFVectorProperty *prop);
    //uint8* deserializeVectorProp(uint8* bufferHead, UObject& object);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;


public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

    UFUNCTION(BlueprintCallable)
    void serializeObjects();

    UFUNCTION(BlueprintCallable)
    void restoreObjects();

};
