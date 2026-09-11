#include "ClonerBatchController.h"

#include "Cloner/CEClonerActor.h"
#include "Cloner/Layouts/CEClonerCircleLayout.h"
#include "Cloner/Layouts/CEClonerLineLayout.h"
#include "Cloner/Layouts/CEClonerMeshLayout.h"
#include "Cloner/Layouts/CEClonerSphereRandomLayout.h"
#include "Cloner/Layouts/CEClonerSphereUniformLayout.h"
#include "Cloner/Layouts/CEClonerSplineLayout.h"
#include "Components/SceneComponent.h"
#include "EngineUtils.h"

#if WITH_EDITOR
#include "ScopedTransaction.h"
#endif

DEFINE_LOG_CATEGORY_STATIC(LogClonerBatchController, Log, All);

namespace ClonerBatchControllerPrivate
{
	template <typename LayoutType>
	bool SetCountIfLayout(UCEClonerLayoutBase* Layout, int32 NewCount)
	{
		if (LayoutType* TypedLayout = Cast<LayoutType>(Layout))
		{
			TypedLayout->Modify();
			TypedLayout->SetCount(NewCount);
			return true;
		}
		return false;
	}

	template <typename LayoutType>
	bool GetCountIfLayout(const UCEClonerLayoutBase* Layout, int32& OutCount)
	{
		if (const LayoutType* TypedLayout = Cast<LayoutType>(Layout))
		{
			OutCount = TypedLayout->GetCount();
			return true;
		}
		return false;
	}

	bool NameMatchesGroup(const FString& Value, const FString& GroupName)
	{
		return Value.Equals(GroupName, ESearchCase::IgnoreCase)
			|| Value.Contains(GroupName + TEXT("_"), ESearchCase::IgnoreCase)
			|| Value.Contains(GroupName + TEXT(" "), ESearchCase::IgnoreCase);
	}
}

AClonerBatchController::AClonerBatchController()
{
	PrimaryActorTick.bCanEverTick = false;
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;
}

bool AClonerBatchController::IsInNamedGroup(const ACEClonerActor* Cloner, const FString& GroupName)
{
	if (!IsValid(Cloner))
	{
		return false;
	}

	const FName GroupTag(*GroupName);
	if (Cloner->ActorHasTag(GroupTag)
		|| ClonerBatchControllerPrivate::NameMatchesGroup(Cloner->GetActorLabel(), GroupName))
	{
		return true;
	}

#if WITH_EDITOR
	FString FolderPath = Cloner->GetFolderPath().ToString();
	FolderPath.ReplaceInline(TEXT("\\"), TEXT("/"));
	TArray<FString> Segments;
	FolderPath.ParseIntoArray(Segments, TEXT("/"), true);
	for (const FString& Segment : Segments)
	{
		if (Segment.Equals(GroupName, ESearchCase::IgnoreCase))
		{
			return true;
		}
	}
#endif

	for (const AActor* Parent = Cloner->GetAttachParentActor(); Parent; Parent = Parent->GetAttachParentActor())
	{
		if (Parent->ActorHasTag(GroupTag)
			|| ClonerBatchControllerPrivate::NameMatchesGroup(Parent->GetActorLabel(), GroupName))
		{
			return true;
		}
	}
	return false;
}

void AClonerBatchController::DiscoverCloners()
{
	FrontLineCloners.Reset();
	BackLineCloners.Reset();

	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<ACEClonerActor> It(World); It; ++It)
		{
			ACEClonerActor* Cloner = *It;
			const bool bFront = IsInNamedGroup(Cloner, TEXT("FrontLine"));
			const bool bBack = IsInNamedGroup(Cloner, TEXT("BackLine"));
			if (bFront && !bBack)
			{
				FrontLineCloners.AddUnique(Cloner);
			}
			else if (bBack && !bFront)
			{
				BackLineCloners.AddUnique(Cloner);
			}
		}
	}

	Modify();
	TargetClonerCount = FrontLineCloners.Num() + BackLineCloners.Num();
	LastStatus = FString::Printf(
		TEXT("Discovered %d cloners (FrontLine: %d, BackLine: %d). No clone counts were changed."),
		TargetClonerCount, FrontLineCloners.Num(), BackLineCloners.Num());
	UE_LOG(LogClonerBatchController, Log, TEXT("%s"), *LastStatus);
	ValidateTargetCounts();
}

void AClonerBatchController::GetUniqueTargets(TArray<ACEClonerActor*>& OutTargets) const
{
	OutTargets.Reset();
	TSet<ACEClonerActor*> Seen;
	auto Append = [&OutTargets, &Seen](const TArray<TObjectPtr<ACEClonerActor>>& Source)
	{
		for (ACEClonerActor* Cloner : Source)
		{
			if (IsValid(Cloner) && !Seen.Contains(Cloner))
			{
				Seen.Add(Cloner);
				OutTargets.Add(Cloner);
			}
		}
	};
	Append(FrontLineCloners);
	Append(BackLineCloners);
}

bool AClonerBatchController::SetSupportedLayoutCount(ACEClonerActor* Cloner, int32 NewCount)
{
	if (!IsValid(Cloner))
	{
		return false;
	}
	UCEClonerLayoutBase* Layout = Cloner->GetActiveLayout();
	if (!IsValid(Layout))
	{
		return false;
	}

	const bool bSet =
		ClonerBatchControllerPrivate::SetCountIfLayout<UCEClonerLineLayout>(Layout, NewCount)
		|| ClonerBatchControllerPrivate::SetCountIfLayout<UCEClonerCircleLayout>(Layout, NewCount)
		|| ClonerBatchControllerPrivate::SetCountIfLayout<UCEClonerMeshLayout>(Layout, NewCount)
		|| ClonerBatchControllerPrivate::SetCountIfLayout<UCEClonerSplineLayout>(Layout, NewCount)
		|| ClonerBatchControllerPrivate::SetCountIfLayout<UCEClonerSphereUniformLayout>(Layout, NewCount)
		|| ClonerBatchControllerPrivate::SetCountIfLayout<UCEClonerSphereRandomLayout>(Layout, NewCount);
	if (bSet)
	{
		Cloner->Modify();
		Cloner->ForceUpdateCloner();
		Cloner->MarkPackageDirty();
	}
	return bSet;
}

bool AClonerBatchController::GetSupportedLayoutCount(const ACEClonerActor* Cloner, int32& OutCount)
{
	if (!IsValid(Cloner))
	{
		return false;
	}
	const UCEClonerLayoutBase* Layout = Cloner->GetActiveLayout();
	return IsValid(Layout) && (
		ClonerBatchControllerPrivate::GetCountIfLayout<UCEClonerLineLayout>(Layout, OutCount)
		|| ClonerBatchControllerPrivate::GetCountIfLayout<UCEClonerCircleLayout>(Layout, OutCount)
		|| ClonerBatchControllerPrivate::GetCountIfLayout<UCEClonerMeshLayout>(Layout, OutCount)
		|| ClonerBatchControllerPrivate::GetCountIfLayout<UCEClonerSplineLayout>(Layout, OutCount)
		|| ClonerBatchControllerPrivate::GetCountIfLayout<UCEClonerSphereUniformLayout>(Layout, OutCount)
		|| ClonerBatchControllerPrivate::GetCountIfLayout<UCEClonerSphereRandomLayout>(Layout, OutCount));
}

void AClonerBatchController::ApplyCloneCount()
{
	GlobalCloneCount = FMath::Max(1, GlobalCloneCount);
	TArray<ACEClonerActor*> Targets;
	GetUniqueTargets(Targets);
	int32 AppliedCount = 0;
	UnsupportedClonerCount = 0;

#if WITH_EDITOR
	const FScopedTransaction Transaction(NSLOCTEXT("ClonerBatchController", "ApplyCloneCount", "Apply Cloner Batch Count"));
#endif
	Modify();
	for (ACEClonerActor* Cloner : Targets)
	{
		if (SetSupportedLayoutCount(Cloner, GlobalCloneCount))
		{
			++AppliedCount;
		}
		else
		{
			++UnsupportedClonerCount;
		}
	}

	TargetClonerCount = Targets.Num();
	MatchingClonerCount = AppliedCount;
	LastStatus = FString::Printf(TEXT("Applied count %d to %d/%d cloners. Unsupported layouts: %d."),
		GlobalCloneCount, AppliedCount, TargetClonerCount, UnsupportedClonerCount);
	MarkPackageDirty();
	UE_LOG(LogClonerBatchController, Log, TEXT("%s"), *LastStatus);
}

void AClonerBatchController::ValidateTargetCounts()
{
	TArray<ACEClonerActor*> Targets;
	GetUniqueTargets(Targets);
	TargetClonerCount = Targets.Num();
	MatchingClonerCount = 0;
	UnsupportedClonerCount = 0;
	for (const ACEClonerActor* Cloner : Targets)
	{
		int32 CurrentCount = 0;
		if (!GetSupportedLayoutCount(Cloner, CurrentCount))
		{
			++UnsupportedClonerCount;
		}
		else if (CurrentCount == GlobalCloneCount)
		{
			++MatchingClonerCount;
		}
	}
	LastStatus = FString::Printf(TEXT("Validation: %d/%d match count %d. Unsupported layouts: %d."),
		MatchingClonerCount, TargetClonerCount, GlobalCloneCount, UnsupportedClonerCount);
	UE_LOG(LogClonerBatchController, Log, TEXT("%s"), *LastStatus);
}
