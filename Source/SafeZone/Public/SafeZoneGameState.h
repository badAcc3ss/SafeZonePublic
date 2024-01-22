// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "SafeZoneGameState.generated.h"

/**
 * 
 */
UCLASS()
class SAFEZONE_API ASafeZoneGameState : public AGameState
{
	GENERATED_BODY()

	public:
    UPROPERTY(ReplicatedUsing = OnRep_PlayerCount)
    int32 PlayerCount;

    UFUNCTION()
    void OnRep_PlayerCount();

    UFUNCTION(BlueprintCallable, Category = "PlayerCount in Game")
    int32 GetPlayerCount() const { return PlayerCount; }

protected:
    virtual void GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const override;
};