// Fill out your copyright notice in the Description page of Project Settings.

#include "GGPOInterface.h"
#include "Kismet/GameplayStatics.h"

// Sets default values for this component's properties
UGGPOInterface::UGGPOInterface()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}

// Called when the game starts
void UGGPOInterface::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}

// Called when the game ends
void UGGPOInterface::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
    EndSession();
    // ...

}



// Called every frame
void UGGPOInterface::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

namespace GGPOInterface {
    GGPOSession* ggpo;
    GGPOErrorCode result;
    GGPOSessionCallbacks cb;
    bool debugLog = false;

    UGGPOInterface* self;
}

using namespace GGPOInterface;

bool __cdecl beginGame(const char *game) {
    
    if (debugLog) UE_LOG(LogTemp, Warning, TEXT("Begin Game"));
    return true;
}

bool __cdecl saveGameState(unsigned char **buffer, int *len, int *checksum, int frame)
{
    GGPOInterface::self->rollbackManager->serializeObjects();
    unsigned char* allocBuffer = (unsigned char *)malloc(self->rollbackManager->BUFSIZE);
    *len = self->rollbackManager->BUFSIZE;
    memcpy(allocBuffer, self->rollbackManager->saveGameBuffer, self->rollbackManager->BUFSIZE);

    *buffer = allocBuffer;

    if (debugLog) UE_LOG(LogTemp, Warning, TEXT("Saving Game State"));
    return true;
}

bool __cdecl loadGameState(unsigned char *buffer, int len)
{
    memcpy(self->rollbackManager->saveGameBuffer, buffer, len);
    self->rollbackManager->restoreObjects();
    //GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, "LoadGameState");
    if (debugLog) UE_LOG(LogTemp, Warning, TEXT("Loading Game State"));
    return true;
}

void __cdecl freeBuffer(void *buffer)
{
    free(buffer);
    if (debugLog) UE_LOG(LogTemp, Warning, TEXT("Free Buffer"));
    return;
}

bool __cdecl advanceFrame(int flags)
{
    int disconnectFlag;
    
    // player inputs
    int16 p[2];

    // Make sure we fetch new inputs from GGPO and use those to update
    // the game state instead of reading from the keyboard.
    result = ggpo_synchronize_input(ggpo,       // the session object
        p,                                      // array of inputs
        sizeof(p),                              // size of all inputs
        &disconnectFlag);

    self->GameAdvanceFrame(p, disconnectFlag);

    if (debugLog) UE_LOG(LogTemp, Warning, TEXT("Advance Frame"));
    //GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, "Advance Frame");
    ggpo_advance_frame(ggpo);
    return true;
}

bool __cdecl onEvent(GGPOEvent *info)
{
    UE_LOG(LogTemp, Warning, TEXT("Event Code %i"), info->code);
    return true;
}

void UGGPOInterface::GameTick(FGGPOInputFrame localInput)
{
    if (!sessionStarted) {
        if (debugLog) UE_LOG(LogTemp, Warning, TEXT("Tried to Game Tick. Session not yet started"));
        return;
    }
    int disconnectFlag = 0;
    int16 p[2];

    int16 testInput = convertInputFrameToInt16(localInput);
    if (debugLog) UE_LOG(LogTemp, Warning, TEXT("Test Input Convert %i"), testInput);

    if (playerNumber == 0) {
        if (debugLog) UE_LOG(LogTemp, Warning, TEXT("Game Tick P0"));
        p[0] = convertInputFrameToInt16(localInput);
        if (player_handles[0] == GGPO_INVALID_HANDLE) {
            if (debugLog) UE_LOG(LogTemp, Warning, TEXT("Invalid Player Handle"));
        }

        /* notify ggpo of the local player's inputs */
        result = ggpo_add_local_input(ggpo,               // the session object
            player_handles[0],  // handle for p1
            &p[0],              // p1's inputs
            sizeof(p[0]));      // size of p1's inputs
    }
    else {
        if (debugLog) UE_LOG(LogTemp, Warning, TEXT("Game Tick P1"));
        p[1] = convertInputFrameToInt16(localInput);

        if (player_handles[1] == GGPO_INVALID_HANDLE) {
            if (debugLog) UE_LOG(LogTemp, Warning, TEXT("Invalid Player Handle"));
        }

        /* notify ggpo of the local player's inputs */
        result = ggpo_add_local_input(ggpo,               // the session object
            player_handles[1],  // handle for p1
            &p[1],              // p1's inputs
            sizeof(p[1]));      // size of p1's inputs

    }

    /* synchronize the local and remote inputs */
    if (GGPO_SUCCEEDED(result)) {
        result = ggpo_synchronize_input(ggpo,       // the session object
            p,                                      // array of inputs
            sizeof(p),                              // size of all inputs
            &disconnectFlag);
        if (disconnectFlag != 0) {
            if (debugLog) UE_LOG(LogTemp, Warning, TEXT("Player Disconnected %i"), disconnectFlag);
        }
        if (GGPO_SUCCEEDED(result)) {
            /* pass both inputs to our advance function */
            //AdvanceGameState(&p[0], &p[1], &gamestate);
            GameAdvanceFrame(p, disconnectFlag);
            if (debugLog) UE_LOG(LogTemp, Warning, TEXT("Game Tick Success"));
            ggpo_advance_frame(ggpo);
        }
    }
    else {
        if (debugLog) UE_LOG(LogTemp, Warning, TEXT("ggpo_add_local_input error: %i"), result);
    }
   
}


void UGGPOInterface::StartSession()
{
    // Required setup for GGPO Interface
    self = this;
    rollbackManager = Cast<ARollbackManager>(UGameplayStatics::GetActorOfClass(this, ARollbackManager::StaticClass()));
    if (sessionStarted) {
        if (debugLog) UE_LOG(LogTemp, Warning, TEXT("Session Already Started. Please end session first"));
        return;
    }

    if (debugLog) {
        if (debugLog) UE_LOG(LogTemp, Warning, TEXT("PlayerNumber: %i"), playerNumber);
    }
    
    ///* fill in all callback functions */
    cb.begin_game = beginGame;
    cb.advance_frame = advanceFrame;
    cb.load_game_state = loadGameState;
    cb.save_game_state = saveGameState;
    cb.free_buffer = freeBuffer;
    cb.on_event = onEvent;

    unsigned short p1Port = 7000;
    unsigned short p2Port = 7001;
    char* localIp = "127.0.0.1";

    if (playerNumber == 0) {
        // p1 is local
        /* Start a new session */
        if (syncTest) {
            UE_LOG(LogTemp, Warning, TEXT("Starting Sync Test"));
            result = ggpo_start_synctest(&ggpo, 
                &cb, 
                "test_app", 
                2, 
                sizeof(int16),
                1);
        }
        else {
            result = ggpo_start_session(&ggpo,         // the new session object
                &cb,           // our callbacks
                "test_app",    // application name
                2,             // 2 players
                sizeof(int16),   // size of an input packet
                p1Port);         // our local udp port
        }


        p1.size = p2.size = sizeof(GGPOPlayer);
        p1.player_num = 1;
        p2.player_num = 2;

        p1.type = GGPO_PLAYERTYPE_LOCAL;                // local player
        p2.type = GGPO_PLAYERTYPE_REMOTE;               // remote player
        
        strcpy(p2.u.remote.ip_address, "127.0.0.1\0");  // ip addess of the player
        p2.u.remote.port = p2Port;
        if (debugLog) {
            if (debugLog) UE_LOG(LogTemp, Warning, TEXT("Session Result: %i"), result);
            if (debugLog) UE_LOG(LogTemp, Warning, TEXT("Session Result: %p"), ggpo);
        }
        


        result = ggpo_add_player(ggpo, &p1, &player_handles[0]);
        if (debugLog) {
            if (debugLog) UE_LOG(LogTemp, Warning, TEXT("add first player: %i"), result);
            if (debugLog) UE_LOG(LogTemp, Warning, TEXT("p1: %i %i %i"), p1.size, p1.player_num, p1.type);
        }

        result = ggpo_add_player(ggpo, &p2, &player_handles[1]);

        if (debugLog) {
            UE_LOG(LogTemp, Warning, TEXT("add second player %i"), result);
            UE_LOG(LogTemp, Warning, TEXT("p2: %i %i %i %s %i"), p2.size, p2.player_num, p2.type, *FString(strlen(p2.u.remote.ip_address), p2.u.remote.ip_address), p2.u.remote.port);
            UE_LOG(LogTemp, Warning, TEXT("p2: %s"), "127.0.0.1");
        }

        
        result = ggpo_set_frame_delay(ggpo, player_handles[0], 0);
        if (debugLog) {
            UE_LOG(LogTemp, Warning, TEXT("frame delay set %i"), result);
        }

        result = ggpo_set_frame_delay(ggpo, player_handles[1], 0);
        if (debugLog) {
            UE_LOG(LogTemp, Warning, TEXT("frame delay set %i"), result);
        }
    }
    else
    {
        // p2 is local
        /* Start a new session */
        result = ggpo_start_session(&ggpo,         // the new session object
            &cb,           // our callbacks
            "test_app",    // application name
            2,             // 2 players
            sizeof(int16),   // size of an input packet
            p2Port);         // our local udp port
        if (debugLog) UE_LOG(LogTemp, Warning, TEXT("Session Result: %i"), result);
        if (debugLog) UE_LOG(LogTemp, Warning, TEXT("Session Result: %p"), ggpo);

        p1.size = p2.size = sizeof(GGPOPlayer);
        p1.player_num = 1;
        p2.player_num = 2;


        p1.type = GGPO_PLAYERTYPE_REMOTE;               // remote player
        strcpy(p1.u.remote.ip_address, "127.0.0.1");  // ip addess of the player
        p1.u.remote.port = p1Port;

        p2.type = GGPO_PLAYERTYPE_LOCAL;                // local player

        result = ggpo_add_player(ggpo, &p1, &player_handles[0]);
        if (debugLog) {
            UE_LOG(LogTemp, Warning, TEXT("add first player: %i"), result);
            UE_LOG(LogTemp, Warning, TEXT("p1: %i %i %i %s %i"), p1.size, p1.player_num, p1.type, p1.u.remote.ip_address, p1.u.remote.port);
        }
        result = ggpo_add_player(ggpo, &p2, &player_handles[1]);
        if (debugLog) {
            UE_LOG(LogTemp, Warning, TEXT("add second player %i"), result);
            UE_LOG(LogTemp, Warning, TEXT("p2: %i %i %i"), p2.size, p2.player_num, p2.type);
        }

        result = ggpo_set_frame_delay(ggpo, player_handles[0], 0);
        if (debugLog) {
            UE_LOG(LogTemp, Warning, TEXT("frame delay set %i"), result);
        }

        result = ggpo_set_frame_delay(ggpo, player_handles[1], 0);
        if (debugLog) {
            UE_LOG(LogTemp, Warning, TEXT("frame delay set %i"), result);
        }
    }

    sessionStarted = true;

}

void UGGPOInterface::EndSession()
{
    if (sessionStarted) {
        sessionStarted = false;
        ggpo_close_session(ggpo);
    }
}

void UGGPOInterface::GGPOIdle()
{
    if (sessionStarted) {
        ggpo_idle(ggpo, 16);
    }
}

void UGGPOInterface::SetPlayerNumber(int num)
{
    playerNumber = num;
}

void UGGPOInterface::GameAdvanceFrame(int16 inputs[], int disconnect_flags)
{
    FGGPOInputFrame p1Input;
    FGGPOInputFrame p2Input;
    p1Input = convertInt16ToInputFrame(inputs[0]);
    p2Input = convertInt16ToInputFrame(inputs[1]);

    if (debugLog) UE_LOG(LogTemp, Warning, TEXT("Advancing Frame"));
    if (AdvanceFrameDelegate.IsBound()) {
        if (debugLog) UE_LOG(LogTemp, Warning, TEXT("Delegate Bound"));
    }
    else {
        if (debugLog) UE_LOG(LogTemp, Warning, TEXT("Delegate not bound"));
    }
    AdvanceFrameDelegate.ExecuteIfBound(p1Input, p2Input);
}

int getNthBit(int16 input, int n) {
    return (input & (1 << n)) >> n;
}

int16 setNthBit(int16 input, int n) {
    input |= (1UL << n);
    return input;
}

FGGPOInputFrame UGGPOInterface::convertInt16ToInputFrame(int16 input)
{
    FGGPOInputFrame inputFrame;
    int left = getNthBit(input, 0);
    int right = getNthBit(input, 1);
    int up = getNthBit(input, 2);
    int down = getNthBit(input, 3);
    int a1 = getNthBit(input, 4);
    int a2 = getNthBit(input, 5);
    int a3 = getNthBit(input, 6);

    inputFrame.leftRight = right - left;
    inputFrame.upDown = up - down;
    inputFrame.a1 = (bool)a1;
    inputFrame.a2 = (bool)a2;
    inputFrame.a3 = (bool)a3;
    return inputFrame;
}

int16 UGGPOInterface::convertInputFrameToInt16(FGGPOInputFrame input)
{
    int16 intInput = 0;
    if (input.leftRight == -1) {
        intInput = setNthBit(intInput, 0);
    }
    if (input.leftRight == 1) {
        intInput = setNthBit(intInput, 1);
    }

    if (input.upDown == 1) {
        intInput = setNthBit(intInput, 2);
    }
    if (input.upDown == -1) {
        intInput = setNthBit(intInput, 3);
    }
    if (input.a1) {
        intInput = setNthBit(intInput, 4);
    }
    if (input.a2) {
        intInput = setNthBit(intInput, 5);
    }
    if (input.a3) {
        intInput = setNthBit(intInput, 6);
    }

    return intInput;
}