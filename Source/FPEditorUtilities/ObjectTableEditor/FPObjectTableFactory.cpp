#include "FPObjectTableFactory.h"
#include "FPObjectTable.h"

UFPObjectTableFactory::UFPObjectTableFactory()
{
	SupportedClass = UFPObjectTable::StaticClass();
	bCreateNew = true;
}

UObject* UFPObjectTableFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UFPObjectTable>(InParent, Class, Name, Flags, Context);
}
