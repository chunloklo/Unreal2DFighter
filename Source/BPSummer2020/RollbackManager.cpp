// Fill out your copyright notice in the Description page of Project Settings.

PRAGMA_DISABLE_OPTIMIZATION
#include "RollbackManager.h"

// Sets default values
ARollbackManager::ARollbackManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ARollbackManager::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ARollbackManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Call to serialize object
void ARollbackManager::serializeObjects()
{
    uint8 *bufferHead = saveGameBuffer;

    for (UObject* Elem : objectArray)
    {
        UClass * objectClass = Elem->GetClass();

        // Add Object Flag
        DataFlag actor_flag = DataFlag::ACTOR;
        memcpy(bufferHead, &actor_flag, sizeof(DataFlag));
        bufferHead += sizeof(DataFlag);

        // Add Pointer
        memcpy(bufferHead, &Elem, sizeof(UObject*));
        bufferHead += sizeof(UObject *);
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("%p"), Elem));

        if (objectClass->IsChildOf(AActor::StaticClass())) {
            GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, "is actor");

            AActor *actorPointer = Cast<AActor>(Elem);
            FVector location = actorPointer->GetActorLocation();

            // Serialize actor location
            //copy x y z
            memcpy(bufferHead, &location.X, sizeof(float));
            bufferHead += sizeof(float);
            memcpy(bufferHead, &location.Y, sizeof(float));
            bufferHead += sizeof(float);
            memcpy(bufferHead, &location.Z, sizeof(float));
            bufferHead += sizeof(float);

            //Serialize actor rotation
            FRotator rotation = actorPointer->GetActorRotation();
            //copy Pitch Row and Yaw
            memcpy(bufferHead, &rotation.Pitch, sizeof(float));
            bufferHead += sizeof(float);
            memcpy(bufferHead, &rotation.Roll, sizeof(float));
            bufferHead += sizeof(float);
            memcpy(bufferHead, &rotation.Yaw, sizeof(float));
            bufferHead += sizeof(float);
                

            // Serialize components?
            TArray<UActorComponent *> components;
            // not sure right now...?
            actorPointer->GetComponents(components, false);

            for (UActorComponent *component : components) { 
                if (component->ComponentHasTag(TEXT("Rollback"))) {

                    // Add component tag
                    bufferHead = serializeFlag(bufferHead, DataFlag::COMPONENT);

                    // Add Pointer
                    memcpy(bufferHead, &component, sizeof(UActorComponent *));
                    bufferHead += sizeof(UActorComponent *);

                    FString name = component->GetName();
                    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, name);

                    UClass *componentClass = component->GetClass();

                     //Serialize bool properties
                    for (TFieldIterator<UBoolProperty> Prop(componentClass); Prop; ++Prop) {
                        EPropertyFlags flags = Prop->GetPropertyFlags();
                        if ((flags & CPF_SaveGame) > 0) {
                            // Serialize boolean
                            bufferHead = serializeBoolProp(bufferHead, *component, *Prop);
                        }
                    }
                    // Serialize ints
                    //bufferHead = serializeFloatProp(bufferHead, *Elem, Prop);

                    // Add component end tag
                    bufferHead = serializeFlag(bufferHead, DataFlag::COMPONENT_END);
                }
            }
        }
        else {
            GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, "uhhh this is not actor. Something is wrong");
        }

        for (TFieldIterator<UProperty> Prop(objectClass); Prop; ++Prop) {
            EPropertyFlags flags = Prop->GetPropertyFlags();
            UProperty * propPtr = *Prop;
            if ((flags & CPF_SaveGame) > 0) {
                GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, propPtr->GetName());
                UBoolProperty *boolPtr = Cast<UBoolProperty>(propPtr);
                if (boolPtr) {
                    bufferHead = serializeBoolProp(bufferHead, *Elem, boolPtr);
                    continue;
                }

                UIntProperty *intPtr = Cast<UIntProperty>(propPtr);
                if (boolPtr) {
                    bufferHead = serializeIntProp(bufferHead, *Elem, intPtr);
                    continue;
                }

                UFloatProperty *floatPtr = Cast<UFloatProperty>(propPtr);
                if (floatPtr) {
                    bufferHead = serializeFloatProp(bufferHead, *Elem, floatPtr);
                    continue;
                }     

                UStructProperty *structPtr = Cast<UStructProperty>(propPtr);
                if (structPtr) {
                    bufferHead = serializeStructProp(bufferHead, *Elem, structPtr);
                    continue;
                }
            }
        }
    }
    DataFlag endFlag = DataFlag::END;
    memcpy(bufferHead, &endFlag, sizeof(DataFlag));
    bufferHead += sizeof(DataFlag);
}

void ARollbackManager::restoreObjects()
{
    uint8 *bufferHead = saveGameBuffer;

    bool keepReading = true;
    while (keepReading) {
        
        //if (*dataFlagPtr == DataFlag::END) {
        //    keepReading = false;
        //    continue;
        //}

        DataFlag *dataFlagPtr = reinterpret_cast<DataFlag *>(bufferHead);
        bufferHead += sizeof(DataFlag);

        if (*dataFlagPtr == DataFlag::ACTOR) {
            UObject** objectPtr = reinterpret_cast<UObject **>(bufferHead);
            bufferHead += sizeof(UObject *);
            //GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("ChangingCode!"));
            GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("%p"), *objectPtr));
            
            bool hasObject = objectArray.Contains(*objectPtr);
            
            if (!hasObject) {
                GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, "Object Ptr Not Found! Something went wrong E:");
            }

            // Currently all serialized uobject is going to be actors.... Maybe that will change in the future.
            AActor *actorPointer = Cast<AActor>(*objectPtr);

            // Reset location x y z
            float *locX = reinterpret_cast<float *>(bufferHead);
            bufferHead += sizeof(float);

            float *locY = reinterpret_cast<float *>(bufferHead);
            bufferHead += sizeof(float);

            float *locZ = reinterpret_cast<float *>(bufferHead);
            bufferHead += sizeof(float);

            FVector location = FVector::FVector(*locX, *locY, *locZ);
            actorPointer->SetActorLocation(location);

            // Reset rotation row pitch yaw
            float *rotPitch = reinterpret_cast<float *>(bufferHead);
            bufferHead += sizeof(float);

            float *rotRoll = reinterpret_cast<float *>(bufferHead);
            bufferHead += sizeof(float);

            float *rotYaw = reinterpret_cast<float *>(bufferHead);
            bufferHead += sizeof(float);

            FRotator rotation = FRotator::FRotator(*rotPitch, *rotYaw, *rotRoll);

            actorPointer->SetActorRotation(rotation);

            dataFlagPtr = reinterpret_cast<DataFlag *>(bufferHead);
            bufferHead += sizeof(DataFlag);
            while (
                *dataFlagPtr == DataFlag::DATA_BOOL ||
                *dataFlagPtr == DataFlag::DATA_INT  ||
                *dataFlagPtr == DataFlag::DATA_FLOAT ||
                *dataFlagPtr == DataFlag::DATA_STRUCT ||
                *dataFlagPtr == DataFlag::COMPONENT) {

                if (*dataFlagPtr == DataFlag::DATA_BOOL) {
                    bufferHead = deserializeBoolProp(bufferHead, **objectPtr);
                }

                if (*dataFlagPtr == DataFlag::DATA_INT) {
                    bufferHead = deserializeIntProp(bufferHead, **objectPtr);
                }

                if (*dataFlagPtr == DataFlag::DATA_FLOAT) {
                    bufferHead = deserializeFloatProp(bufferHead, **objectPtr);
                }

                if (*dataFlagPtr == DataFlag::COMPONENT) {
                    UActorComponent * componentPtr = *reinterpret_cast<UActorComponent **>(bufferHead);
                    bufferHead += sizeof(UActorComponent *);

                    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Component!!!"));
                    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, componentPtr->GetName());


                    DataFlag* componentFlagPtr = reinterpret_cast<DataFlag *>(bufferHead);
                    bufferHead += sizeof(DataFlag);

                    while (*componentFlagPtr != DataFlag::COMPONENT_END) {
                        if (*componentFlagPtr == DataFlag::DATA_BOOL) {
                            bufferHead = deserializeBoolProp(bufferHead, *componentPtr);
                        }

                        componentFlagPtr = reinterpret_cast<DataFlag *>(bufferHead);
                        bufferHead += sizeof(DataFlag);
                    }

                }
                
                if (*dataFlagPtr == DataFlag::DATA_STRUCT) {
                    UStructProperty * structPtr = *reinterpret_cast<UStructProperty **>(bufferHead);
                    bufferHead += sizeof(UStructProperty *);

                    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Struct Prop Name: %s"), *structPtr->GetName()));


                    //UScriptStruct* scriptStruct = structPtr->Struct;
                    int structSize = structPtr->Struct->GetStructureSize();
                    void * StructAddress = structPtr->ContainerPtrToValuePtr<void>(*objectPtr);
                    FVector * vec = structPtr->ContainerPtrToValuePtr<FVector>(*objectPtr);
                    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Y: %f"), vec->X));
                    memcpy(StructAddress, bufferHead, structSize);
                    


                    FVector a;
                    memcpy(&a, bufferHead, structSize);
                    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Test Float Retrieve: %f"), a.X));
                    bufferHead += structSize;

                    //UScriptStruct* scriptStruct = Prop->Struct;
                    //void * StructAddress = Prop->ContainerPtrToValuePtr<void>(Elem);
                    //int structSize = scriptStruct->GetStructureSize();
                    //void *valuePtr = Prop->ContainerPtrToValuePtr<void>(Elem);


                    //// Put in Data Flag for Bool
                    //DataFlag struct_type = DataFlag::DATA_STRUCT;
                    //memcpy(bufferHead, &struct_type, sizeof(DataFlag));
                    //bufferHead += sizeof(DataFlag);

                    //memcpy(bufferHead, &Prop, sizeof(UStructProperty *));
                    //bufferHead += sizeof(UStructProperty *);

                    //// Int is constant size. Just serialize the value.
                    //memcpy(bufferHead, &valuePtr, structSize);
                    //bufferHead += structSize;
                }

                dataFlagPtr = reinterpret_cast<DataFlag *>(bufferHead);
                bufferHead += sizeof(DataFlag);
                
            }

            break;
        }
        break;
    }
    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, "Loop Ended");
}

uint8* ARollbackManager::serializeFlag(uint8* bufferHead, DataFlag flag)
{
    // Add Flag
    memcpy(bufferHead, &flag, sizeof(DataFlag));
    bufferHead += sizeof(DataFlag);
    return bufferHead;
}

uint8* ARollbackManager::serializePropName(uint8* bufferHead, FString propName)
{
    const TCHAR *serializedChar = *propName;

    // Accounting for the last null character in a string
    uint32 name_size = FCString::Strlen(serializedChar) + 1;

    // Put in size for name:
    memcpy(bufferHead, &name_size, sizeof(uint32));
    bufferHead += sizeof(uint32);

    // Put in buffer with string of size
    memcpy(bufferHead, TCHAR_TO_UTF8(serializedChar), name_size);
    bufferHead += name_size;

    return bufferHead;
}

uint8* ARollbackManager::deserializePropName(uint8* bufferHead, FString* propName)
{
    // Get Name Size
    uint32 *name_size = reinterpret_cast<uint32 *>(bufferHead);
    bufferHead += sizeof(uint32);

    // Get Name
    ANSICHAR *name_arr = reinterpret_cast<ANSICHAR *>(bufferHead);
    FString name = FString::FString(name_arr);
    bufferHead += *name_size;

    *propName = name;
    return bufferHead;
}

uint8* ARollbackManager::serializeBoolProp(uint8* bufferHead, UObject& object, UBoolProperty *prop)
{
    // Put in Data Flag for Bool
    DataFlag bool_type = DataFlag::DATA_BOOL;
    memcpy(bufferHead, &bool_type, sizeof(DataFlag));
    bufferHead += sizeof(DataFlag);

    memcpy(bufferHead, &prop, sizeof(UBoolProperty *));
    bufferHead += sizeof(UBoolProperty *);

    // Boolean is constant size. Just serialize the value.
    bool propVal = prop->GetPropertyValue_InContainer(&object);
    memcpy(bufferHead, &propVal, sizeof(bool));
    bufferHead += sizeof(bool);

    FString name = prop->GetName();
    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, name);
    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("%d"), propVal));

    return bufferHead;
}

uint8* ARollbackManager::deserializeBoolProp(uint8* bufferHead, UObject& object)
{
    FString propName;

    UBoolProperty *propPtr = *reinterpret_cast<UBoolProperty **>(bufferHead);
    bufferHead += sizeof(UBoolProperty *);

    // Read bool
    bool *propVal = reinterpret_cast<bool *>(bufferHead);
    bufferHead += sizeof(bool);

    propPtr->SetPropertyValue_InContainer(&object, *propVal);

    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, propPtr->GetName());
    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("%d"), *propVal));

    return bufferHead;
}

/**
 * Serializes bool prop from object
 * @param current buffer head. Should be at the beginning of a bool frame
 * @param object Object whose property value you want to serialize
 * @param prop bool property you are trying to serialize
 * @return buffer head at the end after data is serialized
 */
uint8* ARollbackManager::serializeIntProp(uint8* bufferHead, UObject& object, UIntProperty *prop)
{
    // Put in Data Flag for Bool
    DataFlag int_type = DataFlag::DATA_INT;
    memcpy(bufferHead, &int_type, sizeof(DataFlag));
    bufferHead += sizeof(DataFlag);

    memcpy(bufferHead, &prop, sizeof(UIntProperty *));
    bufferHead += sizeof(UIntProperty *);

    // Int is constant size. Just serialize the value.
    int32 propVal = prop->GetPropertyValue_InContainer(&object);
    memcpy(bufferHead, &propVal, sizeof(int32));
    bufferHead += sizeof(int32);

    FString name = prop->GetName();
    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, name);
    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("%d"), propVal));

    return bufferHead;
}

/**
* Deserializes bool prop from object
* @param current buffer head. Should be at the beginning of a bool frame
* @param object Object whose property you want to deserialize into
* @return buffer head at the end of the read data
*/
uint8* ARollbackManager::deserializeIntProp(uint8* bufferHead, UObject& object)
{
    UIntProperty *propPtr = *reinterpret_cast<UIntProperty **>(bufferHead);
    bufferHead += sizeof(UIntProperty *);

    // Read int
    int32 *propVal = reinterpret_cast<int *>(bufferHead);
    bufferHead += sizeof(int32);

    propPtr->SetPropertyValue_InContainer(&object, *propVal);

    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, propPtr->GetName());
    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("%d"), *propVal));

    return bufferHead;
}

uint8* ARollbackManager::serializeFloatProp(uint8* bufferHead, UObject& object, UFloatProperty *prop)
{
    // Put in Data Flag for Bool
    DataFlag float_type = DataFlag::DATA_FLOAT;
    memcpy(bufferHead, &float_type, sizeof(DataFlag));
    bufferHead += sizeof(DataFlag);

    memcpy(bufferHead, &prop, sizeof(UFloatProperty *));
    bufferHead += sizeof(UFloatProperty *);

    // Boolean is constant size. Just serialize the value.
    float propVal = prop->GetPropertyValue_InContainer(&object);
    memcpy(bufferHead, &propVal, sizeof(float));
    bufferHead += sizeof(float);

    FString name = prop->GetName();
    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, name);
    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("%f"), propVal));

    return bufferHead;
}

uint8* ARollbackManager::deserializeFloatProp(uint8* bufferHead, UObject& object)
{
    UFloatProperty *propPtr = *reinterpret_cast<UFloatProperty **>(bufferHead);
    bufferHead += sizeof(UFloatProperty *);

    // Read float
    float *propVal = reinterpret_cast<float *>(bufferHead);
    bufferHead += sizeof(float);

    propPtr->SetPropertyValue_InContainer(&object, *propVal);

    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, propPtr->GetName());
    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("%f"), *propVal));

    return bufferHead;
}


uint8* ARollbackManager::serializeStructProp(uint8* bufferHead, UObject& object, UStructProperty *prop)
{


    int structSize = prop->Struct->GetStructureSize();
    void * StructAddress = prop->ContainerPtrToValuePtr<void>(&object);
    memcpy(bufferHead, StructAddress, structSize);
    bufferHead += structSize;
    return bufferHead;
}
uint8* ARollbackManager::deserializeStructProp(uint8* bufferHead, UObject& object)
{
    UStructProperty * structPtr = *reinterpret_cast<UStructProperty **>(bufferHead);
    bufferHead += sizeof(UStructProperty *);

    int structSize = structPtr->Struct->GetStructureSize();
    void * StructAddress = structPtr->ContainerPtrToValuePtr<void>(&object);
    memcpy(StructAddress, bufferHead, structSize);
    bufferHead += structSize;
    return bufferHead;
}


PRAGMA_ENABLE_OPTIMIZATION

