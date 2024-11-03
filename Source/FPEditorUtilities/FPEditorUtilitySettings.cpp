// Fill out your copyright notice in the Description page of Project Settings.


#include "FPEditorUtilitySettings.h"

void UFPEditorUtilitySettings::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.Property->GetName() == GET_MEMBER_NAME_CHECKED(ThisClass, TablesUsingGameplayTags))
	{
		OnTablesChanged.Broadcast();
	}
}
