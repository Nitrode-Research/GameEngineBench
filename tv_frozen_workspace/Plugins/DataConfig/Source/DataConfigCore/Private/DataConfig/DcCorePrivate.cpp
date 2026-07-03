#include "DcCorePrivate.h"
#include "Misc/EngineVersionComparison.h"

#if UE_VERSION_OLDER_THAN(5, 5, 0)
// 5.5 get rids of this boilerplate

#if !IS_MONOLITHIC

//	amend module boilerplate on modular builds
PER_MODULE_BOILERPLATE

#endif

#endif // !UE_VERSION_OLDER_THAN(5, 5, 0)

namespace DcCorePrivate {

void FStringNewlineDevice::Serialize(const TCHAR* InData, ELogVerbosity::Type Verbosity, const class FName& Category)
{
	FString::operator+=((TCHAR*)InData);
	*this += LINE_TERMINATOR;
}

FString& FStringNewlineDevice::operator+=(const FString& Other)
{
	return FString::operator+=(Other);
}


} // namespace DcCorePrivate


