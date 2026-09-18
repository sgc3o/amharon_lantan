#include "ClonerLightRigFollower.h"

#include "Components/PointLightComponent.h"
#include "UObject/UnrealType.h"

namespace
{
	const FName GeneratedLightTag(TEXT("GeneratedByLightCloner"));
}

AClonerLightRigFollower::AClonerLightRigFollower()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.bTickEvenWhenPaused = true;
}

void AClonerLightRigFollower::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RebuildLightClones();
}

void AClonerLightRigFollower::BeginPlay()
{
	Super::BeginPlay();
	RecaptureFollowOffset();
	UpdateFollowTransform();
}

void AClonerLightRigFollower::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateFollowTransform();
	UpdateLightFlicker(DeltaSeconds);
}

bool AClonerLightRigFollower::ShouldTickIfViewportsOnly() const
{
	return true;
}

void AClonerLightRigFollower::RecaptureFollowOffset()
{
	bHasCapturedOffset = false;
	CachedTarget.Reset();
}

void AClonerLightRigFollower::RebuildLightClones()
{
	LightFlickerStates.Reset();
	TInlineComponentArray<UPointLightComponent*> PointLights(this);

	// Normally components created as UserConstructionScript components are removed
	// before the next construction pass. Explicit cleanup also covers manual calls.
	for (UPointLightComponent* Light : PointLights)
	{
		if (IsValid(Light) && Light->ComponentHasTag(GeneratedLightTag))
		{
			Light->DestroyComponent();
		}
	}
	GeneratedLightClones.Reset();

	if (!bEnableLightCloner)
	{
		return;
	}

	PointLights.Reset();
	GetComponents(PointLights);

	TArray<UPointLightComponent*> TemplateCandidates;
	for (UPointLightComponent* Light : PointLights)
	{
		if (IsValid(Light) && !Light->ComponentHasTag(GeneratedLightTag))
		{
			TemplateCandidates.Add(Light);
		}
	}

	TemplateCandidates.Sort([](const UPointLightComponent& Left, const UPointLightComponent& Right)
	{
		return Left.GetName() < Right.GetName();
	});

	UPointLightComponent* TemplateLight = nullptr;
	for (UPointLightComponent* Candidate : TemplateCandidates)
	{
		if (Candidate->GetFName() == TemplateLightComponentName)
		{
			TemplateLight = Candidate;
			break;
		}
	}
	if (TemplateLight == nullptr && TemplateCandidates.Num() > 0)
	{
		TemplateLight = TemplateCandidates[0];
	}
	if (TemplateLight == nullptr)
	{
		return;
	}

	// Older versions of BP_ClonerLightRig contain several hand-placed lights.
	// Keep one as the template and make all legacy extras non-rendering.
	for (UPointLightComponent* Candidate : TemplateCandidates)
	{
		if (Candidate != TemplateLight)
		{
			Candidate->SetVisibility(false, true);
			Candidate->SetHiddenInGame(true);
		}
	}
	TemplateLight->SetVisibility(true, true);
	TemplateLight->SetHiddenInGame(false);

	const int32 SafeCloneCount = FMath::Max(1, LightCloneCount);
	const FVector SafeAxis = LightCloneAxis.IsNearlyZero() ? FVector::UpVector : LightCloneAxis.GetSafeNormal();
	const FTransform TemplateTransform = TemplateLight->GetRelativeTransform();
	USceneComponent* AttachParent = TemplateLight->GetAttachParent();
	if (AttachParent == nullptr)
	{
		AttachParent = GetRootComponent();
	}

	for (int32 Index = 1; Index < SafeCloneCount; ++Index)
	{
		const FName ComponentName(*FString::Printf(TEXT("ClonedPointLight_%02d"), Index + 1));
		UPointLightComponent* Clone = DuplicateObject<UPointLightComponent>(TemplateLight, this, ComponentName);
		if (Clone == nullptr)
		{
			continue;
		}

		Clone->CreationMethod = EComponentCreationMethod::UserConstructionScript;
		Clone->ComponentTags.AddUnique(GeneratedLightTag);
		Clone->SetupAttachment(AttachParent, TemplateLight->GetAttachSocketName());

		FTransform CloneTransform = TemplateTransform;
		CloneTransform.SetLocation(
			TemplateTransform.GetLocation() + SafeAxis * LightCloneSpacing * static_cast<float>(Index));
		Clone->SetRelativeTransform(CloneTransform);

		AddOwnedComponent(Clone);
		Clone->RegisterComponent();
		GeneratedLightClones.Add(Clone);
	}

	InitializeLightFlicker(TemplateLight);
}

void AClonerLightRigFollower::InitializeLightFlicker(UPointLightComponent* TemplateLight)
{
	LightFlickerStates.Reset();

	TArray<UPointLightComponent*> ActiveLights;
	if (IsValid(TemplateLight))
	{
		ActiveLights.Add(TemplateLight);
	}
	for (UPointLightComponent* Clone : GeneratedLightClones)
	{
		if (IsValid(Clone))
		{
			ActiveLights.Add(Clone);
		}
	}

	const float MinimumIntensity = FMath::Min(FlickerMinimumIntensity, FlickerMaximumIntensity);
	const float MaximumIntensity = FMath::Max(FlickerMinimumIntensity, FlickerMaximumIntensity);
	if (!bEnableLightFlicker)
	{
		for (UPointLightComponent* Light : ActiveLights)
		{
			Light->SetIntensity(MaximumIntensity);
		}
		return;
	}

	const float Duration = FMath::Max(0.01f, FlickerTransitionDuration);
	LightFlickerStates.Reserve(ActiveLights.Num());
	for (int32 Index = 0; Index < ActiveLights.Num(); ++Index)
	{
		FLightFlickerState& State = LightFlickerStates.AddDefaulted_GetRef();
		State.Light = ActiveLights[Index];
		State.RandomStream.Initialize(FlickerRandomSeed + Index * 7919);
		State.StartIntensity = MaximumIntensity;
		State.TargetIntensity = ChooseNextFlickerTarget(State.StartIntensity, State.RandomStream);

		// Every transition still lasts exactly one Duration, but a random initial
		// phase prevents all lights from reaching their targets simultaneously.
		State.ElapsedTime = State.RandomStream.FRandRange(0.0f, Duration);
		const float LinearAlpha = FMath::Clamp(State.ElapsedTime / Duration, 0.0f, 1.0f);
		const float SmoothAlpha = LinearAlpha * LinearAlpha * (3.0f - 2.0f * LinearAlpha);
		ActiveLights[Index]->SetIntensity(
			FMath::Lerp(State.StartIntensity, State.TargetIntensity, SmoothAlpha));
	}
}

void AClonerLightRigFollower::UpdateLightFlicker(float DeltaSeconds)
{
	if (!bEnableLightFlicker || LightFlickerStates.IsEmpty())
	{
		return;
	}

	const float Duration = FMath::Max(0.01f, FlickerTransitionDuration);
	for (FLightFlickerState& State : LightFlickerStates)
	{
		UPointLightComponent* Light = State.Light.Get();
		if (!IsValid(Light))
		{
			continue;
		}

		State.ElapsedTime += FMath::Max(0.0f, DeltaSeconds);
		while (State.ElapsedTime >= Duration)
		{
			State.ElapsedTime -= Duration;
			State.StartIntensity = State.TargetIntensity;
			State.TargetIntensity = ChooseNextFlickerTarget(
				State.StartIntensity, State.RandomStream);
		}

		const float LinearAlpha = FMath::Clamp(State.ElapsedTime / Duration, 0.0f, 1.0f);
		const float SmoothAlpha = LinearAlpha * LinearAlpha * (3.0f - 2.0f * LinearAlpha);
		Light->SetIntensity(FMath::Lerp(State.StartIntensity, State.TargetIntensity, SmoothAlpha));
	}
}

float AClonerLightRigFollower::ChooseNextFlickerTarget(
	float CurrentIntensity, FRandomStream& RandomStream) const
{
	const float MinimumIntensity = FMath::Min(FlickerMinimumIntensity, FlickerMaximumIntensity);
	const float MaximumIntensity = FMath::Max(FlickerMinimumIntensity, FlickerMaximumIntensity);
	const float IntensityRange = MaximumIntensity - MinimumIntensity;
	if (IntensityRange <= KINDA_SMALL_NUMBER)
	{
		return MinimumIntensity;
	}

	const float ClampedCurrent = FMath::Clamp(CurrentIntensity, MinimumIntensity, MaximumIntensity);
	const float RequiredDifference = FMath::Clamp(FlickerMinimumDifference, 0.0f, IntensityRange);
	const float LowerMaximum = ClampedCurrent - RequiredDifference;
	const float UpperMinimum = ClampedCurrent + RequiredDifference;
	const bool bCanChooseLower = LowerMaximum >= MinimumIntensity;
	const bool bCanChooseUpper = UpperMinimum <= MaximumIntensity;

	if (bCanChooseLower && bCanChooseUpper)
	{
		return RandomStream.RandRange(0, 1) == 0
			? RandomStream.FRandRange(MinimumIntensity, LowerMaximum)
			: RandomStream.FRandRange(UpperMinimum, MaximumIntensity);
	}
	if (bCanChooseLower)
	{
		return RandomStream.FRandRange(MinimumIntensity, LowerMaximum);
	}
	if (bCanChooseUpper)
	{
		return RandomStream.FRandRange(UpperMinimum, MaximumIntensity);
	}

	// A user can set a difference that is temporarily impossible from a custom
	// middle value. Move to the farther endpoint; subsequent targets can then
	// satisfy the configured difference exactly.
	return (ClampedCurrent - MinimumIntensity) >= (MaximumIntensity - ClampedCurrent)
		? MinimumIntensity
		: MaximumIntensity;
}

AActor* AClonerLightRigFollower::ResolveTargetCloner() const
{
	// Keep using the TargetCloner variable that already exists on the Blueprint.
	if (const FObjectPropertyBase* TargetProperty = FindFProperty<FObjectPropertyBase>(GetClass(), TEXT("TargetCloner")))
	{
		if (AActor* BlueprintTarget = Cast<AActor>(TargetProperty->GetObjectPropertyValue_InContainer(this)))
		{
			return BlueprintTarget;
		}
	}

	return FollowTargetOverride.Get();
}

void AClonerLightRigFollower::UpdateFollowTransform()
{
	if (!bFollowTargetCloner)
	{
		return;
	}

	AActor* Target = ResolveTargetCloner();
	if (!IsValid(Target) || Target == this)
	{
		return;
	}

	const FTransform TargetTransform = Target->GetActorTransform();
	const FTransform CurrentTransform = GetActorTransform();

	if (!bHasCapturedOffset || CachedTarget.Get() != Target)
	{
		RelativeToTarget = CurrentTransform.GetRelativeTransform(TargetTransform);
		CachedTarget = Target;
		bHasCapturedOffset = true;
	}
	else
	{
		// If the user moves the light rig while its target stayed still, accept the
		// new placement as the intended offset instead of snapping it back.
		const bool bRigEdited = !CurrentTransform.Equals(LastAppliedWorldTransform, 0.01f);
		const bool bTargetStayedStill = TargetTransform.Equals(LastTargetWorldTransform, 0.01f);
		if (bRigEdited && bTargetStayedStill)
		{
			RelativeToTarget = CurrentTransform.GetRelativeTransform(TargetTransform);
		}
	}

	const FTransform DesiredTransform = RelativeToTarget * TargetTransform;
	if (!CurrentTransform.Equals(DesiredTransform, 0.01f))
	{
		SetActorTransform(DesiredTransform, false, nullptr, ETeleportType::TeleportPhysics);
	}

	LastTargetWorldTransform = TargetTransform;
	LastAppliedWorldTransform = GetActorTransform();
}
