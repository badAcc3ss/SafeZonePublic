// Fill out your copyright notice in the Description page of Project Settings.


#include "SafeZoneGameMode.h"
#include "GamePlayerCharacter.h"
#include "QuadrantSystemActor.h"
#include "SafeZoneActor.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerState.h"
#include "SafeZoneGameState.h"
#include "GamePlayerController.h"
#include "UObject/ConstructorHelpers.h"


ASafeZoneGameMode::ASafeZoneGameMode()
{
    currentPlayerQuadrant = nullptr;
}

void ASafeZoneGameMode::BeginPlay()
{
    Super::BeginPlay();

    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ASafeZoneActor::StaticClass(), FoundActors);

    if (FoundActors.Num() > 0)
    {
        safeZoneActor_Ref = Cast<ASafeZoneActor>(FoundActors[0]);
        // Optionally, handle cases where there are multiple actors
    }
}

void ASafeZoneGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);
    
    ASafeZoneGameState* GS = GetGameState<ASafeZoneGameState>();
    if (GS)
    {
        GS->PlayerCount++;
    }
}

void ASafeZoneGameMode::Logout(AController* Exiting)
{
    Super::Logout(Exiting);
    ASafeZoneGameState* GS = GetGameState<ASafeZoneGameState>();
    if (GS)
    {
        GS->PlayerCount--;

        if (GS->PlayerCount <= 0)
        {
            EndGame();
        }
    }
}

ASafeZoneActor* ASafeZoneGameMode::SpawnSafeZoneActor()
{
    // Ensure we have a valid world context
    UWorld* World = GetWorld();
    if (!World)
    {
        return nullptr;
    }

    // Define spawn parameters (optional, customize as needed)
    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = this;

    // Define the initial transform for the actor (customize as needed)
    FTransform SpawnTransform;
    SpawnTransform.SetLocation(FVector::ZeroVector); // Spawn at world origin, for example
    SpawnTransform.SetRotation(FQuat::Identity);
    SpawnTransform.SetScale3D(FVector(1.0f));

    // Spawn the actor
    ASafeZoneActor* SpawnedActor = World->SpawnActor<ASafeZoneActor>(ASafeZoneActor::StaticClass(), SpawnTransform, SpawnParams);

    return SpawnedActor;
}


void ASafeZoneGameMode::UpdatePlayerQuadrant(FString PlayerID, AQuadrantSystemActor* Quadrant)
{
    if (Quadrant)
    {
        PlayerQuadrantMap.Add(PlayerID, Quadrant);
        UE_LOG(LogTemp, Warning, TEXT("Adding player %s in quadrant %s"), *PlayerID, *Quadrant->GetName());

        RemoveOutsideSafeZoneTagIfApplicable(PlayerID);
    }
    else
    {
        PlayerQuadrantMap.Remove(PlayerID);
        UE_LOG(LogTemp, Warning, TEXT("Removing player %s from quadrant"), *PlayerID);

        ApplyOutsideSafeZoneTagIfApplicable(PlayerID);
    }
}

void ASafeZoneGameMode::DelayedUpdatePlayerQuadrant(FString PlayerID)
{
    if (IsPlayerInAnyQuadrant(PlayerID))
    {
        UpdatePlayerQuadrant(PlayerID, currentPlayerQuadrant);
        UE_LOG(LogTemp, Warning, TEXT("Player %s is in quadrant %s"), *PlayerID, *currentPlayerQuadrant->GetName());
    }
    else
    {
        PlayerQuadrantMap.Remove(PlayerID);
        UE_LOG(LogTemp, Warning, TEXT("Removing player %s from quadrant"), *PlayerID);

        ApplyOutsideSafeZoneTagIfApplicable(PlayerID);
    }
}

void ASafeZoneGameMode::RemoveOutsideSafeZoneTagIfApplicable(const FString& PlayerID)
{
    AGamePlayerCharacter* PlayerCharacter = GetPlayerCharacterFromID(PlayerID);
    if (PlayerCharacter && PlayerCharacter->GetAbilitySystemComponent()->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(FName("State.OutsideSafeZone"))))
    {
        PlayerCharacter->GetAbilitySystemComponent()->RemoveLooseGameplayTag(FGameplayTag::RequestGameplayTag(FName("State.OutsideSafeZone")));
        UE_LOG(LogTemp, Warning, TEXT("Removed OutsideSafeZone tag from player %s"), *PlayerID);
    }
}

void ASafeZoneGameMode::ApplyOutsideSafeZoneTagIfApplicable(const FString& PlayerID)
{
    AGamePlayerCharacter* PlayerCharacter = GetPlayerCharacterFromID(PlayerID);
    if (PlayerCharacter && !IsPlayerInSafeZone(PlayerCharacter)
        && !PlayerCharacter->GetAbilitySystemComponent()->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(FName("State.OutsideSafeZone"))))
    {
        PlayerCharacter->GetAbilitySystemComponent()->AddLooseGameplayTag(FGameplayTag::RequestGameplayTag(FName("State.OutsideSafeZone")));
        UE_LOG(LogTemp, Warning, TEXT("Player %s is outside safezone, tag set"), *PlayerID);
    }
}

bool ASafeZoneGameMode::IsPlayerInSafeZone(AGamePlayerCharacter* PlayerCharacter)
{
    if (PlayerCharacter && safeZoneActor_Ref)
    {
        return safeZoneActor_Ref->IsOverlappingActor(PlayerCharacter);
    }
    return false;
}

AGamePlayerCharacter* ASafeZoneGameMode::GetPlayerCharacterFromID(const FString& PlayerID)
{
    for (auto It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        APlayerController* PC = It->Get();
        if (PC && PC->PlayerState && PC->PlayerState->GetUniqueId().ToString() == PlayerID)
        {
            return Cast<AGamePlayerCharacter>(PC->GetPawn());
        }
    }
    return nullptr;
}

void ASafeZoneGameMode::StartQuadrantUpdateDelay(FString PlayerID, float DelayTime)
{
    // Start a timer to delay the update
    FTimerHandle TimerHandle;
    GetWorld()->GetTimerManager().SetTimer(TimerHandle, FTimerDelegate::CreateUObject(this, &ASafeZoneGameMode::DelayedUpdatePlayerQuadrant, PlayerID), DelayTime, false);
}

bool ASafeZoneGameMode::IsPlayerInAnyQuadrant(const FString& PlayerID)
{
    // Iterate through all quadrants to check if player is in any of them
    for (AQuadrantSystemActor* Quadrant : safeZoneActor_Ref->GetQuadrantsInSafeZone())
    {
        if (Quadrant && Quadrant->IsPlayerInside(PlayerID))
        {
            currentPlayerQuadrant = Quadrant;

            return true; // Player is inside one of the quadrants
        }
    }
    return false; // Player is not inside any quadrant
}

void ASafeZoneGameMode::ManagePlayerCount()
{
    ASafeZoneGameState* GS = GetGameState<ASafeZoneGameState>();
    if (GS)
    {
        GS->PlayerCount--;
    };

    if (GS->PlayerCount <= 0)
    {
        EndGame();
    }
}

void ASafeZoneGameMode::EndGame()
{
    // Assuming your widget class is UGameOverWidget
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        AGamePlayerController* PlayerController = Cast<AGamePlayerController>(It->Get());
        if (PlayerController)
        {
           PlayerController->EndGameReturnToMain();   
        }
    }
}