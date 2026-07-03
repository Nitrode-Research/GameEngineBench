// MIT


#include "Components/Mesh/AlsxtPaintableStaticMeshComponent.h"
#include "Interfaces/AlsxtMeshPaintingInterface.h"
#include "Settings/AlsxtCharacterSettings.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetRenderingLibrary.h"

UAlsxtPaintableStaticMeshComponent::UAlsxtPaintableStaticMeshComponent()
{
	// Bind InitializeMaterials to When Materials Change
}

void UAlsxtPaintableStaticMeshComponent::BeginPlay()
{
	Super::BeginPlay();

	if (IsMeshPaintingConfigured())
	{
		SceneCaptureComponent = IAlsxtMeshPaintingInterface::Execute_GetSceneCaptureComponent(GetOwner());
		GlobalGeneralMeshPaintingSettings = IAlsxtMeshPaintingInterface::Execute_GetGlobalGeneralMeshPaintingSettings(GetOwner());
		if (IsMeshPaintingEnabled())
		{
			// InitializeMaterials();
		}
	}
}

bool UAlsxtPaintableStaticMeshComponent::SetStaticMesh(UStaticMesh* NewMesh)
{
	Super::SetStaticMesh(NewMesh);

	if (GetMaterial(0))
	{
		FAlsxtServerMeshPaintingSettings ServerGeneralMeshPaintingSettings{ IAlsxtMeshPaintingInterface::Execute_GetServerGeneralMeshPaintingSettings(GetOwner()) };
		FAlsxtGeneralMeshPaintingSettings UserGeneralMeshPaintingSettings{ IAlsxtMeshPaintingInterface::Execute_GetUserGeneralMeshPaintingSettings(GetOwner()) };
		if (GlobalGeneralMeshPaintingSettings.GeneralSettings.bEnableMeshPainting && ServerGeneralMeshPaintingSettings.GeneralSettings.bEnableMeshPainting && UserGeneralMeshPaintingSettings.bEnableMeshPainting)
		{
			// InitializeMaterials();
		}
	}

	return true;
}

bool UAlsxtPaintableStaticMeshComponent::IsMeshPaintingConfigured() const
{
	return false;
}

bool UAlsxtPaintableStaticMeshComponent::IsMeshPaintingEnabled() const
{
	return false;
}

void UAlsxtPaintableStaticMeshComponent::InitializeMaterials()
{
}

void UAlsxtPaintableStaticMeshComponent::SetSceneCaptureRenderTarget(UTextureRenderTarget2D* NewRenderTarget)
{
	SceneCaptureComponent->TextureTarget = NewRenderTarget;
}

void UAlsxtPaintableStaticMeshComponent::SetPhysicalMaterialMask(UPhysicalMaterialMask* NewPhysicalMaterialMask)
{
	PhysicalMaterialMask = NewPhysicalMaterialMask;
}

UAlsxtMeshPaintingSettingsMap* UAlsxtPaintableStaticMeshComponent::GetMeshPaintingSettingsMap() const
{
	return MeshPaintingSettingsMap;
}

void UAlsxtPaintableStaticMeshComponent::SetMeshPaintingSettingsMap(UAlsxtMeshPaintingSettingsMap* NewMeshPaintingSettingsMap)
{
	// OnChangeMeshPaintingSettingsMap.Broadcast(MeshPaintingSettingsMap, NewMeshPaintingSettingsMap);
	MeshPaintingSettingsMap = NewMeshPaintingSettingsMap;
}

UAlsxtMeshPaintingSettings*& UAlsxtPaintableStaticMeshComponent::GetMeshPaintingSettings(TEnumAsByte<EPhysicalSurface> SurfaceType)
{
	UAlsxtMeshPaintingSettingsMap* FoundMeshPaintingSettingsMap = MeshPaintingSettingsMap;
	TMap<TEnumAsByte<EPhysicalSurface>, UAlsxtMeshPaintingSettings*> FoundSettings{ MeshPaintingSettingsMap->SettingsMap };
	UAlsxtMeshPaintingSettings*& FoundMeshPaintingSettings { *FoundSettings.Find(SurfaceType) };
	return *&FoundMeshPaintingSettings;
}

void UAlsxtPaintableStaticMeshComponent::SetMeshPaintingSettings(UAlsxtMeshPaintingSettings* NewMeshPaintingSettings)
{
	// OnChangeMeshPaintingSettings.Broadcast(MeshPaintingSettings, NewMeshPaintingSettings);
	MeshPaintingSettings = NewMeshPaintingSettings;
}

FALSXTMeshPaintCriteria UAlsxtPaintableStaticMeshComponent::GetMeshPaintCriteriaEntry(TEnumAsByte<EPhysicalSurface> SurfaceType)
{
	TArray<FALSXTMeshPaintCriteria> NewMeshPaintCriteriaArray{ GetMeshPaintingSettings(SurfaceType)->MeshPaintCriteria };
	FALSXTMeshPaintCriteria NewMeshPaintCriteria;
	return NewMeshPaintCriteria;
}

FALSXTMeshPaintCriteria UAlsxtPaintableStaticMeshComponent::GetItemMeshPaintCriteriaEntry(TEnumAsByte<EPhysicalSurface> SurfaceType)
{
	FALSXTMeshPaintCriteria NewMeshPaintCriteria;
	return NewMeshPaintCriteria;
}

void UAlsxtPaintableStaticMeshComponent::SetMeshPaintCriteria(TMap<TEnumAsByte<EPhysicalSurface>, FALSXTMeshPaintCriteria> NewMeshPaintCriteria)
{
	FALSXTMeshPaintCriteriaMap PreviousMap;
	PreviousMap.MeshPaintCriteriaMap = MeshPaintCriteria;
	FALSXTMeshPaintCriteriaMap NewMap;
	NewMap.MeshPaintCriteriaMap = NewMeshPaintCriteria;
	// OnChangeItemMeshPaintCriteria.Broadcast(PreviousMap, NewMap);
	MeshPaintCriteria = NewMeshPaintCriteria;
}

void UAlsxtPaintableStaticMeshComponent::SetItemMeshPaintCriteria(TMap<TEnumAsByte<EPhysicalSurface>, FALSXTMeshPaintCriteria> NewMeshPaintCriteria)
{
	FALSXTMeshPaintCriteriaMap PreviousMap;
	PreviousMap.MeshPaintCriteriaMap = ItemMeshPaintCriteria;
	FALSXTMeshPaintCriteriaMap NewMap;
	NewMap.MeshPaintCriteriaMap = NewMeshPaintCriteria;
	// OnChangeItemMeshPaintCriteria.Broadcast(PreviousMap, NewMap);
	ItemMeshPaintCriteria = NewMeshPaintCriteria;
}

FGameplayTag UAlsxtPaintableStaticMeshComponent::GetElementalCondition()
{
	return ElementalCondition;
}

void UAlsxtPaintableStaticMeshComponent::SetElementalCondition(const FGameplayTag NewElementalCondition)
{
	// OnChangeElementalCondition.Broadcast(ElementalCondition, NewElementalCondition);
	ElementalCondition = NewElementalCondition;
}

TEnumAsByte<EPhysicalSurface> UAlsxtPaintableStaticMeshComponent::GetSurfaceAtLocation(FVector Location)
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
bool UAlsxtPaintableStaticMeshComponent::CanBePainted(const FGameplayTag PaintType)
{
	return false;
}

// Check if a Surface Type can be painted by Paint Type of Element Surface Type. Performs a search for the provided Surface in the current Criteria Map, and Item Mesh Criteria (if it is set)
bool UAlsxtPaintableStaticMeshComponent::ShouldBePainted(TEnumAsByte<EPhysicalSurface> SurfaceType, TEnumAsByte<EPhysicalSurface> ElementSurfaceType, const FGameplayTag PaintType)
{
	return false;
}

void UAlsxtPaintableStaticMeshComponent::GetMaterialsForPaintType(const FGameplayTag PaintType, UMaterialInstanceDynamic*& MaterialInstance, UMaterialInstanceDynamic*& FadeMaterialInstance, UTextureRenderTarget2D*& RenderTarget, UTextureRenderTarget2D*& FadeRenderTarget, FName& ParamName)
{
	if (PaintType == ALSXTMeshPaintTypeTags::BloodDamage)
	{
		MaterialInstance = MIDBloodDamage;
		FadeMaterialInstance = MIDBloodDamageFade;
		RenderTarget = BloodDamageRenderTarget;
		FadeRenderTarget = BloodDamageFadeRenderTarget;
		ParamName = "BloodDamage";
	}
	if (PaintType == ALSXTMeshPaintTypeTags::SurfaceDamage)
	{
		MaterialInstance = MIDSurfaceDamage;
		FadeMaterialInstance = MIDSurfaceDamageFade;
		RenderTarget = SurfaceDamageRenderTarget;
		FadeRenderTarget = SurfaceDamageFadeRenderTarget;
		ParamName = "SurfaceDamage";
	}
	if (PaintType == ALSXTMeshPaintTypeTags::BackSpatter)
	{
		MaterialInstance = MIDBackSpatter;
		FadeMaterialInstance = MIDBackSpatterFade;
		RenderTarget = BackSpatterRenderTarget;
		FadeRenderTarget = BackSpatterFadeRenderTarget;
		ParamName = "BackSpatter";
	}
	if (PaintType == ALSXTMeshPaintTypeTags::Saturation)
	{
		MaterialInstance = MIDSaturation;
		FadeMaterialInstance = MIDSaturationFade;
		RenderTarget = SaturationRenderTarget;
		FadeRenderTarget = SaturationFadeRenderTarget;
		ParamName = "Saturation";
	}
	if (PaintType == ALSXTMeshPaintTypeTags::Burn)
	{
		MaterialInstance = MIDBurn;
		FadeMaterialInstance = MIDBurnFade;
		RenderTarget = BurnRenderTarget;
		FadeRenderTarget = BurnFadeRenderTarget;
		ParamName = "Burn";
	}
}

void UAlsxtPaintableStaticMeshComponent::PaintMesh(TEnumAsByte<EPhysicalSurface> SurfaceType, const FGameplayTag PaintType, FVector Location, float Radius)
{
}

void UAlsxtPaintableStaticMeshComponent::VolumePaintMesh(TEnumAsByte<EPhysicalSurface> SurfaceType, const FGameplayTag PaintType, FVector Origin, FVector Extent)
{
}

void UAlsxtPaintableStaticMeshComponent::ResetChannel(const FGameplayTag PaintType)
{

}

void UAlsxtPaintableStaticMeshComponent::ResetAllChannels()
{

}