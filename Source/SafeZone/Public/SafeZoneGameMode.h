// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "SafeZoneGameMode.generated.h"

/**
 * 
 */

class AQuadrantSystemActor;
class ASafeZoneActor;
class AGamePlayerCharacter;

UCLASS()
class SAFEZONE_API ASafeZoneGameMode : public AGameMode
{
	GENERATED_BODY()

	ASafeZoneGameMode();

	TMap<FString, AQuadrantSystemActor*> PlayerQuadrantMap;

	AQuadrantSystemActor* currentPlayerQuadrant;

protected:

	virtual void BeginPlay() override;

	virtual void PostLogin(APlayerController* NewPlayer) override;

	virtual void Logout(AController* Exiting) override;
public:
	void UpdatePlayerQuadrant(FString PlayerID, AQuadrantSystemActor* Quadrant);
	
	void StartQuadrantUpdateDelay(FString PlayerID, float DelayTime);

	void ManagePlayerCount();

	UPROPERTY(BlueprintReadWrite,EditAnywhere,Category = "Map SafeZone")
	ASafeZoneActor* safeZoneActor_Ref;

private:
	void DelayedUpdatePlayerQuadrant(FString PlayerID);

	void RemoveOutsideSafeZoneTagIfApplicable(const FString& PlayerID);

	void ApplyOutsideSafeZoneTagIfApplicable(const FString& PlayerID);

	bool IsPlayerInSafeZone(AGamePlayerCharacter* PlayerCharacter);

	AGamePlayerCharacter* GetPlayerCharacterFromID(const FString& PlayerID);

	bool IsPlayerInAnyQuadrant(const FString& PlayerID);

	AQuadrantSystemActor* FindCurrentQuadrantOfPlayer(const FString& PlayerID) const;

	ASafeZoneActor* SpawnSafeZoneActor();

	void EndGame();
};
