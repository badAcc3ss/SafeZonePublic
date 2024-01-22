// Fill out your copyright notice in the Description page of Project Settings.


#include "GamePlayerCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"	
#include "GameFramework/SpringArmComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/PlayerState.h"
#include "SafeZoneGameMode.h"
#include "GamePlayerController.h"
//Abiilty System Component
#include "PlayerAttributeSet.h"
#include "AbilitySystemBlueprintLibrary.h"

AGamePlayerCharacter::AGamePlayerCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f); // ...at this rotation rate
	GetCharacterMovement()->JumpZVelocity = 600.f;
	GetCharacterMovement()->AirControl = 0.2f;

	// Initialize the AbilitySystemComponent, but only on the server
	if (GetLocalRole() == ROLE_Authority)
	{
		AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	}

	// Create the attribute set, this replicates by default
	// Adding it as a subobject of the owning actor of an AbilitySystemComponent
	// automatically registers the AttributeSet with the AbilitySystemComponent
	PlayerAttribute = CreateDefaultSubobject<UPlayerAttributeSet>(TEXT("PlayerAttributeSet"));

	bIsKnockedDown = false;
	KnockdownHealthThreshold = 20.0f;

	OutsideSafeZoneTag = FGameplayTag::RequestGameplayTag(TEXT("State.OutsideSafeZone"));

	SetReplicates(true);
	SetReplicateMovement(true);

}

void AGamePlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Setup delegate if this is the server
	if (GetLocalRole() == ROLE_Authority && AbilitySystemComponent)
	{
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(PlayerAttribute->GetHealthAttribute()).AddUObject(this, &AGamePlayerCharacter::HealthChanged);
		AbilitySystemComponent->RegisterGameplayTagEvent(OutsideSafeZoneTag, EGameplayTagEventType::NewOrRemoved).AddUObject(this, &AGamePlayerCharacter::OutsideSafeZoneTagChanged);
	}
}

void AGamePlayerCharacter::PossessedBy(AController* NewController)
{

	Super::PossessedBy(NewController);

	InitializeAttributes();

	AddStartupEffects();

	AddCharacterAbilities();

	SetHealth(GetCharacterMaxHealth());
}

void AGamePlayerCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AGamePlayerCharacter, bIsKnockedDown);
	DOREPLIFETIME(AGamePlayerCharacter, IsCharacterDead);	
}

void AGamePlayerCharacter::ApplyOutsideSafeZoneTag()
{
	if (IsValid(AbilitySystemComponent))
	{
		AbilitySystemComponent->AddLooseGameplayTag(OutsideSafeZoneTag);
	}
}

void AGamePlayerCharacter::RemoveOutsideSafeZoneTag()
{
	if (IsValid(AbilitySystemComponent))
	{
		AbilitySystemComponent->RemoveLooseGameplayTag(OutsideSafeZoneTag);
	}
}


UAbilitySystemComponent* AGamePlayerCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

bool AGamePlayerCharacter::IsCharacterAlive()
{
	return GetCharacterHealth() > 0.0f;
}

void AGamePlayerCharacter::RemoveCharacterAbilities()
{
	if (GetLocalRole() != ROLE_Authority || !IsValid(AbilitySystemComponent) || !bCharacterAbilitiesGiven)
	{
		return;
	}

	// Remove any abilities added from a previous call. This checks to make sure the ability is in the startup 'CharacterAbilities' array.
	TArray<FGameplayAbilitySpecHandle> AbilitiesToRemove;
	for (const FGameplayAbilitySpec& Spec : AbilitySystemComponent->GetActivatableAbilities())
	{
		if ((Spec.SourceObject == this) && CharacterAbilities.Contains(Spec.Ability->GetClass()))
		{
			AbilitiesToRemove.Add(Spec.Handle);
		}
	}

	// Do in two passes so the removal happens after we have the full list
	for (int32 i = 0; i < AbilitiesToRemove.Num(); i++)
	{
		AbilitySystemComponent->ClearAbility(AbilitiesToRemove[i]);
	}

	bCharacterAbilitiesGiven = false;
}

void AGamePlayerCharacter::Die()
{
	if (!HasAuthority())
	{
		return;
	}

	// Only runs on Server
	RemoveCharacterAbilities();
	MulticastPlayDeathAnimation();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetCharacterMovement()->GravityScale = 0;
	GetCharacterMovement()->Velocity = FVector(0);

	if (IsValid(AbilitySystemComponent))
	{
		// Clear the timer for applying damage
		if (GetWorld()->GetTimerManager().IsTimerActive(DamageTimerHandle))
		{
			GetWorld()->GetTimerManager().ClearTimer(DamageTimerHandle);
		}

		// Remove the active gameplay effect if it exists
		if (DamageEffectHandle.IsValid())
		{
			AbilitySystemComponent->RemoveActiveGameplayEffect(DamageEffectHandle);
			DamageEffectHandle.Invalidate(); // Invalidate the handle after removing the effect
		}

		AbilitySystemComponent->CancelAllAbilities();
		ForceNetUpdate();
	}

	// Set a timer to call FinishDying after 3 seconds
	FTimerHandle TimerHandle;
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &AGamePlayerCharacter::FinishDying, 3.0f, false);
}


void AGamePlayerCharacter::FinishDying()
{
	if (!HasAuthority())
	{
		return;
	}
	
	ASafeZoneGameMode* GameMode = Cast<ASafeZoneGameMode>(GetWorld()->GetAuthGameMode());
	if (GameMode)
	{
		GameMode->ManagePlayerCount();
	}


	AController* OwningController = GetController();
	if (OwningController)
	{
		OwningController->UnPossess();
	}


	Destroy();
}

float AGamePlayerCharacter::GetCharacterHealth() const
{
	if (IsValid(PlayerAttribute))
	{
		return PlayerAttribute->GetHealth();
	}

	return 0.0f;
}

float AGamePlayerCharacter::GetCharacterMaxHealth() const
{
	if (IsValid(PlayerAttribute))
	{
		return PlayerAttribute->GetMaxHealth();
	}

	return 0.0f;
}

void AGamePlayerCharacter::MulticastPlayKnockdownAnimation_Implementation()
{

}

void AGamePlayerCharacter::MulticastPlayDeathAnimation_Implementation()
{
	IsCharacterDead = true;
}	

void AGamePlayerCharacter::AddCharacterAbilities()
{
	// Grant abilities, but only on the server	
	if (GetLocalRole() != ROLE_Authority || !IsValid(AbilitySystemComponent) || bCharacterAbilitiesGiven)
	{
		return;
	}

	for (TSubclassOf<UGameplayAbility>& StartupAbility : CharacterAbilities)
	{
		FGameplayAbilitySpecHandle DefaultHandle = AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(StartupAbility, 1, 1, this));
		AbilitySystemComponent->TryActivateAbility(DefaultHandle);
	}

	bCharacterAbilitiesGiven = true;
}

void AGamePlayerCharacter::InitializeAttributes()
{
	if (!IsValid(AbilitySystemComponent))
	{
		return;
	}

	if (!DefaultAttributes)
	{
		UE_LOG(LogTemp, Error, TEXT("%s() Missing DefaultAttributes for %s. Please fill in the character's Blueprint."), *FString(__FUNCTION__), *GetName());
		return;
	}

	// Can run on Server and Client
	FGameplayEffectContextHandle EffectContext = AbilitySystemComponent->MakeEffectContext();
	EffectContext.AddSourceObject(this);

	FGameplayEffectSpecHandle NewHandle = AbilitySystemComponent->MakeOutgoingSpec(DefaultAttributes, 1, EffectContext);
	if (NewHandle.IsValid())
	{
		FActiveGameplayEffectHandle ActiveGEHandle = AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*NewHandle.Data.Get());
	}
}

void AGamePlayerCharacter::AddStartupEffects()
{
	if (GetLocalRole() != ROLE_Authority || !IsValid(AbilitySystemComponent) || bStartupEffectsApplied)
	{
		return;
	}

	FGameplayEffectContextHandle EffectContext = AbilitySystemComponent->MakeEffectContext();
	EffectContext.AddSourceObject(this);

	for (TSubclassOf<UGameplayEffect> GameplayEffect : StartupEffects)
	{
		FGameplayEffectSpecHandle NewHandle = AbilitySystemComponent->MakeOutgoingSpec(GameplayEffect, 1, EffectContext);
		if (NewHandle.IsValid())
		{
			FActiveGameplayEffectHandle ActiveGEHandle = AbilitySystemComponent->ApplyGameplayEffectSpecToTarget(*NewHandle.Data.Get(), AbilitySystemComponent);
		}
	}

	bStartupEffectsApplied = true;
}

void AGamePlayerCharacter::HealthChanged(const FOnAttributeChangeData& Data)
{
	if (GetLocalRole() == ROLE_Authority)
	{
		if (Data.NewValue <= 0.0f)
		{
			Die();
		}
		else if (Data.NewValue <= KnockdownHealthThreshold && !bIsKnockedDown)
		{
			Knockdown();
		}
	}
}

void AGamePlayerCharacter::OutsideSafeZoneTagChanged(const FGameplayTag CallbackTag, int32 NewCount)
{
	if (!HasAuthority())
	{
		return;
	}

	if (AbilitySystemComponent && IsCharacterAlive())
	{
		if (AbilitySystemComponent->HasMatchingGameplayTag(OutsideSafeZoneTag))
		{
			GetWorld()->GetTimerManager().SetTimer(
				DamageTimerHandle,
				[WeakThis = TWeakObjectPtr<AGamePlayerCharacter>(this)]()
			{
				if (WeakThis.IsValid() && WeakThis->AbilitySystemComponent)
				{
					FGameplayEffectSpecHandle SpecHandle = WeakThis->AbilitySystemComponent->MakeOutgoingSpec(WeakThis->DamageEffectClass, 1, WeakThis->AbilitySystemComponent->MakeEffectContext());
					if (SpecHandle.IsValid())
					{
						// Set the damage magnitude
						float DamageAmount = 5.0f; // Calculate your damage amount here
						UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(SpecHandle, FGameplayTag::RequestGameplayTag(FName("Data.Damage")), DamageAmount);

						WeakThis->DamageEffectHandle = WeakThis->AbilitySystemComponent->BP_ApplyGameplayEffectSpecToSelf(SpecHandle);
					}
				}
			},
				DamageInterval, // Your damage interval
				true // Repeating timer
				);
		}
		else if (GetWorld()->GetTimerManager().IsTimerActive(DamageTimerHandle))
		{
			// Tag does not exist, stop the effect and clear the timer
			GetWorld()->GetTimerManager().ClearTimer(DamageTimerHandle);

			// Stop the gameplay effect if it's active
			if (AbilitySystemComponent && DamageEffectHandle.IsValid())
			{
				AbilitySystemComponent->RemoveActiveGameplayEffect(DamageEffectHandle);
			}
		}
	}
}

void AGamePlayerCharacter::Knockdown()
{
	bIsKnockedDown = true;
	MulticastPlayKnockdownAnimation();
}

void AGamePlayerCharacter::SetHealth(float Health)
{
	if (IsValid(PlayerAttribute))
	{
		PlayerAttribute->SetHealth(Health);
	}
}

void AGamePlayerCharacter::OnRep_IsKnockedDown()
{
	//logic when knockeddown
	//like removing weapons, abilities etc
}

FString AGamePlayerCharacter::GetPlayerUniqueNetIdAsString()
{
	if (GetPlayerState())
	{
		return GetPlayerState()->GetUniqueId().ToString();
	}
	return FString();
}
