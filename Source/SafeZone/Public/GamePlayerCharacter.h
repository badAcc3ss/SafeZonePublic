// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GamePlayerCharacter.generated.h"




UCLASS()
class SAFEZONE_API AGamePlayerCharacter : public ACharacter , public IAbilitySystemInterface
{
	GENERATED_BODY()
public:

	AGamePlayerCharacter();

	/** Base turn rate, in deg/sec. Other scaling may affect final turn rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	float BaseTurnRate;

	/** Base look up/down rate, in deg/sec. Other scaling may affect final rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	float BaseLookUpRate;

    // Ability System Interface
    UFUNCTION(BlueprintCallable, Category = "Abilities")
    virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UFUNCTION(BlueprintCallable, Category = "Character|State")
	bool IsCharacterAlive();

	// Removes all CharacterAbilities. Can only be called by the Server. Removing on the Server will remove from Client too.
	virtual void RemoveCharacterAbilities();

	virtual void Die();

	UFUNCTION(BlueprintCallable, Category = "Character|State")
	virtual void FinishDying();

	UFUNCTION(BlueprintCallable, Category = "Character|Attributes")
	float GetCharacterHealth() const;

	UFUNCTION(BlueprintCallable, Category = "Character|Attributes")
	float GetCharacterMaxHealth() const;

	// RPC to play knockdown montage on all clients
    UFUNCTION(NetMulticast, Reliable)
    void MulticastPlayKnockdownAnimation();

    // RPC to play death montage on all clients
    UFUNCTION(NetMulticast, Reliable)
    void MulticastPlayDeathAnimation();

	void ApplyOutsideSafeZoneTag();

	void RemoveOutsideSafeZoneTag();

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Anim State")
	bool IsCharacterDead;

protected:

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
    UAbilitySystemComponent* AbilitySystemComponent;

protected:

	// Reference to the AttributeSetBase. It will live on the PlayerState or here if the character doesn't have a PlayerState.
	UPROPERTY()
	class UPlayerAttributeSet* PlayerAttribute;

	//trigger a montage of death
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Character|Animation")
	UAnimMontage* DeathMontage;

	// Default abilities for this Character. These will be removed on Character death and regiven if Character respawns.
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Character|Abilities")
	TArray<TSubclassOf<class UGameplayAbility>> CharacterAbilities;

	// Default attributes for a character for initializing on spawn/respawn.
	// This is an instant GE that overrides the values for attributes that get reset on spawn/respawn.
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Character|Abilities")
	TSubclassOf<class UGameplayEffect> DefaultAttributes;

	// These effects are only applied one time on startup
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Character|Abilities")
	TArray<TSubclassOf<class UGameplayEffect>> StartupEffects;

	UPROPERTY(BlueprintReadOnly,EditAnywhere, Category = "Character | Effects")
	TSubclassOf<class UGameplayEffect> DamageEffectClass;

	// Grant abilities on the Server. The Ability Specs will be replicated to the owning client.
	virtual void AddCharacterAbilities();

	// Initialize the Character's attributes. Must run on Server but we run it on Client too
	// so that we don't have to wait. The Server's replication to the Client won't matter since
	// the values should be the same.
	virtual void InitializeAttributes();

	virtual void AddStartupEffects();

	virtual void BeginPlay() override;

	void HealthChanged(const FOnAttributeChangeData& Data);

	void OutsideSafeZoneTagChanged(const FGameplayTag CallbackTag, int32 NewCount);

	void Knockdown();

	virtual void PossessedBy(AController* NewController) override;

	// Replicate properties
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	/**
* Setters for Attributes. Only use these in special cases like Respawning, otherwise use a GE to change Attributes.
* These change the Attribute's Base Value.
*/

	virtual void SetHealth(float Health);

private:

	UPROPERTY(ReplicatedUsing=OnRep_IsKnockedDown)
	bool bIsKnockedDown;

	// Called when the bIsKnockedDown property is replicated
    UFUNCTION()
    void OnRep_IsKnockedDown();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Health", meta = (AllowPrivateAccess = "true"))
	float KnockdownHealthThreshold;

	bool bCharacterAbilitiesGiven;

	bool bStartupEffectsApplied;

	FGameplayTag OutsideSafeZoneTag;

	FTimerHandle DamageTimerHandle;

	FActiveGameplayEffectHandle DamageEffectHandle;

	float DamageInterval = 1; // will be set by the current iteration

//Networking
public:

	FString GetPlayerUniqueNetIdAsString();

};
