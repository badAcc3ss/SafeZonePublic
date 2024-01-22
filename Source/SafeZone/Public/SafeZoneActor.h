#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "QuadrantSystemActor.h"
#include "SafeZoneActor.generated.h"

UCLASS()
class SAFEZONE_API ASafeZoneActor : public AActor
{
    GENERATED_BODY()

public:    
    ASafeZoneActor();

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    UPROPERTY(ReplicatedUsing = OnRep_UpdateSafeZone)
    FVector TargetLocation;

    UPROPERTY(ReplicatedUsing = OnRep_UpdateSafeZone)
    float TargetRadius;

    UPROPERTY(Replicated)
    bool bShouldShrink;

    UFUNCTION()
    void OnRep_UpdateSafeZone();

    virtual void GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Safe Zone")
    USphereComponent* SafeZoneSphere;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Safe Zone | Visualization")
    UStaticMeshComponent* SafeZoneVisual;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Safe Zone")
    float ShrinkSpeed;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Safe Zone")
    int32 MaxIterations;

    int32 GetCurrentIteration()
    {
        return CurrentIteration;
    }

private:
    TArray<AQuadrantSystemActor*> Quadrants;

    int32 CurrentIteration;

    int8 MinSafeZoneRadius;

    void CreateQuadrants();

    void UpdateQuadrants(float NewRadius, FVector NewCenter);

    void ShrinkSafeZone();

    void MoveSafeZone(FVector NewLocation);

    void UpdateSafeZoneProperties();

    AQuadrantSystemActor* FindQuadrantWithMinimumPlayers();

    // Timer handle for delaying the shrinking process
    FTimerHandle ShrinkDelayTimerHandle;

    // Function to start the shrinking process after a delay
    void StartShrinkingWithDelay(float DelayInSeconds);

public:
    TArray<AQuadrantSystemActor*> GetQuadrantsInSafeZone()
    {
        return Quadrants;
    }
};
