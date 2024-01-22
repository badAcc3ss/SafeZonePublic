// Fill out your copyright notice in the Description page of Project Settings.


#include "DeafultGameplayAbility.h"

UDeafultGameplayAbility::UDeafultGameplayAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	bActivateAbilityOnGranted = false;
	bActivateOnInput = true;

	//can be used to trigger the effects based on tags
	ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag("State.Dead"));
	ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag("State.KnockedDown"));
}
