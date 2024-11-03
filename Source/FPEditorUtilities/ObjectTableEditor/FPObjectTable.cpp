#include "FPObjectTable.h"

UFPObjectTable::UFPObjectTable()
	: ClassFilter(UObject::StaticClass())
	, RootDirectory(FPaths::ProjectDir())
	, bCheckSubfolders(true)
{
}

#if WITH_EDITOR
void UFPObjectTable::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	UObject::PostEditChangeProperty(PropertyChangedEvent);
	OnChange.Broadcast();
}
#endif
