#include "QuadrantSystemActor.h"
#include "GamePlayerCharacter.h"
#include "SafeZoneGameMode.h"

AQuadrantSystemActor::AQuadrantSystemActor()
{
    PrimaryActorTick.bCanEverTick = false;

    QuadrantSphere = CreateDefaultSubobject<USphereComponent>(TEXT("QuadrantSphere"));
    QuadrantSphere->SetSphereRadius(10);
    QuadrantSphere->bHiddenInGame = false;
    RootComponent = QuadrantSphere;

    QuadrantSphere->OnComponentBeginOverlap.AddDynamic(this, &AQuadrantSystemActor::OnPlayerEnterQuadrant);
    QuadrantSphere->OnComponentEndOverlap.AddDynamic(this, &AQuadrantSystemActor::OnPlayerExitQuadrant);

    bReplicates = true;
}

void AQuadrantSystemActor::BeginPlay()
{
    Super::BeginPlay();
}

void AQuadrantSystemActor::OnPlayerEnterQuadrant(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (!HasAuthority())
    {
        return;
    }
    AGamePlayerCharacter* PlayerCharacter = Cast<AGamePlayerCharacter>(OtherActor);
    if (PlayerCharacter)
    {
        FString PlayerID = PlayerCharacter->GetPlayerUniqueNetIdAsString();
        ASafeZoneGameMode* GameMode = Cast<ASafeZoneGameMode>(GetWorld()->GetAuthGameMode());
        if (GameMode)
        {
            GameMode->UpdatePlayerQuadrant(PlayerID, this);
        }

        PlayersInQuadrant.Add(PlayerCharacter);
    }
}

FVector AQuadrantSystemActor::GetRandomLocationInQuadrant() const
{
    FVector SphereCenter = QuadrantSphere->GetComponentLocation();
    float SphereRadius = QuadrantSphere->GetScaledSphereRadius();

    FVector RandomPointInSphere = FMath::RandPointInBox(FBox(SphereCenter - FVector(SphereRadius), SphereCenter + FVector(SphereRadius)));

    return RandomPointInSphere;
}

void AQuadrantSystemActor::OnPlayerExitQuadrant(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    if (!HasAuthority())
    {
        return;
    }

    AGamePlayerCharacter* PlayerCharacter = Cast<AGamePlayerCharacter>(OtherActor);
    if (PlayerCharacter)
    {
        FString PlayerID = PlayerCharacter->GetPlayerUniqueNetIdAsString();
        ASafeZoneGameMode* GameMode = Cast<ASafeZoneGameMode>(GetWorld()->GetAuthGameMode());
        if (GameMode)
        {
            // Start the delay before confirming the player's quadrant status
            const float QuadrantTransitionDelay = 1.5f; // Adjust the delay time as needed
            GameMode->StartQuadrantUpdateDelay(PlayerID, QuadrantTransitionDelay);
        }

        PlayersInQuadrant.Remove(PlayerCharacter);
    }
}

TArray<AGamePlayerCharacter*> AQuadrantSystemActor::GetPlayersInQuadrant() const
{
    return PlayersInQuadrant;
}

int32 AQuadrantSystemActor::GetNumberOfPlayersInQuadrant() const
{
    return PlayersInQuadrant.Num();
}

bool AQuadrantSystemActor::IsPlayerInside(const FString& PlayerID) const
{
    for (AActor* Player : PlayersInQuadrant)
    {
        AGamePlayerCharacter* PlayerCharacter = Cast<AGamePlayerCharacter>(Player);
        if (PlayerCharacter && PlayerCharacter->GetPlayerUniqueNetIdAsString() == PlayerID)
        {
            return true; // PlayerID matches a player in the quadrant
        }
    }
    return false; // PlayerID not found in this quadrant
}