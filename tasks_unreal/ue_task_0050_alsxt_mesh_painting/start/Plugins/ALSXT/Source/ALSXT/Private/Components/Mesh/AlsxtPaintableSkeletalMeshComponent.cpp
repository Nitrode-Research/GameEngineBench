// MIT


#include "Components/Mesh/AlsxtPaintableSkeletalMeshComponent.h"
#include "Interfaces/AlsxtMeshPaintingInterface.h"
#include "Settings/AlsxtCharacterSettings.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Utility/AlsLog.h"

UAlsxtPaintableSkeletalMeshComponent::UAlsxtPaintableSkeletalMeshComponent()
{
	// Bind InitializeMaterials to When Materials Change
}

void UAlsxtPaintableSkeletalMeshComponent::BeginPlay()
{
	Super::BeginPlay();

	if (IsMeshPaintingConfigured())
	{
		SceneCaptureComponent = IAlsxtMeshPaintingInterface::Execute_GetSceneCaptureComponent(GetOwner());
		GlobalGeneralMeshPaintingSettings = IAlsxtMeshPaintingInterface::Execute_GetGlobalGeneralMeshPaintingSettings(GetOwner());
		if (IsMeshPaintingEnabled())
		{
			// InitializeMaterials();
			MaterialInstanceDynamic = CreateAndSetMaterialInstanceDynamic(0);

			if (RenderTargetAsset)
			{
				MaterialInstanceDynamic->SetTextureParameterValue("RT_Effects", RenderTargetAsset);
			}
		}
	}
	else
	{
		UE_LOG(LogAls, Error, TEXT("IsMeshPaintingConfigured is false"));
	}
}

void UAlsxtPaintableSkeletalMeshComponent::SetSkeletalMesh(USkeletalMesh* NewMesh, bool bReinitPose)
{
	Super::SetSkeletalMesh(NewMesh, bReinitPose);

	if (GetMaterial(0))
	{
		FAlsxtServerMeshPaintingSettings ServerGeneralMeshPaintingSettings{ IAlsxtMeshPaintingInterface::Execute_GetServerGeneralMeshPaintingSettings(GetOwner()) };
		FAlsxtGeneralMeshPaintingSettings UserGeneralMeshPaintingSettings{ IAlsxtMeshPaintingInterface::Execute_GetUserGeneralMeshPaintingSettings(GetOwner()) };
		if (GlobalGeneralMeshPaintingSettings.GeneralSettings.bEnableMeshPainting && ServerGeneralMeshPaintingSettings.GeneralSettings.bEnableMeshPainting && UserGeneralMeshPaintingSettings.bEnableMeshPainting)
		{
			// InitializeMaterials();
		}
	}
}

bool UAlsxtPaintableSkeletalMeshComponent::IsMeshPaintingConfigured() const
{
	return false;
}

bool UAlsxtPaintableSkeletalMeshComponent::IsMeshPaintingEnabled() const
{
	return false;
}

void UAlsxtPaintableSkeletalMeshComponent::InitializeMaterials()
{
}

void UAlsxtPaintableSkeletalMeshComponent::SetSceneCaptureRenderTarget(UTextureRenderTarget2D* NewRenderTarget)
{
	SceneCaptureComponent->TextureTarget = NewRenderTarget;
}

void UAlsxtPaintableSkeletalMeshComponent::SetPhysicalMaterialMask(UPhysicalMaterialMask* NewPhysicalMaterialMask)
{
	PhysicalMaterialMask = NewPhysicalMaterialMask;
}

UAlsxtMeshPaintingSettingsMap* UAlsxtPaintableSkeletalMeshComponent::GetMeshPaintingSettingsMap() const
{
	return MeshPaintingSettingsMap;
}

void UAlsxtPaintableSkeletalMeshComponent::SetMeshPaintingSettingsMap(UAlsxtMeshPaintingSettingsMap* NewMeshPaintingSettingsMap)
{
	OnChangeMeshPaintingSettingsMap.Broadcast(MeshPaintingSettingsMap, NewMeshPaintingSettingsMap);
	MeshPaintingSettingsMap = NewMeshPaintingSettingsMap;
}

UAlsxtMeshPaintingSettings*& UAlsxtPaintableSkeletalMeshComponent::GetMeshPaintingSettings(TEnumAsByte<EPhysicalSurface> SurfaceType)
{
	UAlsxtMeshPaintingSettingsMap* FoundMeshPaintingSettingsMap = MeshPaintingSettingsMap;
	TMap<TEnumAsByte<EPhysicalSurface>, UAlsxtMeshPaintingSettings*> FoundSettings{ MeshPaintingSettingsMap->SettingsMap };
	UAlsxtMeshPaintingSettings*& FoundMeshPaintingSettings { *FoundSettings.Find(SurfaceType) };
	return *&FoundMeshPaintingSettings;
}

void UAlsxtPaintableSkeletalMeshComponent::SetMeshPaintingSettings(UAlsxtMeshPaintingSettings* NewMeshPaintingSettings)
{
	OnChangeMeshPaintingSettings.Broadcast(MeshPaintingSettings, NewMeshPaintingSettings);
	MeshPaintingSettings = NewMeshPaintingSettings;
}

FALSXTMeshPaintCriteria UAlsxtPaintableSkeletalMeshComponent::GetMeshPaintCriteriaEntry(TEnumAsByte<EPhysicalSurface> SurfaceType)
{
	TArray<FALSXTMeshPaintCriteria> NewMeshPaintCriteriaArray{ GetMeshPaintingSettings(SurfaceType)->MeshPaintCriteria };
	FALSXTMeshPaintCriteria NewMeshPaintCriteria;
	return NewMeshPaintCriteria;
}

FALSXTMeshPaintCriteria UAlsxtPaintableSkeletalMeshComponent::GetItemMeshPaintCriteriaEntry(TEnumAsByte<EPhysicalSurface> SurfaceType)
{
	FALSXTMeshPaintCriteria NewMeshPaintCriteria;
	return NewMeshPaintCriteria;
}

void UAlsxtPaintableSkeletalMeshComponent::SetMeshPaintCriteria(TMap<TEnumAsByte<EPhysicalSurface>, FALSXTMeshPaintCriteria> NewMeshPaintCriteria)
{
	FALSXTMeshPaintCriteriaMap PreviousMap;
	PreviousMap.MeshPaintCriteriaMap = MeshPaintCriteria;
	FALSXTMeshPaintCriteriaMap NewMap;
	NewMap.MeshPaintCriteriaMap = NewMeshPaintCriteria;
	OnChangeItemMeshPaintCriteria.Broadcast(PreviousMap, NewMap);
	MeshPaintCriteria = NewMeshPaintCriteria;
}

void UAlsxtPaintableSkeletalMeshComponent::SetItemMeshPaintCriteria(TMap<TEnumAsByte<EPhysicalSurface>, FALSXTMeshPaintCriteria> NewMeshPaintCriteria)
{
	FALSXTMeshPaintCriteriaMap PreviousMap;
	PreviousMap.MeshPaintCriteriaMap = ItemMeshPaintCriteria;
	FALSXTMeshPaintCriteriaMap NewMap;
	NewMap.MeshPaintCriteriaMap = NewMeshPaintCriteria;
	OnChangeItemMeshPaintCriteria.Broadcast(PreviousMap, NewMap);
	ItemMeshPaintCriteria = NewMeshPaintCriteria;
}

FGameplayTag UAlsxtPaintableSkeletalMeshComponent::GetElementalCondition()
{
	return ElementalCondition;
}

void UAlsxtPaintableSkeletalMeshComponent::SetElementalCondition(const FGameplayTag NewElementalCondition)
{
	OnChangeElementalCondition.Broadcast(ElementalCondition, NewElementalCondition);
	ElementalCondition = NewElementalCondition;
}

TEnumAsByte<EPhysicalSurface> UAlsxtPaintableSkeletalMeshComponent::GetSurfaceAtLocation(FVector Location)
{
	TEnumAsByte<EPhysicalSurface> NewSurface;

	TArray<FHitResult> OutHits;
	TArray<AActor*> IgnoredActors;
	// IgnoredActors.Add(Character);	// Add Self to Initial Trace Ignored Actors
	TArray<TEnumAsByte<EObjectTypeQuery>> TraceObjectTypes;
	// TraceObjectTypes = ;
	bool isHit = UKismetSystemLibrary::SphereTraceMultiForObjects(GetWorld(), Location, Location, 2, TraceObjectTypes, false, IgnoredActors, EDrawDebugTrace::None, OutHits, true, FLinearColor::Green, FLinearColor::Red, 0.0f);

	if (isHit)
	{
		for (auto& Hit : OutHits)
		{
			if (Hit.GetComponent() == this)
			{				
				NewSurface = Hit.PhysMaterial->SurfaceType;
				return NewSurface;
			}
		}
	}

	return NewSurface;
}

// Check if a Paint Type is Enabled Globally (ALSXT Character Settings), on Server (Delegate Implementable), and in User Preferences (Delegate Implementable). Global, Server and User must be True to return True. Default: All True
bool UAlsxtPaintableSkeletalMeshComponent::CanBePainted(const FGameplayTag PaintType)
{
	return false;
}

// Check if a Surface Type can be painted by Paint Type of Element Surface Type. Performs a search for the provided Surface in the current Criteria Map, and Item Mesh Criteria (if it is set)
bool UAlsxtPaintableSkeletalMeshComponent::ShouldBePainted(TEnumAsByte<EPhysicalSurface> SurfaceType, TEnumAsByte<EPhysicalSurface> ElementSurfaceType, const FGameplayTag PaintType)
{
	return false;
}

void UAlsxtPaintableSkeletalMeshComponent::GetMaterialsForPaintType(const FGameplayTag PaintType, UMaterialInstanceDynamic*& MaterialInstance, UMaterialInstanceDynamic*& FadeMaterialInstance, UTextureRenderTarget2D*& CurrentRenderTarget, UTextureRenderTarget2D*& FadeRenderTarget, FName& ParamName)
{
	if (PaintType == ALSXTMeshPaintTypeTags::BloodDamage)
	{
		MaterialInstance = MIDBloodDamage;
		FadeMaterialInstance = MIDBloodDamageFade;
		RenderTargetAsset = BloodDamageRenderTarget;
		FadeRenderTarget = BloodDamageFadeRenderTarget;
		ParamName = "BloodDamage";
	}
	if (PaintType == ALSXTMeshPaintTypeTags::SurfaceDamage)
	{
		MaterialInstance = MIDSurfaceDamage;
		FadeMaterialInstance = MIDSurfaceDamageFade;
		RenderTargetAsset = SurfaceDamageRenderTarget;
		FadeRenderTarget = SurfaceDamageFadeRenderTarget;
		ParamName = "SurfaceDamage";
	}
	if (PaintType == ALSXTMeshPaintTypeTags::BackSpatter)
	{
		MaterialInstance = MIDBackSpatter;
		FadeMaterialInstance = MIDBackSpatterFade;
		RenderTargetAsset = BackSpatterRenderTarget;
		FadeRenderTarget = BackSpatterFadeRenderTarget;
		ParamName = "BackSpatter";
	}
	if (PaintType == ALSXTMeshPaintTypeTags::Saturation)
	{
		MaterialInstance = MIDSaturation;
		FadeMaterialInstance = MIDSaturationFade;
		RenderTargetAsset = SaturationRenderTarget;
		FadeRenderTarget = SaturationFadeRenderTarget;
		ParamName = "Saturation";
	}
	if (PaintType == ALSXTMeshPaintTypeTags::Burn)
	{
		MaterialInstance = MIDBurn;
		FadeMaterialInstance = MIDBurnFade;
		RenderTargetAsset = BurnRenderTarget;
		FadeRenderTarget = BurnFadeRenderTarget;
		ParamName = "Burn";
	}
}

void UAlsxtPaintableSkeletalMeshComponent::PaintMesh(TEnumAsByte<EPhysicalSurface> SurfaceType, const FGameplayTag PaintType, FVector Location, float Radius)
{
}

void UAlsxtPaintableSkeletalMeshComponent::VolumePaintMesh(TEnumAsByte<EPhysicalSurface> SurfaceType, const FGameplayTag PaintType, FVector Origin, FVector Extent)
{
}

void UAlsxtPaintableSkeletalMeshComponent::ResetChannel(const FGameplayTag PaintType)
{

}

void UAlsxtPaintableSkeletalMeshComponent::ResetAllChannels()
{

}