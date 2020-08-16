// Fill out your copyright notice in the Description page of Project Settings.

PRAGMA_DISABLE_OPTIMIZATION
#include "RollbackManager.h"
#include "Serialization/BufferArchive.h"

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

    if (logSerialization) {
        UE_LOG(LogTemp, Warning, TEXT("Serialization Info"));
    }

    for (TPair<AActor*, int>& Pair : actorMap)
    {
        if (Pair.Value != -1) {
            // Don't serialize objects pending kill
            continue;
        }
        AActor* actor = Pair.Key;

        bufferHead = serializeFlag(bufferHead, DataFlag::ACTOR);

        // Add Pointer
        memcpy(bufferHead, &actor, sizeof(AActor*));
        bufferHead += sizeof(AActor*);
        if (logSerialization) {
            UE_LOG(LogTemp, Warning, TEXT("%p"), actor);
        }

        bufferHead = serializeActorTransform(bufferHead, *actor);

        // Serialize components?
        TArray<UActorComponent *> components;
        actor->GetComponents(components, false);
        for (UActorComponent *component : components) { 
            if (component->ComponentHasTag(TEXT("Rollback"))) {
                bufferHead = serializeActorComponent(bufferHead, component);
            }
        }

        bufferHead = serializeProps(bufferHead, *actor);
    }
    DataFlag endFlag = DataFlag::END;
    memcpy(bufferHead, &endFlag, sizeof(DataFlag));
    bufferHead += sizeof(DataFlag);
}

void ARollbackManager::restoreObjects()
{
    uint8 *bufferHead = saveGameBuffer;
    if (logSerialization) {
        UE_LOG(LogTemp, Warning, TEXT("Deserialization Info"));
    }

    if (logSerialization) {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, "Restoring State");
    }

    bool actorLoopBool = true;
    int actorCount = 0;
    while (actorLoopBool) {

        bool flagReadSuccess = false;
        bufferHead = deserializeFlag(bufferHead, DataFlag::ACTOR, &flagReadSuccess);
        actorLoopBool = flagReadSuccess;
        if (!actorLoopBool) {
            continue;
        }
        actorCount += 1;

        AActor* actorPtr = *reinterpret_cast<AActor**>(bufferHead);
        bufferHead += sizeof(AActor*);

        int *value = actorMap.Find(actorPtr);
            
        if (!value) {
            GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, "Actor Ptr Not Found! Something went wrong E:");
        }
        deserializedMap.Add(actorPtr, true);

        bufferHead = deserializeActorTransform(bufferHead, *actorPtr);
        bufferHead = deserializeProps(bufferHead, *actorPtr);

    }

    // Used to check whether the number of actors we keep track of is exploding!
    //GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("NumActorDeserialized: %i"), actorCount));

    // CLean up actormap and deserialized map in case certain actors did not exists beforehand.
    for (auto Elem = deserializedMap.CreateConstIterator(); Elem; ++Elem)
    {
        if (!Elem.Value()) {
            // Actor was not restored or was pending kill. Remove from map and delete.
            DestroyActor(Elem.Key());
        }
        else {
            // switch back to false
            deserializedMap.Add(Elem.Key(), false);
        }

        // Was pending destroy, but is now alive
        if (actorMap.Contains(Elem.Key()) && *actorMap.Find(Elem.Key()) != -1) {
            // Show actor in game
            ShowActor(Elem.Key());
            // Restore to alive state.
            AddActor(Elem.Key());
        }
    }

}

void ARollbackManager::saveToSlot()
{
    serializeObjects();
    memcpy(saveGameSlot, saveGameBuffer, BUFSIZE);
}

void ARollbackManager::restoreFromSlot()
{
    memcpy(saveGameBuffer, saveGameSlot, BUFSIZE);
    restoreObjects();
}

uint8* ARollbackManager::serializeFlag(uint8* bufferHead, DataFlag flag)
{
    // Add Flag
    memcpy(bufferHead, &flag, sizeof(DataFlag));
    bufferHead += sizeof(DataFlag);
    if (logSerialization) {
        UE_LOG(LogTemp, Warning, TEXT("DataFlag::%i"), flag);
    }

    return bufferHead;
}

uint8* ARollbackManager::deserializeFlag(uint8* bufferHead, DataFlag flag, bool *success)
{
    // Add Flag
    DataFlag dataFlag = *reinterpret_cast<DataFlag *>(bufferHead);

    if (dataFlag != flag) {
        *success = false;
        return bufferHead;
    }

    if (logSerialization) {
        UE_LOG(LogTemp, Warning, TEXT("DataFlag:: %i"), flag);
    }
    bufferHead += sizeof(DataFlag);

    *success = true;
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
    bufferHead = serializeFlag(bufferHead, DataFlag::DATA_BOOL);

    memcpy(bufferHead, &prop, sizeof(UBoolProperty *));
    bufferHead += sizeof(UBoolProperty *);
    if (logSerialization) {
        UE_LOG(LogTemp, Warning, TEXT("%s"), *prop->GetName());
    }

    // Boolean is constant size. Just serialize the value.
    bool propVal = prop->GetPropertyValue_InContainer(&object);
    memcpy(bufferHead, &propVal, sizeof(bool));
    bufferHead += sizeof(bool);
    if (logSerialization) {
        UE_LOG(LogTemp, Warning, TEXT("%i"), propVal);
    }

    return bufferHead;
}

uint8* ARollbackManager::deserializeBoolProp(uint8* bufferHead, UObject& object, bool *success)
{

    bool flagReadSuccess = false;
    bufferHead = deserializeFlag(bufferHead, DataFlag::DATA_BOOL, &flagReadSuccess);
    *success = flagReadSuccess;
    if (!flagReadSuccess) {
        return bufferHead;
    }

    UBoolProperty *propPtr = *reinterpret_cast<UBoolProperty **>(bufferHead);
    bufferHead += sizeof(UBoolProperty *);

    if (logSerialization) {
        UE_LOG(LogTemp, Warning, TEXT("%s"), *propPtr->GetName());
    }

    // Read bool
    bool propVal = *reinterpret_cast<bool *>(bufferHead);
    bufferHead += sizeof(bool);
    if (logSerialization) {
        UE_LOG(LogTemp, Warning, TEXT("%i"), propVal);
    }

    propPtr->SetPropertyValue_InContainer(&object, propVal);

    return bufferHead;
}

/**
 * Serializes int prop from object
 * @param current buffer head. Should be at the beginning of a bool frame
 * @param object Object whose property value you want to serialize
 * @param prop bool property you are trying to serialize
 * @return buffer head at the end after data is serialized
 */
uint8* ARollbackManager::serializeIntProp(uint8* bufferHead, UObject& object, UIntProperty *prop)
{
    // Put in Data Flag for Bool
    bufferHead = serializeFlag(bufferHead, DataFlag::DATA_INT);

    memcpy(bufferHead, &prop, sizeof(UIntProperty *));
    bufferHead += sizeof(UIntProperty *);
    if (logSerialization) {
        UE_LOG(LogTemp, Warning, TEXT("%s"), *prop->GetName());
    }

    // Int is constant size. Just serialize the value.
    int32 propVal = prop->GetPropertyValue_InContainer(&object);
    memcpy(bufferHead, &propVal, sizeof(int32));
    bufferHead += sizeof(int32);
    if (logSerialization) {
        UE_LOG(LogTemp, Warning, TEXT("%i"), propVal);
    }

    return bufferHead;
}

/**
* Deserializes int prop from object
* @param current buffer head. Should be at the beginning of a bool frame
* @param object Object whose property you want to deserialize into
* @return buffer head at the end of the read data
*/
uint8* ARollbackManager::deserializeIntProp(uint8* bufferHead, UObject& object, bool *success)
{
    bool flagReadSuccess = false;
    bufferHead = deserializeFlag(bufferHead, DataFlag::DATA_INT, &flagReadSuccess);
    *success = flagReadSuccess;
    if (!flagReadSuccess) {
        return bufferHead;
    }

    UIntProperty *propPtr = *reinterpret_cast<UIntProperty **>(bufferHead);
    bufferHead += sizeof(UIntProperty *);
    if (logSerialization) {
        UE_LOG(LogTemp, Warning, TEXT("%s"), *propPtr->GetName());
    }

    // Read int
    int32 propVal = *reinterpret_cast<int *>(bufferHead);
    bufferHead += sizeof(int32);

    propPtr->SetPropertyValue_InContainer(&object, propVal);

    if (logSerialization) {
        UE_LOG(LogTemp, Warning, TEXT("%i"), propVal);
    }

    return bufferHead;
}

uint8* ARollbackManager::serializeFloatProp(uint8* bufferHead, UObject& object, UFloatProperty *prop)
{
    // Put in Data Flag for Bool
    bufferHead = serializeFlag(bufferHead, DataFlag::DATA_FLOAT);

    memcpy(bufferHead, &prop, sizeof(UFloatProperty *));
    bufferHead += sizeof(UFloatProperty *);
    if (logSerialization) {
        UE_LOG(LogTemp, Warning, TEXT("%s"), *prop->GetName());
    }

    // Float is constant size. Just serialize the value.
    float propVal = prop->GetPropertyValue_InContainer(&object);
    memcpy(bufferHead, &propVal, sizeof(float));
    bufferHead += sizeof(float);
    if (logSerialization) {
        UE_LOG(LogTemp, Warning, TEXT("%f"), propVal);
    }

    return bufferHead;
}

uint8* ARollbackManager::deserializeFloatProp(uint8* bufferHead, UObject& object, bool *success)
{
    bool flagReadSuccess = false;
    bufferHead = deserializeFlag(bufferHead, DataFlag::DATA_FLOAT, &flagReadSuccess);
    *success = flagReadSuccess;
    if (!flagReadSuccess) {
        return bufferHead;
    }

    UFloatProperty *propPtr = *reinterpret_cast<UFloatProperty **>(bufferHead);
    bufferHead += sizeof(UFloatProperty *);
    if (logSerialization) {
        UE_LOG(LogTemp, Warning, TEXT("%s"), *propPtr->GetName());
    }

    // Read float
    float propVal = *reinterpret_cast<float *>(bufferHead);
    bufferHead += sizeof(float);
    propPtr->SetPropertyValue_InContainer(&object, propVal);
    if (logSerialization) {
        UE_LOG(LogTemp, Warning, TEXT("%f"), propVal);
    }

    return bufferHead;
}


uint8* ARollbackManager::serializeStructProp(uint8* bufferHead, UObject& object, UStructProperty *prop)
{
    // Put in Data Flag for Bool
    bufferHead = serializeFlag(bufferHead, DataFlag::DATA_STRUCT);
    
    // Copy Struct Pointer
    memcpy(bufferHead, &prop, sizeof(UStructProperty *));
    bufferHead += sizeof(UStructProperty *);
    if (logSerialization) {
        UE_LOG(LogTemp, Warning, TEXT("%s"), *prop->GetName());
    }

    // Shallow copy struct info
    int structSize = prop->Struct->GetStructureSize();
    void * StructAddress = prop->ContainerPtrToValuePtr<void>(&object);
    memcpy(bufferHead, StructAddress, structSize);

    if (logSerialization) {
        UE_LOG(LogTemp, Warning, TEXT("Pointer: %p"), StructAddress);
    }

    bufferHead += structSize;
    return bufferHead;
}

uint8* ARollbackManager::deserializeStructProp(uint8* bufferHead, UObject& object, bool *success)
{
    bool flagReadSuccess = false;
    bufferHead = deserializeFlag(bufferHead, DataFlag::DATA_STRUCT, &flagReadSuccess);
    *success = flagReadSuccess;
    if (!flagReadSuccess) {
        return bufferHead;
    }

    // Get UStructProperty ptr
    UStructProperty * structPtr = *reinterpret_cast<UStructProperty **>(bufferHead);
    bufferHead += sizeof(UStructProperty *);
    if (logSerialization) {
        UE_LOG(LogTemp, Warning, TEXT("%s"), *structPtr->GetName());
    }

    // Copy struct data from buffer.
    int structSize = structPtr->Struct->GetStructureSize();
    void * StructAddress = structPtr->ContainerPtrToValuePtr<void>(&object);
    memcpy(StructAddress, bufferHead, structSize);
    bufferHead += structSize;
    if (logSerialization) {
        UE_LOG(LogTemp, Warning, TEXT("Pointer: %p"), StructAddress);
    }

    return bufferHead;
}

uint8* ARollbackManager::serializeDelegateProp(uint8* bufferHead, UObject& object, UDelegateProperty *prop)
{
    // Put in Data Flag for Bool
    bufferHead = serializeFlag(bufferHead, DataFlag::DATA_DELEGATE);

    memcpy(bufferHead, &prop, sizeof(UDelegateProperty *));
    bufferHead += sizeof(UDelegateProperty *);
    if (logSerialization) {
        UE_LOG(LogTemp, Warning, TEXT("%s"), *prop->GetName());
    }

    FScriptDelegate propVal = prop->GetPropertyValue_InContainer(&object);
    memcpy(bufferHead, &propVal, sizeof(FScriptDelegate));
    bufferHead += sizeof(FScriptDelegate);
    if (logSerialization) {
        UE_LOG(LogTemp, Warning, TEXT("%s"), *propVal.GetFunctionName().ToString());
    }

    return bufferHead;
}

uint8* ARollbackManager::deserializeDelegateProp(uint8* bufferHead, UObject& object, bool *success)
{
    bool flagReadSuccess = false;
    bufferHead = deserializeFlag(bufferHead, DataFlag::DATA_DELEGATE, &flagReadSuccess);
    *success = flagReadSuccess;
    if (!flagReadSuccess) {
        return bufferHead;
    }

    UDelegateProperty *propPtr = *reinterpret_cast<UDelegateProperty **>(bufferHead);
    bufferHead += sizeof(UDelegateProperty *);
    if (logSerialization) {
        UE_LOG(LogTemp, Warning, TEXT("%s"), *propPtr->GetName());
    }

    // Read Delegate
    FScriptDelegate propVal = *reinterpret_cast<FScriptDelegate *>(bufferHead);
    bufferHead += sizeof(FScriptDelegate);
    propPtr->SetPropertyValue_InContainer(&object, propVal);
    if (logSerialization) {
        UE_LOG(LogTemp, Warning, TEXT("%s"), *propVal.GetFunctionName().ToString());
    }

    return bufferHead;
}

uint8* ARollbackManager::serializeByteProp(uint8* bufferHead, UObject& object, UByteProperty *prop)
{
    //// Put in Data Flag for Bool
    bufferHead = serializeFlag(bufferHead, DataFlag::DATA_BYTE);

    memcpy(bufferHead, &prop, sizeof(UByteProperty *));
    bufferHead += sizeof(UByteProperty *);
    if (logSerialization) {
        UE_LOG(LogTemp, Warning, TEXT("%s"), *prop->GetName());
    }

    // Byte is constant size. Just serialize the value.
    uint8 propVal = prop->GetPropertyValue_InContainer(&object);
    memcpy(bufferHead, &propVal, sizeof(uint8));
    bufferHead += sizeof(uint8);
    if (logSerialization) {
        UE_LOG(LogTemp, Warning, TEXT("%i"), propVal);
    }

    return bufferHead;
}
uint8* ARollbackManager::deserializeByteProp(uint8* bufferHead, UObject& object, bool *success)
{
    bool flagReadSuccess = false;
    bufferHead = deserializeFlag(bufferHead, DataFlag::DATA_BYTE, &flagReadSuccess);
    *success = flagReadSuccess;
    if (!flagReadSuccess) {
        return bufferHead;
    }

    UByteProperty *propPtr = *reinterpret_cast<UByteProperty **>(bufferHead);
    bufferHead += sizeof(UByteProperty *);
    if (logSerialization) {
        UE_LOG(LogTemp, Warning, TEXT("%s"), *propPtr->GetName());
    }

    // Read float
    uint8 propVal = *reinterpret_cast<uint8 *>(bufferHead);
    bufferHead += sizeof(uint8);
    propPtr->SetPropertyValue_InContainer(&object, propVal);
    if (logSerialization) {
        UE_LOG(LogTemp, Warning, TEXT("%f"), propVal);
    }

    return bufferHead;
}

uint8* ARollbackManager::serializeObjectProp(uint8* bufferHead, UObject& object, UObjectProperty* prop)
{
    //// Put in Data Flag for Bool
    bufferHead = serializeFlag(bufferHead, DataFlag::DATA_OBJECT);

    memcpy(bufferHead, &prop, sizeof(UObjectProperty*));
    bufferHead += sizeof(UObjectProperty*);
    if (logSerialization) {
        UE_LOG(LogTemp, Warning, TEXT("%s"), *prop->GetName());
    }

    // Just save the pointer. Assume pointer is still valid (should be!) Otherwise we'll get an error.
    UObject *propVal = prop->GetPropertyValue_InContainer(&object);
    memcpy(bufferHead, &propVal, sizeof(UObject*));
    bufferHead += sizeof(UObject*);
    if (logSerialization) {
        UE_LOG(LogTemp, Warning, TEXT("%p"), propVal);
    }

    return bufferHead;
}
uint8* ARollbackManager::deserializeObjectProp(uint8* bufferHead, UObject& object, bool* success)
{
    bool flagReadSuccess = false;
    bufferHead = deserializeFlag(bufferHead, DataFlag::DATA_OBJECT, &flagReadSuccess);
    *success = flagReadSuccess;
    if (!flagReadSuccess) {
        return bufferHead;
    }

    UObjectProperty* propPtr = *reinterpret_cast<UObjectProperty**>(bufferHead);
    bufferHead += sizeof(UByteProperty*);
    if (logSerialization) {
        UE_LOG(LogTemp, Warning, TEXT("%s"), *propPtr->GetName());
    }

    // Read object prop
    UObject *propVal = *reinterpret_cast<UObject **>(bufferHead);
    bufferHead += sizeof(UObject*);
    propPtr->SetPropertyValue_InContainer(&object, propVal);
    if (!IsValid(propVal)) {
        // Reference might just not be set...?
        //UE_LOG(LogTemp, Warning, TEXT("OBJECT IS NOT VALID D: var:%s"), *propPtr->GetName());
    }
    if (logSerialization) {
        UE_LOG(LogTemp, Warning, TEXT("%f"), propVal);
    }

    return bufferHead;
}

uint8* ARollbackManager::serializeArrayProp(uint8* bufferHead, UObject& object, UArrayProperty* prop)
{
    FScriptArray *propVal = prop->GetPropertyValuePtr_InContainer(&object);
    UProperty *innerProp = prop->Inner;

    int32 numBytesPerElement = -1;

    UObjectProperty* objectPtr = Cast<UObjectProperty>(innerProp);
    if (objectPtr) {
        numBytesPerElement = sizeof(UObject*);
    }

    UByteProperty* bytePtr = Cast<UByteProperty>(innerProp);
    if (bytePtr) {
        numBytesPerElement = sizeof(uint8);
    }

    UDelegateProperty* delegatePtr = Cast<UDelegateProperty>(innerProp);
    if (delegatePtr) {
        numBytesPerElement = sizeof(FScriptDelegate);
    }

    UIntProperty* intPtr = Cast<UIntProperty>(innerProp);
    if (intPtr) {
        numBytesPerElement = sizeof(int32);
    }

    UFloatProperty* floatPtr = Cast<UFloatProperty>(innerProp);
    if (floatPtr) {
        numBytesPerElement = sizeof(float);
    }

    UStructProperty* structPtr = Cast<UStructProperty>(innerProp);
    if (structPtr) {
        numBytesPerElement = structPtr->Struct->GetStructureSize();
    }

    UBoolProperty* boolPtr = Cast<UBoolProperty>(innerProp);
    if (boolPtr) {
        numBytesPerElement = sizeof(bool);
    }

    if (numBytesPerElement == -1) {
        // Unsupported array type. Log and return;
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Unsupported Array Type %s"), *innerProp->GetClass()->GetName()));
        UE_LOG(LogTemp, Warning, TEXT("Unsupported Array Type %s"), *innerProp->GetClass()->GetName());
        return bufferHead;
    }

    // Put in Data Flag for Array
    bufferHead = serializeFlag(bufferHead, DataFlag::DATA_ARRAY);

    // Store pointer to array prop
    memcpy(bufferHead, &prop, sizeof(UArrayProperty*));
    bufferHead += sizeof(UArrayProperty*);
    if (logSerialization) {
        UE_LOG(LogTemp, Warning, TEXT("%s"), *prop->GetName());
    }

    // Store the number of elements
    int32 numElements = propVal->Num();
    memcpy(bufferHead, &numElements, sizeof(int32));
    bufferHead += sizeof(int32);
    if (logSerialization) {
        UE_LOG(LogTemp, Warning, TEXT("%i"), numElements);
    }

    // Store the size of each element so we don't have to do this logic again
    memcpy(bufferHead, &numBytesPerElement, sizeof(int32));
    bufferHead += sizeof(int32);
    if (logSerialization) {
        UE_LOG(LogTemp, Warning, TEXT("%i"), numBytesPerElement);
    }

    // Store data in the allocated array
    memcpy(bufferHead, propVal->GetData(), numBytesPerElement * propVal->Num());
    bufferHead += numBytesPerElement * propVal->Num();

    return bufferHead;
}
uint8* ARollbackManager::deserializeArrayProp(uint8* bufferHead, UObject& object, bool* success)
{
    // Read array flag
    bool flagReadSuccess = false;
    bufferHead = deserializeFlag(bufferHead, DataFlag::DATA_ARRAY, &flagReadSuccess);
    *success = flagReadSuccess;
    if (!flagReadSuccess) {
        return bufferHead;
    }

    // Read array property ptr
    UArrayProperty* propPtr = *reinterpret_cast<UArrayProperty**>(bufferHead);
    bufferHead += sizeof(UArrayProperty*);
    if (logSerialization) {
        UE_LOG(LogTemp, Warning, TEXT("%s"), *propPtr->GetName());
    }

    // Read num elements
    int numElements = *reinterpret_cast<int32*>(bufferHead);
    bufferHead += sizeof(int32);
    if (logSerialization) {
        UE_LOG(LogTemp, Warning, TEXT("%i"), numElements);
    }

    // Read numBytesPerElement
    int numBytesPerElement = *reinterpret_cast<int32*>(bufferHead);
    bufferHead += sizeof(int32);
    if (logSerialization) {
        UE_LOG(LogTemp, Warning, TEXT("%i"), numBytesPerElement);
    }

    // Read data from array?
    FScriptArray* propVal = propPtr->GetPropertyValuePtr_InContainer(&object);

    void *data = reinterpret_cast<void*>(bufferHead);
    bufferHead += numElements * numBytesPerElement;
    
    // Just empty the array please! Set the size to the current size. Shouldn't need to allocate new memory then.
    propVal->Empty(propVal->Num() + propVal->GetSlack(), numBytesPerElement);
    // Set array num to number of elements. Realloc I guess if you need to at this point.
    propVal->Add(numElements, numBytesPerElement);
    // Copy data into allocated memory
    memcpy(propVal->GetData(), data, numElements * numBytesPerElement);

    return bufferHead;
}

uint8* ARollbackManager::serializeProps(uint8* bufferHead, UObject& object)
{
    UClass *objectClass = object.GetClass();
    for (TFieldIterator<UProperty> Prop(objectClass); Prop; ++Prop) {
        EPropertyFlags flags = Prop->GetPropertyFlags();
        UProperty * propPtr = *Prop;
        if ((flags & CPF_SaveGame) > 0) {

            //UE_LOG(LogTemp, Warning, TEXT("%s"), *propPtr->GetName());
            //UE_LOG(LogTemp, Warning, TEXT("%s"), *propPtr->GetClass()->GetName());

            UArrayProperty* arrPtr = Cast<UArrayProperty>(propPtr);
            if (arrPtr) {
                bufferHead = serializeArrayProp(bufferHead, object, arrPtr);
                continue;
            }


            UObjectProperty* objectPtr = Cast<UObjectProperty>(propPtr);
            if (objectPtr) {
                bufferHead = serializeObjectProp(bufferHead, object, objectPtr);
                continue;
            }

            UByteProperty *bytePtr = Cast<UByteProperty>(propPtr);
            if (bytePtr) {
                bufferHead = serializeByteProp(bufferHead, object, bytePtr);
                continue;
            }

            UDelegateProperty *delegatePtr = Cast<UDelegateProperty>(propPtr);
            if (delegatePtr) {
                bufferHead = serializeDelegateProp(bufferHead, object, delegatePtr);
                continue;
            }

            UBoolProperty *boolPtr = Cast<UBoolProperty>(propPtr);
            if (boolPtr) {
                bufferHead = serializeBoolProp(bufferHead, object, boolPtr);
                continue;
            }

            UIntProperty *intPtr = Cast<UIntProperty>(propPtr);
            if (intPtr) {
                bufferHead = serializeIntProp(bufferHead, object, intPtr);
                continue;
            }

            UFloatProperty *floatPtr = Cast<UFloatProperty>(propPtr);
            if (floatPtr) {
                bufferHead = serializeFloatProp(bufferHead, object, floatPtr);
                continue;
            }

            UStructProperty *structPtr = Cast<UStructProperty>(propPtr);
            if (structPtr) {
                bufferHead = serializeStructProp(bufferHead, object, structPtr);
                continue;
            }
        }
    }

    return bufferHead;
}

uint8* ARollbackManager::deserializeProps(uint8* bufferHead, UObject& object)
{
    bool propReadSuccess = true;

    while (propReadSuccess) {
        propReadSuccess = false;

        bool success = false;
        bufferHead = deserializeBoolProp(bufferHead, object, &success);
        if (success) {
            propReadSuccess = true;
        }
        bufferHead = deserializeIntProp(bufferHead, object, &success);
        if (success) {
            propReadSuccess = true;
        }
        bufferHead = deserializeFloatProp(bufferHead, object, &success);
        if (success) {
            propReadSuccess = true;
        }

        bufferHead = deserializeStructProp(bufferHead, object, &success);
        if (success) {
            propReadSuccess = true;
        }

        bufferHead = deserializeDelegateProp(bufferHead, object, &success);
        if (success) {
            propReadSuccess = true;
        }

        bufferHead = deserializeByteProp(bufferHead, object, &success);
        if (success) {
            propReadSuccess = true;
        }

        bufferHead = deserializeObjectProp(bufferHead, object, &success);
        if (success) {
            propReadSuccess = true;
        }

        bufferHead = deserializeArrayProp(bufferHead, object, &success);
        if (success) {
            propReadSuccess = true;
        }

        bufferHead = deserializeActorComponent(bufferHead, &success);
        if (success) {
            propReadSuccess = true;
        }

    }

    return bufferHead;
}

uint8* ARollbackManager::serializeActorComponent(uint8* bufferHead, UActorComponent *component)
{
    // Add component tag
    bufferHead = serializeFlag(bufferHead, DataFlag::COMPONENT);

    //Add ActorComponent Pointer
    memcpy(bufferHead, &component, sizeof(UActorComponent *));
    bufferHead += sizeof(UActorComponent *);

    if (logSerialization) {
        UE_LOG(LogTemp, Warning, TEXT("%s, %p"), *component->GetName(), component);
    }

    bufferHead = serializeProps(bufferHead, *component);

    // Add component end tag
    bufferHead = serializeFlag(bufferHead, DataFlag::COMPONENT_END);

    return bufferHead;
}

uint8* ARollbackManager::deserializeActorComponent(uint8* bufferHead, bool *success)
{
    bool flagReadSuccess = false;
    bufferHead = deserializeFlag(bufferHead, DataFlag::COMPONENT, &flagReadSuccess);
    *success = flagReadSuccess;
    if (!flagReadSuccess) {
        return bufferHead;
    }

    UActorComponent * componentPtr = *reinterpret_cast<UActorComponent **>(bufferHead);
    bufferHead += sizeof(UActorComponent *);

    if (logSerialization) {
        UE_LOG(LogTemp, Warning, TEXT("%s: %p"), *componentPtr->GetName(), componentPtr);
    }

    //read props here!

    bufferHead = deserializeProps(bufferHead, *componentPtr);

    bool flagEndRead = false;
    bufferHead = deserializeFlag(bufferHead, DataFlag::COMPONENT_END, &flagEndRead);
    if (!flagEndRead) {
        UE_LOG(LogTemp, Warning, TEXT("COMPONENT_END NOT FOUND. SOMETHING WENT WRONG"));
    }

    return bufferHead;
}

uint8* ARollbackManager::serializeActorTransform(uint8* bufferHead, AActor& actor)
{
    FVector location = actor.GetActorLocation();
    // Serialize actor location
    //copy x y z
    memcpy(bufferHead, &location.X, sizeof(float));
    bufferHead += sizeof(float);
    memcpy(bufferHead, &location.Y, sizeof(float));
    bufferHead += sizeof(float);
    memcpy(bufferHead, &location.Z, sizeof(float));
    bufferHead += sizeof(float);

    //Serialize actor rotation
    FRotator rotation = actor.GetActorRotation();
    //copy Pitch Row and Yaw
    memcpy(bufferHead, &rotation.Pitch, sizeof(float));
    bufferHead += sizeof(float);
    memcpy(bufferHead, &rotation.Roll, sizeof(float));
    bufferHead += sizeof(float);
    memcpy(bufferHead, &rotation.Yaw, sizeof(float));
    bufferHead += sizeof(float);

    return bufferHead;
}

uint8* ARollbackManager::deserializeActorTransform(uint8* bufferHead, AActor& actor)
{
    // Reset location x y z
    float *locX = reinterpret_cast<float *>(bufferHead);
    bufferHead += sizeof(float);

    float *locY = reinterpret_cast<float *>(bufferHead);
    bufferHead += sizeof(float);

    float *locZ = reinterpret_cast<float *>(bufferHead);
    bufferHead += sizeof(float);

    FVector location = FVector::FVector(*locX, *locY, *locZ);
    actor.SetActorLocation(location);

    // Reset rotation row pitch yaw
    float *rotPitch = reinterpret_cast<float *>(bufferHead);
    bufferHead += sizeof(float);

    float *rotRoll = reinterpret_cast<float *>(bufferHead);
    bufferHead += sizeof(float);

    float *rotYaw = reinterpret_cast<float *>(bufferHead);
    bufferHead += sizeof(float);

    FRotator rotation = FRotator::FRotator(*rotPitch, *rotYaw, *rotRoll);

    actor.SetActorRotation(rotation);

    return bufferHead;
}

void ARollbackManager::HideActor(AActor* actor)
{
    if (!IsValid(actor)) {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, "InvalidActor in Hide Actor");
        UE_LOG(LogTemp, Warning, TEXT("Invalid Actor in Hide Actor"));
        return;
    }
    actor->SetActorEnableCollision(false);
    actor->SetActorHiddenInGame(true);
    // Shouldn't be needed. Just in case.
    actor->SetActorTickEnabled(false);
}

void ARollbackManager::ShowActor(AActor* actor)
{
    if (!IsValid(actor)) {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, "InvalidActor in Show Actor");
        UE_LOG(LogTemp, Warning, TEXT("Invalid Actor in Show Actor"));
        return;
    }
    actor->SetActorEnableCollision(true);
    actor->SetActorHiddenInGame(false);
    // Shouldn't be needed. Just in case.
    actor->SetActorTickEnabled(true);
}

void ARollbackManager::AddActor(AActor* actor)
{
    //GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Adding Actor %s"), *actor->GetName()));
    actorMap.Add(actor, -1);
    deserializedMap.Add(actor, false);
}

void ARollbackManager::KillActor(AActor* actor)
{
    // change to start counting down
    //GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, "Killing Actor");
    actorMap.Add(actor, GGPOPredictionBarrier);
    HideActor(actor);
}

void ARollbackManager::DestroyActor(AActor* actor)
{
    if (!IsValid(actor)) {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, "InvalidActor in Destroy Actor");
        UE_LOG(LogTemp, Warning, TEXT("InvalidActor in Destroy Actor"));
        return;
    }
    //GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Destroying Actor %s"), *actor->GetName()));
    int numRemoved = 0;
    numRemoved = actorMap.Remove(actor);
    if (numRemoved == 0) {
        //GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, "Actor Not Found in actorMap");
        UE_LOG(LogTemp, Warning, TEXT("Actor Not Found in actorMap"));
    }
    numRemoved = deserializedMap.Remove(actor);
    if (numRemoved == 0) {
        //GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, "Actor Not Found in deserializeMap");
        UE_LOG(LogTemp, Warning, TEXT("Actor Not Found in deserializeMap"));
    }
    actor->Destroy();
    //GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, "Actor Removed");
    //UE_LOG(LogTemp, Warning, TEXT("Actor Removed"));
}

void ARollbackManager::NetworkTick()
{

    for (auto Elem = actorMap.CreateConstIterator(); Elem; ++Elem)
    {
        if (Elem.Value() > 0) {
            // Decrement frames to delete
            actorMap.Add(Elem.Key(), Elem.Value() - 1);
        }
        else if (Elem.Value() == 0) {
            DestroyActor(Elem.Key());
        }
    }
}

PRAGMA_ENABLE_OPTIMIZATION

