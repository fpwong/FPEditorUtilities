#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "FPObjectTableFactory.generated.h"

UCLASS()
class UFPObjectTableFactory : public UFactory
{
	GENERATED_BODY()

public:
	UFPObjectTableFactory();
	UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn);
};
