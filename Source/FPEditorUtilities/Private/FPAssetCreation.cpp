#include "FPAssetCreation.h"
#include "Factories/AnimMontageFactory.h"
#include "AssetToolsModule.h"
#include "DesktopPlatform/Public/IDesktopPlatform.h"
#include "DesktopPlatform/Public/DesktopPlatformModule.h"
#include "Framework/Application/SlateApplication.h"
#include "EditorDirectories.h"
#include "Misc/MessageDialog.h"
#include "Misc/PackageName.h"
#include "Factories/BlueprintFactory.h"
#include "Factories/AnimBlueprintFactory.h"
#include "Factories/BlendSpaceFactory1D.h"
#include "Animation/BlendSpaceBase.h"
#include "Animation/AnimInstance.h"
#include "GameFramework/Character.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Animation/AnimBlueprint.h"

UFPAssetCreation::UFPAssetCreation()
{
	DefaultPath.Path = TEXT("/Game");
	CharacterName = "DefaultName";
	CharacterClass = ACharacter::StaticClass();
	AnimInstanceClass = UAnimInstance::StaticClass();
}

void UFPAssetCreation::CreateCharacter()
{
	if (!CharacterSkeleton)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s No character skeleton selected"), TEXT(__FUNCTION__));
		return;
	}

	static FName AssetToolsModuleName = FName("AssetTools");
	FAssetToolsModule& AssetToolsModule = FModuleManager::GetModuleChecked<FAssetToolsModule>(AssetToolsModuleName);

	if (!SelectDestinationFolder())
	{
		return;
	}

	for (FString Suffix : MontagesToCreate)
	{
		CreateMontage(AssetToolsModule, Suffix);
	}

	CreateAnimBP(AssetToolsModule);
	CreateBlendspace(AssetToolsModule);

	CreateCharacter(AssetToolsModule);

	// UE_LOG(LogTemp, Warning, TEXT("%s |%s | %s"), *DestinationFolder, *PackagePath, *OutPath);
	// AssetToolsModule.Get().CreateUniqueAssetName(Path + TEXT("/") + NewFactory->GetDefaultNewAssetName(), TEXT(""), PackageNameToUse, DefaultAssetName);
	// CreateNewAsset(DefaultAssetName, SelectedPath, NewFactory->GetSupportedClass(), NewFactory);
}

void UFPAssetCreation::TryLoadObj()
{
	FString FullPath = FString::Printf(TEXT("%sBP_%s.BP_%s"), *DefaultPath.Path, *CharacterName, *CharacterName);

	UE_LOG(LogTemp, Warning, TEXT("Tryin to load object?"));

	auto Actor = LoadObject<AActor>(nullptr, TEXT("/Game/ArenaGame/Heroes/Test/BP_Test241.BP_Test241"));
	if (Actor)
	{
		UE_LOG(LogTemp, Warning, TEXT("Loaded properly?"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to load actor"));
	}
}

bool UFPAssetCreation::SelectDestinationFolder()
{
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (!DesktopPlatform)
		return false;

	const void* ParentWindowWindowHandle = FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr);

	const FString Title = FString("Choose a destination");
	bool bFolderAccepted = false;

	FString FilenameDefaultPath;
	FPackageName::TryConvertLongPackageNameToFilename(DefaultPath.Path, FilenameDefaultPath);

	FString SelectedFolder;
	while (!bFolderAccepted)
	{
		const bool bFolderSelected = DesktopPlatform->OpenDirectoryDialog(
			ParentWindowWindowHandle,
			Title,
			FilenameDefaultPath,
			SelectedFolder
		);

		if (!bFolderSelected)
		{
			// User canceled, return
			return false;
		}

		FPaths::NormalizeFilename(SelectedFolder);
		if (!SelectedFolder.EndsWith(TEXT("/")))
		{
			SelectedFolder += TEXT("/");
		}

		bFolderAccepted = true;
	}

	FPackageName::TryConvertFilenameToLongPackageName(SelectedFolder, DestinationFolder);
	return true;
}

void UFPAssetCreation::CreateMontage(FAssetToolsModule& AssetToolsModule, FString Suffix)
{
	UAnimMontageFactory* Factory = NewObject<UAnimMontageFactory>();
	Factory->TargetSkeleton = CharacterSkeleton->Skeleton;

	FString Name = FString::Printf(TEXT("Montage_%s_%s"), *CharacterName, *Suffix);
	UObject* CreatedAsset = AssetToolsModule.Get().CreateAsset(Name, DestinationFolder, Factory->SupportedClass, Factory);
	UAnimMontage* Montage = Cast<UAnimMontage>(CreatedAsset);
	if (Montage)
	{
		Montage->BlendIn = 0.1f;
		Montage->BlendOut = 0.1f;

		if (Suffix == "Death")
		{
			Montage->bEnableAutoBlendOut = false;
		}
	}

	if (CreatedAsset)
	{
		SaveAsset(CreatedAsset);
	}
}

void UFPAssetCreation::CreateCharacter(FAssetToolsModule& AssetToolsModule)
{
	UBlueprintFactory* Factory = NewObject<UBlueprintFactory>();
	Factory->ParentClass = CharacterClass;

	FString Name = FString::Printf(TEXT("BP_%s"), *CharacterName);

	UObject* CreatedAsset = AssetToolsModule.Get().CreateAsset(Name, DestinationFolder, Factory->SupportedClass, Factory);

	if (CreatedAsset)
	{
		SaveAsset(CreatedAsset);
		EditCharacterBlueprint(CreatedAsset);
	}
}

void UFPAssetCreation::CreateAnimBP(FAssetToolsModule& AssetToolsModule)
{
	UAnimBlueprintFactory* Factory = NewObject<UAnimBlueprintFactory>();
	Factory->TargetSkeleton = CharacterSkeleton->Skeleton;
	Factory->ParentClass = AnimInstanceClass;

	FString Name = FString::Printf(TEXT("ABP_%s"), *CharacterName);
	UObject* CreatedAsset = AssetToolsModule.Get().CreateAsset(Name, DestinationFolder, Factory->SupportedClass, Factory);
	if (CreatedAsset)
	{
		SaveAsset(CreatedAsset);
		CreatedAnimBlueprint = Cast<UAnimBlueprint>(CreatedAsset);
		UE_LOG(LogTemp, Warning, TEXT("%s"), *CreatedAsset->GetName());
	}
}

void UFPAssetCreation::CreateBlendspace(FAssetToolsModule& AssetToolsModule)
{
	UBlendSpaceFactory1D* Factory = NewObject<UBlendSpaceFactory1D>();
	Factory->TargetSkeleton = CharacterSkeleton->Skeleton;

	FString Name = FString::Printf(TEXT("BS_%s_Walk"), *CharacterName);
	UObject* CreatedAsset = AssetToolsModule.Get().CreateAsset(Name, DestinationFolder, Factory->SupportedClass, Factory);

	if (CreatedAsset)
	{
		SaveAsset(CreatedAsset);
	}

	// UObject* CreatedAsset = AssetToolsModule.Get().CreateAsset(CharacterName, DestinationFolder, Factory->SupportedClass, Factory);
	// if (UBlendSpaceBase* BlendSpace = Cast<UBlendSpaceBase>(CreatedAsset))
	// {
	// 	BlendSpace->GetBlendParameter(0);
	// }
}

void UFPAssetCreation::EditCharacterBlueprint(UObject* CreatedAsset)
{
	// UBlueprint* Blueprint = Cast<UBlueprint>(Object);
	UBlueprint* Blueprint = Cast<UBlueprint>(CreatedAsset);
	check(Blueprint);

	// FKismetEditorUtilities::CompileBlueprint(Blueprint);


	// if (auto BoolProperty = FindField<UBoolProperty>(Blueprint->GetClass(), "bRunConstructionScriptInSequencer"))
	// {
	// 	UE_LOG(LogTemp, Warning, TEXT("Set bAlwaysRelevant"))
	//
	// 	auto Oldv = BoolProperty->GetPropertyValue(Blueprint);
	// 	UE_LOG(LogTemp, Warning, TEXT("Old value %s"), Oldv ? *FString("True") : *FString("False"));
	//
	// 	BoolProperty->SetPropertyValue_InContainer(Blueprint, true);
	// 	auto Test = BoolProperty->GetPropertyValue(Blueprint);
	// 	UE_LOG(LogTemp, Warning, TEXT("New value %s"), Test ? *FString("True") : *FString("False"));
	// }
	//
	//
	// auto CDO = Blueprint->GeneratedClass->GetDefaultObject();
	//
	// // TESTING STUFF
	// if (auto IsCrouched = Cast<UBoolProperty>(Blueprint->GeneratedClass->FindPropertyByName("bAlwaysRelevant")))
	// {
	// 	UE_LOG(LogTemp, Warning, TEXT("Set bAlwaysRelevant"))
	//
	// 		auto Oldv = IsCrouched->GetPropertyValue(CDO);
	// 	UE_LOG(LogTemp, Warning, TEXT("Old value %s"), Oldv ? *FString("True") : *FString("False"));
	//
	//
	// 	IsCrouched->SetPropertyValue_InContainer(CDO, true);
	// 	auto Test = IsCrouched->GetPropertyValue(CDO);
	// 	UE_LOG(LogTemp, Warning, TEXT("New value %s"), Test ? *FString("True") : *FString("False"));
	// }
	//
	// for (TFieldIterator<UProperty> PropIt(Blueprint->GeneratedClass.Get()); PropIt; ++PropIt)
	// {
	// 	UProperty* Property = *PropIt;
	// 	UE_LOG(LogTemp, Warning, TEXT("\tGen class Property %s %s"), *Property->GetName(), *Property->GetClass()->GetName());
	// }
	//
	// // Get the mesh component
	// if (UProperty* MeshProperty = Blueprint->GeneratedClass->FindPropertyByName("Mesh"))
	// {
	// 	UE_LOG(LogTemp, Warning, TEXT("Found mesh property!"));
	//
	// 	if (UObjectProperty* ObjProperty = Cast<UObjectProperty>(MeshProperty))
	// 	{
	// 		// set the skeleton
	// 		UE_LOG(LogTemp, Warning, TEXT("Is object proprety"));
	// 		if (auto SkeleMeshProperty = Cast<UObjectProperty>(ObjProperty->PropertyClass->FindPropertyByName("SkeletalMesh")))
	// 		{
	// 			UE_LOG(LogTemp, Warning, TEXT("Set skele mesh"));
	// 			auto SkeleMeshDefaultObj = ObjProperty->PropertyClass->GetDefaultObject();
	// 			SkeleMeshProperty->SetObjectPropertyValue(SkeleMeshProperty->ContainerPtrToValuePtr<UObject*>(SkeleMeshDefaultObj), CharacterSkeleton->GetClass());
	// 		}
	//
	// 		for (TFieldIterator<UProperty> PropIt(ObjProperty->PropertyClass); PropIt; ++PropIt)
	// 		{
	// 			UProperty* Property = *PropIt;
	// 			UE_LOG(LogTemp, Warning, TEXT("\tMesh Property %s %s"), *Property->GetName(), *Property->GetClass()->GetName());
	// 		}
	// 	}
	// }

	if (auto CreatedActorr = Cast<AActor>(Blueprint->GeneratedClass->GetDefaultObject()))
	{
		if (ACharacter* Character = Cast<ACharacter>(CreatedActorr))
		{
			Character->GetMesh()->SetSkeletalMesh(CharacterSkeleton);

			if (CreatedAnimBlueprint)
			{
				Character->GetMesh()->SetAnimClass(CreatedAnimBlueprint->GeneratedClass.Get());
			}
			UE_LOG(LogTemp, Warning, TEXT("Set character skeleton"));
		}
	}
}

void UFPAssetCreation::SaveAsset(UObject* CreatedAsset)
{
	UPackage* const Package = CreatedAsset->GetOutermost();
	FString const PackageName = Package->GetName();
	FString const PackageFileName = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());
	UPackage::SavePackage(Package, nullptr, RF_Standalone, *PackageFileName, GError, nullptr, false, true, SAVE_NoError);
}
