#include "LanternCloner.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace LanternClonerNames
{
	const FName GeneratedTag(TEXT("LanternClonerGenerated"));
}

ALanternCloner::ALanternCloner()
{
	PrimaryActorTick.bCanEverTick = false;
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SceneRoot->SetMobility(EComponentMobility::Static);
	RootComponent = SceneRoot;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> BodyA(TEXT("/Game/c4d/rantan/Geometries/LanternBody_Paper.LanternBody_Paper"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> BodyBC(TEXT("/Game/c4d/rantan/Geometries/LanternBody_Paper_2.LanternBody_Paper_2"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> BodyD(TEXT("/Game/c4d/rantan/Geometries/LanternBody_Paper_3.LanternBody_Paper_3"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Tassel(TEXT("/Game/c4d/rantan/Geometries/Tassel_Paper.Tassel_Paper"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> FrameA(TEXT("/Game/c4d/rantan/Geometries/Frame.Frame"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> FrameB(TEXT("/Game/c4d/rantan/Geometries/Frame_2.Frame_2"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> FrameC(TEXT("/Game/c4d/rantan/Geometries/Frame_3.Frame_3"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> FrameD(TEXT("/Game/c4d/rantan/Geometries/Frame_4.Frame_4"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> FrameMaterial(TEXT("/Game/c4d/rantan/Materials/M_Frame.M_Frame"));

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BodyARed(TEXT("/Game/c4d/rantan/Materials/MI_LanternBody_A_Red.MI_LanternBody_A_Red"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BodyABlue(TEXT("/Game/c4d/rantan/Materials/MI_LanternBody_A_Blue.MI_LanternBody_A_Blue"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BodyBCyan(TEXT("/Game/c4d/rantan/Materials/MI_LanternBody_B_Cyan.MI_LanternBody_B_Cyan"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BodyBPink(TEXT("/Game/c4d/rantan/Materials/MI_LanternBody_B_Pink.MI_LanternBody_B_Pink"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BodyCGreen(TEXT("/Game/c4d/rantan/Materials/MI_LanternBody_C_Green.MI_LanternBody_C_Green"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BodyCYellow(TEXT("/Game/c4d/rantan/Materials/MI_LanternBody_C_Yellow.MI_LanternBody_C_Yellow"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BodyDOrange(TEXT("/Game/c4d/rantan/Materials/MI_LanternBody_D_Orange.MI_LanternBody_D_Orange"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BodyDPurple(TEXT("/Game/c4d/rantan/Materials/MI_LanternBody_D_Purple.MI_LanternBody_D_Purple"));

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> TasselARed(TEXT("/Game/c4d/rantan/Materials/MI_LanternTassel_A_Red.MI_LanternTassel_A_Red"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> TasselABlue(TEXT("/Game/c4d/rantan/Materials/MI_LanternTassel_A_Blue.MI_LanternTassel_A_Blue"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> TasselBCyan(TEXT("/Game/c4d/rantan/Materials/MI_LanternTassel_B_Cyan.MI_LanternTassel_B_Cyan"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> TasselBPink(TEXT("/Game/c4d/rantan/Materials/MI_LanternTassel_B_Pink.MI_LanternTassel_B_Pink"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> TasselCGreen(TEXT("/Game/c4d/rantan/Materials/MI_LanternTassel_C_Green.MI_LanternTassel_C_Green"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> TasselCYellow(TEXT("/Game/c4d/rantan/Materials/MI_LanternTassel_C_Yellow.MI_LanternTassel_C_Yellow"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> TasselDOrange(TEXT("/Game/c4d/rantan/Materials/MI_LanternTassel_D_Orange.MI_LanternTassel_D_Orange"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> TasselDPurple(TEXT("/Game/c4d/rantan/Materials/MI_LanternTassel_D_Purple.MI_LanternTassel_D_Purple"));

	auto AddVariant = [this](
		const TCHAR* Name,
		int32 ShapeId,
		int32 ColorId,
		UStaticMesh* BodyMesh,
		UMaterialInterface* BodyMat,
		const FTransform& BodyTransform,
		UMaterialInterface* TasselMat,
		const FTransform& TasselTransform,
		UStaticMesh* FrameMesh,
		const FTransform& FrameTransform)
	{
		FLanternVariantDefinition& Variant = LanternVariants.AddDefaulted_GetRef();
		Variant.VariantName = FName(Name);
		Variant.ShapeId = ShapeId;
		Variant.ColorId = ColorId;
		Variant.BodyMesh = BodyMesh;
		Variant.BodyMaterial = BodyMat;
		Variant.BodyLocalTransform = BodyTransform;
		Variant.TasselMesh = Tassel.Object;
		Variant.TasselMaterial = TasselMat;
		Variant.TasselLocalTransform = TasselTransform;
		Variant.FrameMesh = FrameMesh;
		Variant.FrameMaterial = FrameMaterial.Object;
		Variant.FrameLocalTransform = FrameTransform;
	};

	const FTransform BodyTransformA(FRotator::ZeroRotator, FVector(0.0, 0.0, -15.375925));
	const FTransform TasselTransformA(FRotator::ZeroRotator, FVector(0.0, 0.0, -47.881208));
	const FTransform FrameTransformA(FRotator::ZeroRotator, FVector(0.0, 0.0, -15.375925));
	const FTransform BodyTransformB(FRotator::ZeroRotator, FVector(0.0, 0.0, -43.703642));
	const FTransform TasselTransformB(FRotator::ZeroRotator, FVector(0.0, 0.0, -62.703321));
	const FTransform FrameTransformB(FRotator::ZeroRotator, FVector(0.0, 0.0, -30.109941));
	const FTransform BodyTransformC(FRotator(0.0, 0.0, 180.0), FVector(0.0, 0.0, -1.243888));
	const FTransform TasselTransformC(FRotator::ZeroRotator, FVector(0.0, 0.0, -62.703321));
	const FTransform FrameTransformC(FRotator::ZeroRotator, FVector(0.0, 0.0, -30.109941));
	const FTransform BodyTransformD(FRotator::ZeroRotator, FVector(0.0, 0.0, -23.348921));
	const FTransform TasselTransformD(FRotator::ZeroRotator, FVector(0.0, 0.0, -63.593380));
	const FTransform FrameTransformD(FRotator::ZeroRotator, FVector(0.0, 0.0, -31.000000));

	AddVariant(TEXT("A_Red"), 0, 0, BodyA.Object, BodyARed.Object, BodyTransformA, TasselARed.Object, TasselTransformA, FrameA.Object, FrameTransformA);
	AddVariant(TEXT("A_Blue"), 0, 1, BodyA.Object, BodyABlue.Object, BodyTransformA, TasselABlue.Object, TasselTransformA, FrameA.Object, FrameTransformA);
	AddVariant(TEXT("B_Cyan"), 1, 2, BodyBC.Object, BodyBCyan.Object, BodyTransformB, TasselBCyan.Object, TasselTransformB, FrameB.Object, FrameTransformB);
	AddVariant(TEXT("B_Pink"), 1, 3, BodyBC.Object, BodyBPink.Object, BodyTransformB, TasselBPink.Object, TasselTransformB, FrameB.Object, FrameTransformB);
	AddVariant(TEXT("C_Green"), 2, 4, BodyBC.Object, BodyCGreen.Object, BodyTransformC, TasselCGreen.Object, TasselTransformC, FrameC.Object, FrameTransformC);
	AddVariant(TEXT("C_Yellow"), 2, 5, BodyBC.Object, BodyCYellow.Object, BodyTransformC, TasselCYellow.Object, TasselTransformC, FrameC.Object, FrameTransformC);
	AddVariant(TEXT("D_Orange"), 3, 6, BodyD.Object, BodyDOrange.Object, BodyTransformD, TasselDOrange.Object, TasselTransformD, FrameD.Object, FrameTransformD);
	AddVariant(TEXT("D_Purple"), 3, 7, BodyD.Object, BodyDPurple.Object, BodyTransformD, TasselDPurple.Object, TasselTransformD, FrameD.Object, FrameTransformD);
}

void ALanternCloner::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	if (bGenerationEnabled)
	{
		RebuildInstances();
	}
	else
	{
		ClearGeneratedComponents();
	}
}

void ALanternCloner::Generate()
{
	bGenerationEnabled = true;
	RebuildInstances();
}

void ALanternCloner::Regenerate()
{
	bGenerationEnabled = true;
	RebuildInstances();
}

void ALanternCloner::Clear()
{
	bGenerationEnabled = false;
	ClearGeneratedComponents();
}

void ALanternCloner::ClearGeneratedComponents()
{
	TArray<UHierarchicalInstancedStaticMeshComponent*> ExistingComponents;
	GetComponents<UHierarchicalInstancedStaticMeshComponent>(ExistingComponents);
	for (UHierarchicalInstancedStaticMeshComponent* Component : ExistingComponents)
	{
		if (IsValid(Component) && Component->ComponentTags.Contains(LanternClonerNames::GeneratedTag))
		{
			Component->ClearInstances();
			Component->DestroyComponent();
		}
	}

	GeneratedComponents.Reset();
	GeneratedLanternCount = 0;
	GeneratedMeshInstanceCount = 0;
	GeneratedHISMGroupCount = 0;
}

UHierarchicalInstancedStaticMeshComponent* ALanternCloner::FindOrCreateBucket(
	UStaticMesh* Mesh,
	UMaterialInterface* Material,
	TMap<FString, UHierarchicalInstancedStaticMeshComponent*>& Buckets)
{
	if (!Mesh)
	{
		return nullptr;
	}

	const FString MaterialPath = Material ? Material->GetPathName() : TEXT("None");
	const FString Key = Mesh->GetPathName() + TEXT("|") + MaterialPath;
	if (UHierarchicalInstancedStaticMeshComponent** Existing = Buckets.Find(Key))
	{
		return *Existing;
	}

	const FName ComponentName(*FString::Printf(TEXT("HISM_Lantern_%02d"), Buckets.Num()));
	UHierarchicalInstancedStaticMeshComponent* Component = NewObject<UHierarchicalInstancedStaticMeshComponent>(this, ComponentName, RF_Transactional);
	Component->CreationMethod = EComponentCreationMethod::UserConstructionScript;
	Component->ComponentTags.Add(LanternClonerNames::GeneratedTag);
	Component->SetupAttachment(SceneRoot);
	Component->SetStaticMesh(Mesh);
	if (Material)
	{
		Component->SetMaterial(0, Material);
	}
	Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Component->SetGenerateOverlapEvents(false);
	Component->SetCanEverAffectNavigation(false);
	Component->SetCastShadow(bCastShadows);
	Component->SetMobility(EComponentMobility::Static);
	Component->SetOwnerNoSee(false);
	Component->SetOnlyOwnerSee(false);
	Component->SetHiddenInGame(false);
	Component->SetVisibility(true);
	Component->SetRenderInMainPass(true);
	AddInstanceComponent(Component);

	Buckets.Add(Key, Component);
	GeneratedComponents.Add(Component);
	return Component;
}

void ALanternCloner::RefillShuffleBag(FRandomStream& Stream, TArray<int32>& Bag) const
{
	Bag.Reset();
	for (int32 VariantIndex = 0; VariantIndex < LanternVariants.Num(); ++VariantIndex)
	{
		const int32 Tickets = FMath::Max(1, FMath::RoundToInt(LanternVariants[VariantIndex].Weight));
		for (int32 Ticket = 0; Ticket < Tickets; ++Ticket)
		{
			Bag.Add(VariantIndex);
		}
	}

	for (int32 Index = Bag.Num() - 1; Index > 0; --Index)
	{
		const int32 SwapIndex = Stream.RandRange(0, Index);
		Bag.Swap(Index, SwapIndex);
	}
}

int32 ALanternCloner::PopVariantFromShuffleBag(FRandomStream& Stream, TArray<int32>& Bag, int32 PreviousVariantIndex)
{
	if (Bag.IsEmpty())
	{
		RefillShuffleBag(Stream, Bag);
	}
	if (Bag.IsEmpty())
	{
		return INDEX_NONE;
	}

	int32 SelectedPosition = Bag.Num() - 1;
	if (LanternVariants.IsValidIndex(PreviousVariantIndex))
	{
		const FLanternVariantDefinition& Previous = LanternVariants[PreviousVariantIndex];
		for (int32 Position = Bag.Num() - 1; Position >= 0; --Position)
		{
			const FLanternVariantDefinition& Candidate = LanternVariants[Bag[Position]];
			const bool bSameShape = bAvoidConsecutiveSameShape && Candidate.ShapeId == Previous.ShapeId;
			const bool bSameColor = bAvoidConsecutiveSameColor && Candidate.ColorId == Previous.ColorId;
			if (!bSameShape && !bSameColor)
			{
				SelectedPosition = Position;
				break;
			}
		}
	}

	const int32 Result = Bag[SelectedPosition];
	Bag.RemoveAt(SelectedPosition, 1, EAllowShrinking::No);
	return Result;
}

float ALanternCloner::GetEffectiveVerticalSpacing(int32 EffectiveVerticalCount) const
{
	if (HeightMode == ELanternHeightMode::TotalHeight && EffectiveVerticalCount > 1)
	{
		return TotalHeight / static_cast<float>(EffectiveVerticalCount - 1);
	}
	return VerticalSpacing;
}

void ALanternCloner::RebuildInstances()
{
	ClearGeneratedComponents();
	if (!bGenerationEnabled || LanternVariants.IsEmpty())
	{
		return;
	}

	const int32 EffectiveColumnCount = FMath::Max(1, ColumnCount);
	const int32 EffectiveDepthCount = FMath::Max(1, DepthCount);
	const int32 RequestedVerticalCount = FMath::Max(1, VerticalCount);
	const int32 EffectiveVerticalCount = bUsePreviewVerticalCount
		? FMath::Min(RequestedVerticalCount, FMath::Max(1, PreviewVerticalCount))
		: RequestedVerticalCount;
	const float EffectiveVerticalSpacing = GetEffectiveVerticalSpacing(EffectiveVerticalCount);
	const float EffectiveScaleMin = FMath::Min(RandomScaleMin, RandomScaleMax);
	const float EffectiveScaleMax = FMath::Max(RandomScaleMin, RandomScaleMax);

	FRandomStream Stream(RandomSeed);
	TArray<int32> ShuffleBag;
	TMap<FString, UHierarchicalInstancedStaticMeshComponent*> Buckets;
	int32 PreviousVariantIndex = INDEX_NONE;

	for (int32 VerticalIndex = 0; VerticalIndex < EffectiveVerticalCount; ++VerticalIndex)
	{
		for (int32 DepthIndex = 0; DepthIndex < EffectiveDepthCount; ++DepthIndex)
		{
			for (int32 ColumnIndex = 0; ColumnIndex < EffectiveColumnCount; ++ColumnIndex)
			{
				const float X = (static_cast<float>(ColumnIndex) - 0.5f * static_cast<float>(EffectiveColumnCount - 1)) * HorizontalSpacing;
				const float Y = -static_cast<float>(DepthIndex) * DepthSpacing;
				const float Z = static_cast<float>(VerticalIndex) * EffectiveVerticalSpacing;

				float LocalDensity = FMath::Clamp(Density, 0.0f, 1.0f);
				const bool bForegroundLayer = DepthIndex < FMath::Max(1, ForegroundDepthLayerCount);
				const bool bInsideForegroundCenter = FMath::Abs(X) <= ForegroundGapHalfWidth;
				if (bEnableForegroundCenterGap && bForegroundLayer && bInsideForegroundCenter)
				{
					LocalDensity *= bHardExcludeForegroundCenter ? 0.0f : FMath::Clamp(ForegroundCenterDensity, 0.0f, 1.0f);
				}

				if (Stream.FRand() > LocalDensity)
				{
					continue;
				}

				const int32 VariantIndex = PopVariantFromShuffleBag(Stream, ShuffleBag, PreviousVariantIndex);
				if (!LanternVariants.IsValidIndex(VariantIndex))
				{
					continue;
				}
				PreviousVariantIndex = VariantIndex;
				const FLanternVariantDefinition& Variant = LanternVariants[VariantIndex];

				const FVector Jitter(
					Stream.FRandRange(-RandomPositionOffset.X, RandomPositionOffset.X),
					Stream.FRandRange(-RandomPositionOffset.Y, RandomPositionOffset.Y),
					Stream.FRandRange(-RandomPositionOffset.Z, RandomPositionOffset.Z));
				const FRotator RandomRotation(
					Stream.FRandRange(-RandomPitchRange, RandomPitchRange),
					Stream.FRandRange(-RandomYawRange, RandomYawRange),
					Stream.FRandRange(-RandomRollRange, RandomRollRange));
				const float UniformScale = Stream.FRandRange(EffectiveScaleMin, EffectiveScaleMax);
				const FTransform LanternTransform(RandomRotation, FVector(X, Y, Z) + Jitter, FVector(UniformScale));

				auto AddPart = [&Buckets, &LanternTransform, this](UStaticMesh* Mesh, UMaterialInterface* Material, const FTransform& PartLocalTransform)
				{
					if (UHierarchicalInstancedStaticMeshComponent* Bucket = FindOrCreateBucket(Mesh, Material, Buckets))
					{
						Bucket->AddInstance(PartLocalTransform * LanternTransform, false);
						++GeneratedMeshInstanceCount;
					}
				};

				AddPart(Variant.BodyMesh, Variant.BodyMaterial, Variant.BodyLocalTransform);
				AddPart(Variant.TasselMesh, Variant.TasselMaterial, Variant.TasselLocalTransform);
				AddPart(Variant.FrameMesh, Variant.FrameMaterial, Variant.FrameLocalTransform);
				++GeneratedLanternCount;
			}
		}
	}

	for (const TPair<FString, UHierarchicalInstancedStaticMeshComponent*>& Pair : Buckets)
	{
		if (IsValid(Pair.Value) && !Pair.Value->IsRegistered())
		{
			Pair.Value->RegisterComponent();
		}
	}
	GeneratedHISMGroupCount = Buckets.Num();
}
