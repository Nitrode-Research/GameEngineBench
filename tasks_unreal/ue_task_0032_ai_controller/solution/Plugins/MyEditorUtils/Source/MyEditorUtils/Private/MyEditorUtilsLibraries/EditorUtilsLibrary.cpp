// Copyright (c) Yevhenii Selivanov

#include "MyEditorUtilsLibraries/EditorUtilsLibrary.h"

// UE
#include "CookOnTheSide/CookOnTheFlyServer.h"
#include "Editor.h"
#include "Editor/EditorEngine.h"
#include "Editor/UnrealEdEngine.h"
#include "EditorFramework/AssetImportData.h"
#include "Engine/DataTable.h"
#include "LevelEditor.h"
#include "Misc/FileHelper.h"
#include "SLevelViewport.h"
#include "UnrealEdGlobals.h"

// Checks, is the current world placed in the editor
bool FEditorUtilsLibrary::IsEditor()
{
	return GIsEditor && GEditor && GWorld && GWorld->IsEditorWorld();
}

// Checks, that this actor placed in the editor world and the game is not started yet
bool FEditorUtilsLibrary::IsEditorNotPieWorld()
{
	return IsEditor() && !GEditor->IsPlaySessionInProgress();
}

// Returns true if game is started in the Editor
bool FEditorUtilsLibrary::IsPIE()
{
	return IsEditor() && GEditor->IsPlaySessionInProgress();
}

// Obtains the current world from the editor
UWorld* FEditorUtilsLibrary::GetEditorWorld()
{
	if (!IsEditor())
	{
		return nullptr;
	}

	UWorld* FoundWorld = nullptr;
	if (IsPIE())
	{
		FoundWorld = GEditor->GetCurrentPlayWorld();
		if (!FoundWorld)
		{
			const FWorldContext* PIEWorldContext = GEditor->GetPIEWorldContext();
			FoundWorld = PIEWorldContext ? PIEWorldContext->World() : nullptr;
		}
	}

	if (!FoundWorld)
	{
		// PIE is not started of the PIE's world is not found
		FoundWorld = GEditor->GetEditorWorldContext().World();
	}

	return !FoundWorld ? GWorld : FoundWorld;
}

// Returns true if currently is cooking the package
bool FEditorUtilsLibrary::IsCooking()
{
	if (IsEditorNotPieWorld())
	{
		const UCookOnTheFlyServer* CookServer = GUnrealEd ? GUnrealEd->CookServer : nullptr;
		return CookServer ? CookServer->IsCookByTheBookMode() : true;
	}
	return false;
}

// Returns current editor viewport
FViewport* FEditorUtilsLibrary::GetEditorViewport()
{
	const FLevelEditorModule& LevelEditorModule = FModuleManager::GetModuleChecked<FLevelEditorModule>(TEXT("LevelEditor"));
	const TSharedPtr<ILevelEditor> LevelEditor = LevelEditorModule.GetFirstLevelEditor();
	if (!LevelEditor)
	{
		return nullptr;
	}

	for (const TSharedPtr<SLevelViewport>& LevelViewportIt : LevelEditor->GetViewports())
	{
		const FEditorViewportClient* EditorViewport = LevelViewportIt ? LevelViewportIt->GetViewportClient().Get() : nullptr;
		FViewport* Viewport = EditorViewport ? EditorViewport->Viewport : nullptr;
		if (Viewport && Viewport->GetSizeXY() != FIntPoint::ZeroValue)
		{
			return Viewport;
		}
	}

	return nullptr;
}

// Exports specified data table to already its .json
void FEditorUtilsLibrary::ReExportTableAsJSON(const UDataTable* DataTable)
{
	const UAssetImportData* AssetImportData = DataTable ? DataTable->AssetImportData : nullptr;
	if (!AssetImportData)
	{
		return;
	}

	const FString CurrentFilename = AssetImportData->GetFirstFilename();
	if (!CurrentFilename.IsEmpty())
	{
		const FString TableAsJSON = DataTable->GetTableAsJSON(EDataTableExportFlags::UseJsonObjectsForStructs);
		FFileHelper::SaveStringToFile(TableAsJSON, *CurrentFilename);
	}
}

// Removes all custom assets from context menu
void FEditorUtilsLibrary::UnregisterAssets(TArray<TSharedPtr<FAssetTypeActions_Base>>& RegisteredAssets)
{
	static const FName AssetToolsModuleName = TEXT("AssetTools");
	const FAssetToolsModule* AssetToolsPtr = FModuleManager::GetModulePtr<FAssetToolsModule>(AssetToolsModuleName);
	if (!AssetToolsPtr)
	{
		return;
	}

	IAssetTools& AssetTools = AssetToolsPtr->Get();
	for (TSharedPtr<FAssetTypeActions_Base>& AssetTypeActionIt : RegisteredAssets)
	{
		if (AssetTypeActionIt.IsValid())
		{
			AssetTools.UnregisterAssetTypeActions(AssetTypeActionIt.ToSharedRef());
		}
	}
}
