#include "TransitionController.h"

#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"

ATransitionController::ATransitionController()
{
	PrimaryActorTick.bCanEverTick = true;
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
}

void ATransitionController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!FMath::IsNearlyEqual(Progress, LastAppliedProgress))
	{
		ApplyTransition();
	}
}

bool ATransitionController::ShouldTickIfViewportsOnly() const
{
	return true;
}

#if WITH_EDITOR
void ATransitionController::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	const FName PropertyName = PropertyChangedEvent.GetPropertyName();
	const FName MemberPropertyName = PropertyChangedEvent.GetMemberPropertyName();
	if (PropertyName == GET_MEMBER_NAME_CHECKED(ATransitionController, Progress))
	{
		ApplyTransition();
	}
	else if (MemberPropertyName == GET_MEMBER_NAME_CHECKED(ATransitionController, TransitionActors)
		|| PropertyName == GET_MEMBER_NAME_CHECKED(ATransitionController, TransitionActors))
	{
		CaptureFinalTransforms();
	}
}
#endif

void ATransitionController::SetProgress(float InProgress)
{
	Progress = FMath::Clamp(InProgress, 0.0f, 1.0f);
	ApplyTransition();
}

void ATransitionController::CaptureFinalTransforms()
{
	FinalTransforms.Reset(TransitionActors.Num());
	for (AActor* Actor : TransitionActors)
	{
		FinalTransforms.Add(IsValid(Actor) ? Actor->GetActorTransform() : FTransform::Identity);
	}
	Progress = 1.0f;
	LastAppliedProgress = 1.0f;
	Modify();
}

void ATransitionController::RestoreFinalTransforms()
{
	const int32 Count = FMath::Min(TransitionActors.Num(), FinalTransforms.Num());
	for (int32 Index = 0; Index < Count; ++Index)
	{
		if (AActor* Actor = TransitionActors[Index])
		{
			Actor->SetActorTransform(FinalTransforms[Index], false, nullptr, ETeleportType::TeleportPhysics);
		}
	}
	Progress = 1.0f;
	LastAppliedProgress = 1.0f;
}

FVector ATransitionController::ResolveCommonStartLocation() const
{
	if (!IsValid(TransitionCamera))
	{
		return GetActorLocation();
	}

	const FVector CameraLocation = TransitionCamera->GetActorLocation();
	const FVector Forward = TransitionCamera->GetActorForwardVector();
	const FVector Up = TransitionCamera->GetActorUpVector();
	float Depth = StartDepth;
	if (Depth <= KINDA_SMALL_NUMBER)
	{
		float DepthSum = 0.0f;
		int32 ValidDepthCount = 0;
		for (const FTransform& Transform : FinalTransforms)
		{
			const float Candidate = FVector::DotProduct(Transform.GetLocation() - CameraLocation, Forward);
			if (Candidate > 1.0f)
			{
				DepthSum += Candidate;
				++ValidDepthCount;
			}
		}
		Depth = ValidDepthCount > 0 ? DepthSum / ValidDepthCount : 1000.0f;
	}

	const UCameraComponent* CameraComponent = TransitionCamera->GetCameraComponent();
	const float HorizontalHalfTangent = CameraComponent
		? FMath::Tan(FMath::DegreesToRadians(CameraComponent->FieldOfView * 0.5f))
		: FMath::Tan(FMath::DegreesToRadians(45.0f));
	const float AspectRatio = CameraComponent ? FMath::Max(0.01f, CameraComponent->AspectRatio) : (9.0f / 16.0f);
	const float VerticalHalfExtent = Depth * HorizontalHalfTangent / AspectRatio;
	return CameraLocation + Forward * Depth - Up * VerticalHalfExtent * ScreenBottomFraction;
}

void ATransitionController::ApplyTransition()
{
	if (TransitionActors.Num() == 0 || FinalTransforms.Num() != TransitionActors.Num())
	{
		return;
	}

	const int32 Count = TransitionActors.Num();
	const float SafeTotalDuration = FMath::Max(0.01f, TotalDuration);
	const float SafeMoveDuration = FMath::Clamp(MoveDuration, 0.01f, SafeTotalDuration);
	const float Stagger = Count > 1 ? (SafeTotalDuration - SafeMoveDuration) / static_cast<float>(Count - 1) : 0.0f;
	const float CurrentTime = FMath::Clamp(Progress, 0.0f, 1.0f) * SafeTotalDuration;
	const FVector StartLocation = ResolveCommonStartLocation();

	for (int32 Index = 0; Index < Count; ++Index)
	{
		AActor* Actor = TransitionActors[Index];
		if (!IsValid(Actor))
		{
			continue;
		}

		const float LocalTime = CurrentTime - Stagger * static_cast<float>(Index);
		const float Alpha = FMath::Clamp(LocalTime / SafeMoveDuration, 0.0f, 1.0f);
		const float EasedAlpha = 1.0f - FMath::Pow(1.0f - Alpha, FMath::Max(0.1f, EasingExponent));
		const FTransform& EndTransform = FinalTransforms[Index];
		const FVector EndScale = EndTransform.GetScale3D();
		const FVector Scale = FMath::Lerp(EndScale * StartScale, EndScale, EasedAlpha);
		const FQuat StartRotation = (EndTransform.Rotator() + StartRotationOffset).Quaternion();
		const FQuat Rotation = FQuat::Slerp(StartRotation, EndTransform.GetRotation(), EasedAlpha);
		const FVector Location = FMath::Lerp(StartLocation, EndTransform.GetLocation(), EasedAlpha);

		Actor->SetActorTransform(FTransform(Rotation, Location, Scale), false, nullptr, ETeleportType::TeleportPhysics);
	}

	LastAppliedProgress = Progress;
}
