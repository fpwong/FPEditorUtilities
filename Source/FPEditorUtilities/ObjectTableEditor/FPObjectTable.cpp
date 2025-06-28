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

	if (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(UFPObjectTable, ClassFilter))
	{
		OnClassChanged.Broadcast();
	}

	OnChange.Broadcast();
}
#endif
