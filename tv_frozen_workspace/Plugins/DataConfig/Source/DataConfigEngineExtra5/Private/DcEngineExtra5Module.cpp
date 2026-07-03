#include "DcEngineExtra5Module.h"
#include "DataConfig/DcTypes.h"

void FDcEngineExtra5Module::StartupModule()
{
	UE_LOG(LogDataConfigCore, Log, TEXT("DataConfigEngineExtra5 module starting up"));
}

void FDcEngineExtra5Module::ShutdownModule()
{
	UE_LOG(LogDataConfigCore, Log, TEXT("DataConfigEngineExtra5 module shutting down"));
}

IMPLEMENT_MODULE(FDcEngineExtra5Module, DataConfigEngineExtra5);
