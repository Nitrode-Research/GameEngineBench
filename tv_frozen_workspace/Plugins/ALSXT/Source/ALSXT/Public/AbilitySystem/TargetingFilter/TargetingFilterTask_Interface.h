// MIT

#pragma once

#include "Tasks/TargetingFilterTask_BasicFilterTemplate.h"
#include "Types/TargetingSystemTypes.h"

#include "TargetingFilterTask_Interface.generated.h"

/**
 * 
 */
UCLASS()
class ALSXT_API UTargetingFilterTask_Interface : public UTargetingFilterTask_BasicFilterTemplate
{
	GENERATED_BODY()
	
	public:
    	UTargetingFilterTask_Interface(const FObjectInitializer& ObjectInitializer);
    
    	/** Evaluation function called by derived classes to process the targeting request */
    	virtual bool ShouldFilterTarget(const FTargetingRequestHandle& TargetingHandle, const FTargetingDefaultResultData& TargetData) const override;
    
    protected:
    	/** The set of required actor classes (must be one of these types to not be filtered out) */
    	UPROPERTY(EditAnywhere, Category = "Targeting Filter | Class", Meta = (AllowAbstract=true))
    	TArray<TScriptInterface<UInterface>> RequiredInterfaceClassFilters;
    
    	/** The set of ignored actor classes (must NOT be one of these types) */
    	UPROPERTY(EditAnywhere, Category = "Targeting Filter | Class", Meta = (AllowAbstract = true))
    	TArray<TScriptInterface<UInterface>> IgnoredInterfaceClassFilters;
	
};
