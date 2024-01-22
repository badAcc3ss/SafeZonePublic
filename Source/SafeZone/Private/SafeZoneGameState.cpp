// Fill out your copyright notice in the Description page of Project Settings.


#include "SafeZoneGameState.h"
#include "Net/UnrealNetwork.h"

void ASafeZoneGameState::OnRep_PlayerCount()
{
    // Optionally do something when PlayerCount changes, like updating UI
}

void ASafeZoneGameState::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ASafeZoneGameState, PlayerCount);
}