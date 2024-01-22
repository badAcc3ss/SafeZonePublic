#include "SafeZoneActor.h"
#include "QuadrantSystemActor.h"
#include "Net/UnrealNetwork.h"


ASafeZoneActor::ASafeZoneActor()
{
    PrimaryActorTick.bCanEverTick = true;

    SafeZoneSphere = CreateDefaultSubobject<USphereComponent>(TEXT("SafeZoneSphere"));
    SafeZoneSphere->InitSphereRadius(2500.0); //can be set using a var
    RootComponent = SafeZoneSphere;

    SafeZoneVisual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SafeZoneVisual"));
    SafeZoneVisual->SetupAttachment(RootComponent);

    // Load the sphere mesh
    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("StaticMesh'/Game/Mesh/SafeZoneSphere.SafeZoneSphere'"));
    if (SphereMesh.Succeeded())
    {
        SafeZoneVisual->SetStaticMesh(SphereMesh.Object);
    }

    SafeZoneVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    // Assuming the mesh radius is approximately 50 units
    float MeshRadius = 50.0f;
    float InitialScale = SafeZoneSphere->GetScaledSphereRadius() / MeshRadius;
    SafeZoneVisual->SetWorldScale3D(FVector(InitialScale, InitialScale, InitialScale));

    // Load the material (if needed)
    /* ... */

    ShrinkSpeed = 0.5f;
    MaxIterations = 5;
    bShouldShrink = false;
    CurrentIteration = 0;
    MinSafeZoneRadius = 1;

    bReplicates = true;
}

void ASafeZoneActor::BeginPlay()
{
    Super::BeginPlay();

    if (HasAuthority())
    {
        TargetLocation = GetActorLocation();
        TargetRadius = SafeZoneSphere->GetScaledSphereRadius();
        CreateQuadrants();
        StartShrinkingWithDelay(30.0f);
    }
}

void ASafeZoneActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (bShouldShrink)
    {
        UpdateSafeZoneProperties();
    }
}

void ASafeZoneActor::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ASafeZoneActor, TargetLocation);
    DOREPLIFETIME(ASafeZoneActor, TargetRadius);
    DOREPLIFETIME(ASafeZoneActor, bShouldShrink);
}

void ASafeZoneActor::OnRep_UpdateSafeZone()
{
}

void ASafeZoneActor::CreateQuadrants()
{
    // Clear existing quadrants
    for (AQuadrantSystemActor* Quadrant : Quadrants)
    {
        if (Quadrant)
        {
            Quadrant->Destroy();
        }
    }
    Quadrants.Empty();

    FVector SphereCenter = GetActorLocation();
    float SphereRadius = SafeZoneSphere->GetScaledSphereRadius();
    float QuadrantRadius = SphereRadius / 2.0f;
    float AngleStep = 90.0f; // 90 degrees separation for 4 quadrants

    // Create quadrant actors
    for (int i = 0; i < 4; ++i)
    {
        float Angle = FMath::DegreesToRadians(AngleStep * i);
        FVector Offset = FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.0f) * QuadrantRadius;
        FVector QuadrantLocation = SphereCenter + Offset;
        AQuadrantSystemActor* NewQuadrant = GetWorld()->SpawnActor<AQuadrantSystemActor>(QuadrantLocation, FRotator::ZeroRotator);
        if (NewQuadrant)
        {
            NewQuadrant->QuadrantSphere->SetSphereRadius(QuadrantRadius);
            Quadrants.Add(NewQuadrant);
        }
    }
}

void ASafeZoneActor::UpdateQuadrants(float NewRadius, FVector NewCenter)
{
    float QuadrantRadius = NewRadius / 2.0f;
    float AngleStep = 90.0f;

    int i = 0;
    for (AQuadrantSystemActor* Quadrant : Quadrants)
    {
        if (Quadrant)
        {
            float Angle = FMath::DegreesToRadians(AngleStep * i++);
            FVector Offset = FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.0f) * QuadrantRadius;
            FVector NewLocation = NewCenter + Offset;
            Quadrant->SetActorLocation(NewLocation);
            Quadrant->QuadrantSphere->SetSphereRadius(QuadrantRadius);
        }
    }
}

void ASafeZoneActor::StartShrinkingWithDelay(float DelayInSeconds)
{
    // Clear any existing timers
    GetWorldTimerManager().ClearTimer(ShrinkDelayTimerHandle);

    // Set a timer to start the shrinking process after the specified delay
    GetWorldTimerManager().SetTimer(ShrinkDelayTimerHandle, this, &ASafeZoneActor::ShrinkSafeZone, DelayInSeconds, false);
}

void ASafeZoneActor::UpdateSafeZoneProperties()
{
    FVector CurrentLocation = GetActorLocation();
    float CurrentRadius = SafeZoneSphere->GetScaledSphereRadius();

    FVector TargetLocationXY(TargetLocation.X, TargetLocation.Y, CurrentLocation.Z);
    FVector NewLocationXY = FMath::VInterpTo(CurrentLocation, TargetLocationXY, GetWorld()->GetDeltaSeconds(), ShrinkSpeed);

    float NewRadius = FMath::FInterpTo(CurrentRadius, TargetRadius, GetWorld()->GetDeltaSeconds(), ShrinkSpeed);

    FVector MoveDirection = (TargetLocationXY - CurrentLocation).GetSafeNormal();
    float MoveDistance = FVector::Dist(CurrentLocation, TargetLocationXY) * FMath::Min(GetWorld()->GetDeltaSeconds() * ShrinkSpeed, 1.0f);
    FVector NewLocation = CurrentLocation + MoveDirection * MoveDistance;

    MoveSafeZone(NewLocation);
    SafeZoneSphere->SetSphereRadius(NewRadius);

    float SphereScale = NewRadius / 50.0f; // Adjust if your sphere mesh has a different default radius
    SafeZoneVisual->SetWorldScale3D(FVector(SphereScale, SphereScale, SphereScale));

    UpdateQuadrants(NewRadius, NewLocation); // Update quadrants as well

    if (NewLocation.Equals(TargetLocationXY, 2.0f) && FMath::IsNearlyEqual(NewRadius, TargetRadius, 1.0f))
    {
        if (HasAuthority())
        {
            bShouldShrink = false;
        }
        
        CurrentIteration++;

        if (CurrentIteration >= MaxIterations)
        {
            // Handle end of shrinking process
        }
        else
        {
            // Prepare for the next shrink phase
        }
    }
}

// Add a function to find the quadrant with the minimum number of players
AQuadrantSystemActor* ASafeZoneActor::FindQuadrantWithMinimumPlayers()
{
    AQuadrantSystemActor* MinPlayersQuadrant = nullptr;
    int32 MinPlayers = MAX_int32;

    for (AQuadrantSystemActor* Quadrant : Quadrants)
    {
        if (Quadrant)
        {
            int32 NumPlayersInQuadrant = Quadrant->GetNumberOfPlayersInQuadrant(); // Implement GetNumPlayers() in AQuadrantSystemActor

            if (NumPlayersInQuadrant < MinPlayers)
            {
                MinPlayers = NumPlayersInQuadrant;
                MinPlayersQuadrant = Quadrant;
            }
        }
    }

    return MinPlayersQuadrant;
}

// Modify the ShrinkSafeZone function to incorporate the new logic
void ASafeZoneActor::ShrinkSafeZone()
{
	if (!HasAuthority())
    {
        return;
    }

    if (CurrentIteration == MaxIterations - 1)
    {
        TargetRadius = 0.0f;
    }
    else
    {
        AQuadrantSystemActor* TargetQuadrant = FindQuadrantWithMinimumPlayers();

        if (TargetQuadrant)
        {
            TargetLocation = TargetQuadrant->GetRandomLocationInQuadrant();
            TargetRadius = FMath::Max(TargetQuadrant->QuadrantSphere->GetScaledSphereRadius(), (float)MinSafeZoneRadius);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("No valid target quadrant found."));
        }
    }

    bShouldShrink = true;
}

// Modify the MoveSafeZone function to remove existing quadrants and generate new ones
void ASafeZoneActor::MoveSafeZone(FVector NewLocation)
{
      SetActorLocation(NewLocation);

      if (HasAuthority())
      {
          StartShrinkingWithDelay(30.0f);
      }    
}
