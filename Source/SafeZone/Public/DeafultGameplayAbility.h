// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GamePlayerCharacter.h"
#include "DeafultGameplayAbility.generated.h"

/**
 * 
 */
UCLASS()
class SAFEZONE_API UDeafultGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

	public:
	UDeafultGameplayAbility();

	// Tells an ability to activate immediately when its granted. Used for passive abilities and abilites forced on others.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Ability")
	bool bActivateAbilityOnGranted;
	
private:
	bool bActivateOnInput;
};
