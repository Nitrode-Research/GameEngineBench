#include "DataConfig/Extra/Types/DcPropertyPathAccess.h"
#include "DataConfig/DcEnv.h"

namespace DcExtra
{

FDcResult TraverseReaderByPath(FDcPropertyReader* Reader, const FString& Path)
{
	return DcNoEntry();
}

FDcResult GetDatumPropertyByPath(const FDcPropertyDatum& Datum, const FString& Path, FDcPropertyDatum& OutDatum)
{
	return DcNoEntry();
}

} // namespace DcExtra
