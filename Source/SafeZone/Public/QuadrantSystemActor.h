#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SphereComponent.h"
#include "QuadrantSystemActor.generated.h"


class AGamePlayerCharacter;

UCLASS()
class SAFEZONE_API AQuadrantSystemActor : public AActor
{
    GENERATED_BODY()

public:    
    AQuadrantSystemActor();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Quadrant")
    USphereComponent* QuadrantSphere;

    TArray<AGamePlayerCharacter*> GetPlayersInQuadrant() const;
    int32 GetNumberOfPlayersInQuadrant() const;

    FVector GetRandomLocationInQuadrant() const;

    bool IsPlayerInside(const FString& PlayerID) const;

protected:
    virtual void BeginPlay() override;

private:
    TArray<AGamePlayerCharacter*> PlayersInQuadrant;

    UFUNCTION()
    void OnPlayerEnterQuadrant(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

    UFUNCTION()
    void OnPlayerExitQuadrant(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
   
};
