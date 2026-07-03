#include "DcEngineExtraModule.h"

#include "GameplayTagsManager.h"

#include "DataConfig/DcEnv.h"
#include "DataConfig/Extra/Diagnostic/DcDiagnosticExtra.h"
#include "DataConfig/EngineExtra/Diagnostic/DcDiagnosticEngineExtra.h"

static void _PopulateEngineExtraGameplayTagFixtures()
{
	UGameplayTagsManager::Get().AddNativeGameplayTag(TEXT("DataConfig.Foo.Bar"));
	UGameplayTagsManager::Get().AddNativeGameplayTag(TEXT("DataConfig.Foo.Bar.Baz"));
	UGameplayTagsManager::Get().AddNativeGameplayTag(TEXT("DataConfig.Tar.Taz"));
}

void FDcEngineExtraModule::StartupModule()
{
	UE_LOG(LogDataConfigCore, Log, TEXT("DcEngineExtraModule module starting up"));
	DcRegisterDiagnosticGroup(&DcDExtra::Details);
	DcRegisterDiagnosticGroup(&DcDEngineExtra::Details);

	_PopulateEngineExtraGameplayTagFixtures();

	DcStartUp(EDcInitializeAction::Minimal);
}

void FDcEngineExtraModule::ShutdownModule()
{
	UE_LOG(LogDataConfigCore, Log, TEXT("DcEngineExtraModule module shutting down"));

	DcShutDown();
}

IMPLEMENT_MODULE(FDcEngineExtraModule, DataConfigEngineExtra);
