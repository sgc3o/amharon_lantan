#include "LanternCloner.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/UObjectGlobals.h"

DEFINE_LOG_CATEGORY_STATIC(LogLanternCloner, Log, All);

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
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BodyARed2(TEXT("/Game/c4d/rantan/Materials/MI_LanternBody_A_Red2.MI_LanternBody_A_Red2"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BodyCYellow2(TEXT("/Game/c4d/rantan/Materials/MI_LanternBody_C_Yellow2.MI_LanternBody_C_Yellow2"));

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
	AddVariant(TEXT("A_Red2"), 0, 0, BodyA.Object, BodyARed2.Object, BodyTransformA, TasselARed.Object, TasselTransformA, FrameA.Object, FrameTransformA);
	AddVariant(TEXT("C_Yellow2"), 2, 5, BodyBC.Object, BodyCYellow2.Object, BodyTransformC, TasselCYellow.Object, TasselTransformC, FrameC.Object, FrameTransformC);
}

void ALanternCloner::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	// Construction also runs during editor load, property edits and BP reinstancing.
	// Saved instances are authoritative until an explicit generation command.
	UE_LOG(LogLanternCloner, Log, TEXT("OnConstruction %s: no generation"), *GetPathName());
}

void ALanternCloner::PostLoad()
{
	Super::PostLoad();
	UE_LOG(LogLanternCloner, Log, TEXT("PostLoad %s: no instance mutation"), *GetPathName());
}

void ALanternCloner::PostRegisterAllComponents()
{
	Super::PostRegisterAllComponents();
	UE_LOG(LogLanternCloner, Log, TEXT("PostRegisterAllComponents %s: no generation"), *GetPathName());
}

#if WITH_EDITOR
void ALanternCloner::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	UE_LOG(LogLanternCloner, Log, TEXT("PostEditChangeProperty %s: explicit Preview/Generate required"), *GetPathName());
}

void ALanternCloner::PostEditMove(bool bFinished)
{
	Super::PostEditMove(bFinished);
	UE_LOG(LogLanternCloner, Verbose, TEXT("PostEditMove %s: no generation"), *GetPathName());
}
#endif

bool ALanternCloner::CanUpdateInstances() const
{
	const UWorld* World = GetWorld();
	if (!IsInGameThread() || IsTemplate() || bUpdatingInstances || !World
		|| HasAnyFlags(RF_NeedLoad | RF_NeedPostLoad | RF_NeedPostLoadSubobjects)
		|| IsRunningUserConstructionScript() || World->bIsRunningConstructionScript
		|| (World->WorldType != EWorldType::Editor && !World->IsGameWorld()))
	{
		UE_LOG(LogLanternCloner, Warning, TEXT("Generation rejected during load/construction/reentry: %s"), *GetPathName());
		return false;
	}
	TArray<UHierarchicalInstancedStaticMeshComponent*> Components;
	GetComponents(Components);
	for (const UHierarchicalInstancedStaticMeshComponent* Component : Components)
	{
		if (IsValid(Component) && Component->ComponentTags.Contains(LanternClonerNames::GeneratedTag)
			&& (Component->IsAsyncBuilding() || Component->IsCompiling()
				|| Component->HasAnyFlags(RF_NeedLoad | RF_NeedPostLoad)))
		{
			// Do not pump the task graph or partially clear other buckets. Retry explicitly
			// after the engine's pending build/mesh compilation has completed.
			UE_LOG(LogLanternCloner, Warning, TEXT("Generation deferred: %s is still building; retry the button when ready"), *Component->GetPathName());
			return false;
		}
	}
	return true;
}

bool ALanternCloner::IsGeneratedDataConsistent() const
{
	TArray<UHierarchicalInstancedStaticMeshComponent*> Components;
	GetComponents(Components);
	for (const UHierarchicalInstancedStaticMeshComponent* Component : Components)
	{
		if (!IsValid(Component) || !Component->ComponentTags.Contains(LanternClonerNames::GeneratedTag))
		{
			continue;
		}
		const int32 Count = Component->GetInstanceCount();
		if (Component->IsAsyncBuilding() || Component->IsCompiling() || !Component->IsTreeFullyBuilt()
			|| Component->InstanceReorderTable.Num() != Count || Component->NumBuiltInstances != Count)
		{
			return false;
		}
	}
	return true;
}

void ALanternCloner::Generate()
{
	RebuildInstances();
}

void ALanternCloner::Regenerate()
{
	RebuildInstances();
}

void ALanternCloner::Preview()
{
	RebuildInstances(true);
}

void ALanternCloner::Clear()
{
	if (!CanUpdateInstances())
	{
		return;
	}
	TGuardValue<bool> UpdateGuard(bUpdatingInstances, true);
	Modify();
	bGenerationEnabled = false;
	ClearGeneratedComponents();
	UE_LOG(LogLanternCloner, Log, TEXT("Clear complete %s: components retained"), *GetPathName());
}

void ALanternCloner::ClearGeneratedComponents()
{
	TArray<UHierarchicalInstancedStaticMeshComponent*> ExistingComponents;
	GetComponents<UHierarchicalInstancedStaticMeshComponent>(ExistingComponents);
	for (UHierarchicalInstancedStaticMeshComponent* Component : ExistingComponents)
	{
		if (IsValid(Component) && Component->ComponentTags.Contains(LanternClonerNames::GeneratedTag))
		{
			Component->Modify();
			Component->bAutoRebuildTreeOnInstanceChanges = false;
			Component->ClearInstances();
			// UE 5.4 ClearInstances does not empty InstanceReorderTable itself.
			// ApplyEmpty through the public sync builder before any new additions.
			Component->BuildTreeIfOutdated(false, true);
		}
	}

	GeneratedComponents.Reset();
	GeneratedLanternCount = 0;
	GeneratedMeshInstanceCount = 0;
	GeneratedHISMGroupCount = 0;
	GeneratedVariantCounts.Reset();
	MaxConsecutiveSameShapeRun = 0;
	MaxConsecutiveSameColorRun = 0;
	ForegroundCenterCandidateCount = 0;
	GeneratedForegroundCenterCount = 0;
	GeneratedMidFarCenterCount = 0;
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
	TArray<UHierarchicalInstancedStaticMeshComponent*> ExistingComponents;
	GetComponents(ExistingComponents);
	for (UHierarchicalInstancedStaticMeshComponent* Existing : ExistingComponents)
	{
		if (IsValid(Existing) && Existing->ComponentTags.Contains(LanternClonerNames::GeneratedTag)
			&& Existing->GetStaticMesh() == Mesh && Existing->GetMaterial(0) == (Material ? Material : Mesh->GetMaterial(0)))
		{
			Existing->SetCastShadow(bCastShadows);
			Buckets.Add(Key, Existing);
			GeneratedComponents.Add(Existing);
			return Existing;
		}
	}

	const FName ComponentName = MakeUniqueObjectName(this, UHierarchicalInstancedStaticMeshComponent::StaticClass(), TEXT("HISM_Lantern"));
	UHierarchicalInstancedStaticMeshComponent* Component = NewObject<UHierarchicalInstancedStaticMeshComponent>(this, ComponentName, RF_Transactional);
	Component->bAutoRebuildTreeOnInstanceChanges = false;
	Component->CreationMethod = EComponentCreationMethod::Instance;
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

void ALanternCloner::RefillShuffleBag(FRandomStream& Stream, TArray<int32>& Bag, int32 PreviousVariantIndex) const
{
	TArray<int32> BaseBag;
	for (int32 VariantIndex = 0; VariantIndex < LanternVariants.Num(); ++VariantIndex)
	{
		const int32 Tickets = FMath::Max(1, FMath::RoundToInt(LanternVariants[VariantIndex].Weight));
		for (int32 Ticket = 0; Ticket < Tickets; ++Ticket)
		{
			BaseBag.Add(VariantIndex);
		}
	}

	for (int32 Attempt = 0; Attempt < 64; ++Attempt)
	{
		Bag = BaseBag;
		for (int32 Index = Bag.Num() - 1; Index > 0; --Index)
		{
			const int32 SwapIndex = Stream.RandRange(0, Index);
			Bag.Swap(Index, SwapIndex);
		}

		bool bValidOrder = true;
		int32 PreviousInSequence = PreviousVariantIndex;
		for (int32 Position = Bag.Num() - 1; Position >= 0; --Position)
		{
			if (LanternVariants.IsValidIndex(PreviousInSequence))
			{
				const FLanternVariantDefinition& Previous = LanternVariants[PreviousInSequence];
				const FLanternVariantDefinition& Candidate = LanternVariants[Bag[Position]];
				const bool bSameShape = bAvoidConsecutiveSameShape && Candidate.ShapeId == Previous.ShapeId;
				const bool bSameColor = bAvoidConsecutiveSameColor && Candidate.ColorId == Previous.ColorId;
				if (bSameShape || bSameColor)
				{
					bValidOrder = false;
					break;
				}
			}
			PreviousInSequence = Bag[Position];
		}

		if (bValidOrder)
		{
			return;
		}
	}
}

int32 ALanternCloner::PopVariantFromShuffleBag(FRandomStream& Stream, TArray<int32>& Bag, int32 PreviousVariantIndex)
{
	if (Bag.IsEmpty())
	{
		RefillShuffleBag(Stream, Bag, PreviousVariantIndex);
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

int32 ALanternCloner::PickRandomVariant(FRandomStream& Stream) const
{
	float TotalWeight = 0.0f;
	for (const FLanternVariantDefinition& Variant : LanternVariants)
	{
		TotalWeight += FMath::Max(0.0f, Variant.Weight);
	}
	if (TotalWeight <= 0.0f)
	{
		return LanternVariants.IsEmpty() ? INDEX_NONE : Stream.RandRange(0, LanternVariants.Num() - 1);
	}
	float Choice = Stream.FRandRange(0.0f, TotalWeight);
	for (int32 Index = 0; Index < LanternVariants.Num(); ++Index)
	{
		Choice -= FMath::Max(0.0f, LanternVariants[Index].Weight);
		if (Choice <= 0.0f)
		{
			return Index;
		}
	}
	return LanternVariants.Num() - 1;
}

void ALanternCloner::RebuildInstances(bool bPreview)
{
	if (!CanUpdateInstances())
	{
		return;
	}
	// A mesh completing compilation can initiate an engine build; avoid mutating
	// any bucket until every input mesh is ready.
	for (const FLanternVariantDefinition& Variant : LanternVariants)
	{
		for (UStaticMesh* Mesh : { Variant.BodyMesh.Get(), Variant.TasselMesh.Get(), Variant.FrameMesh.Get() })
		{
			if (Mesh && Mesh->IsCompiling())
			{
				UE_LOG(LogLanternCloner, Warning, TEXT("Generation deferred: mesh compilation in progress"));
				return;
			}
		}
	}
	TGuardValue<bool> UpdateGuard(bUpdatingInstances, true);
	Modify();
	bGenerationEnabled = true;
	UE_LOG(LogLanternCloner, Log, TEXT("Explicit %s start %s"), bPreview ? TEXT("Preview") : TEXT("Generate"), *GetPathName());
	ClearGeneratedComponents();
	if (!bGenerationEnabled || LanternVariants.IsEmpty())
	{
		return;
	}

	const int32 RequestedVerticalCount = FMath::Max(1, VerticalCount);
	const int32 EffectiveVerticalCount = bPreview && bUsePreviewVerticalCount
		? FMath::Min(RequestedVerticalCount, FMath::Max(1, PreviewVerticalCount))
		: RequestedVerticalCount;
	const float EffectiveScaleRandom = FMath::Clamp(ScaleRandom, 0.0f, 0.5f);

	FRandomStream Stream(RandomSeed);
	TArray<int32> ShuffleBag;
	TMap<FString, UHierarchicalInstancedStaticMeshComponent*> Buckets;
	TMap<UHierarchicalInstancedStaticMeshComponent*, TArray<FTransform>> PendingTransforms;
	const int32 PreviewLimit = FMath::Clamp(PreviewMaxLanternCount, 1, 256);
	int32 VisitedPreviewSlots = 0;
	int32 PreviousVariantIndex = INDEX_NONE;
	int32 CurrentSameShapeRun = 0;
	int32 CurrentSameColorRun = 0;
	GeneratedVariantCounts.Init(0, LanternVariants.Num());

	for (int32 VerticalIndex = 0; VerticalIndex < EffectiveVerticalCount && (!bPreview || VisitedPreviewSlots < PreviewLimit); ++VerticalIndex)
	{
		++VisitedPreviewSlots;
		const float Z = static_cast<float>(VerticalIndex) * VerticalSpacing;

		const int32 VariantIndex = CloneMode == ELanternCloneMode::Shuffle
			? PopVariantFromShuffleBag(Stream, ShuffleBag, PreviousVariantIndex)
			: PickRandomVariant(Stream);
		if (!LanternVariants.IsValidIndex(VariantIndex))
		{
			continue;
		}
		const FLanternVariantDefinition& Variant = LanternVariants[VariantIndex];
		if (LanternVariants.IsValidIndex(PreviousVariantIndex))
		{
			const FLanternVariantDefinition& PreviousVariant = LanternVariants[PreviousVariantIndex];
			CurrentSameShapeRun = PreviousVariant.ShapeId == Variant.ShapeId ? CurrentSameShapeRun + 1 : 1;
			CurrentSameColorRun = PreviousVariant.ColorId == Variant.ColorId ? CurrentSameColorRun + 1 : 1;
		}
		else
		{
			CurrentSameShapeRun = 1;
			CurrentSameColorRun = 1;
		}
		MaxConsecutiveSameShapeRun = FMath::Max(MaxConsecutiveSameShapeRun, CurrentSameShapeRun);
		MaxConsecutiveSameColorRun = FMath::Max(MaxConsecutiveSameColorRun, CurrentSameColorRun);
		++GeneratedVariantCounts[VariantIndex];
		PreviousVariantIndex = VariantIndex;
		const FVector Jitter(
			Stream.FRandRange(-RandomPositionOffset.X, RandomPositionOffset.X),
			Stream.FRandRange(-RandomPositionOffset.Y, RandomPositionOffset.Y),
			Stream.FRandRange(-RandomPositionOffset.Z, RandomPositionOffset.Z));
		const FRotator RandomRotation(
			Stream.FRandRange(-RandomPitchRange, RandomPitchRange),
			Stream.FRandRange(-RandomYawRange, RandomYawRange),
			Stream.FRandRange(-RandomRollRange, RandomRollRange));
		const float UniformScale = Stream.FRandRange(1.0f - EffectiveScaleRandom, 1.0f + EffectiveScaleRandom);
		const FTransform LanternTransform(RandomRotation, FVector(0.0f, 0.0f, Z) + Jitter, FVector(UniformScale));

		auto AddPart = [&Buckets, &PendingTransforms, &LanternTransform, this](UStaticMesh* Mesh, UMaterialInterface* Material, const FTransform& PartLocalTransform)
		{
			if (UHierarchicalInstancedStaticMeshComponent* Bucket = FindOrCreateBucket(Mesh, Material, Buckets))
				{
				PendingTransforms.FindOrAdd(Bucket).Add(PartLocalTransform * LanternTransform);
				++GeneratedMeshInstanceCount;
			}
		};

		AddPart(Variant.BodyMesh, Variant.BodyMaterial, Variant.BodyLocalTransform);
		AddPart(Variant.TasselMesh, Variant.TasselMaterial, Variant.TasselLocalTransform);
		AddPart(Variant.FrameMesh, Variant.FrameMaterial, Variant.FrameLocalTransform);
		++GeneratedLanternCount;
	}

	for (const TPair<FString, UHierarchicalInstancedStaticMeshComponent*>& Pair : Buckets)
	{
		Pair.Value->AddInstances(PendingTransforms.FindChecked(Pair.Value), false, false, false);
		Pair.Value->BuildTreeIfOutdated(false, true);
		if (IsValid(Pair.Value) && !Pair.Value->IsRegistered())
		{
			Pair.Value->RegisterComponent();
		}
	}
	GeneratedHISMGroupCount = Buckets.Num();
	UE_LOG(LogLanternCloner, Log, TEXT("Generation complete %s: lanterns=%d meshInstances=%d groups=%d sync build"),
		*GetPathName(), GeneratedLanternCount, GeneratedMeshInstanceCount, GeneratedHISMGroupCount);
}
