#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TransitionController.generated.h"

class USceneComponent;
class ACameraActor;

/** Holds the ordered actor list used by a short screen transition. */
UCLASS(BlueprintType, Blueprintable)
class AMHARON_LANTAN_API ATransitionController : public AActor
{
	GENERATED_BODY()

public:
	ATransitionController();
	virtual void Tick(float DeltaSeconds) override;
	virtual bool ShouldTickIfViewportsOnly() const override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Transition")
	TObjectPtr<USceneComponent> SceneRoot;

	/** Array order is appearance order: index 0 appears first. */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category="Transition", meta=(DisplayName="Transition Actors"))
	TArray<TObjectPtr<AActor>> TransitionActors;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category="Transition|Start", meta=(DisplayName="Transition Camera"))
	TObjectPtr<ACameraActor> TransitionCamera;

	/** 0 = hidden at the common start point, 1 = all actors at their captured positions. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Interp, BlueprintSetter=SetProgress, BlueprintGetter=GetProgress,
		Category="Transition|Playback", meta=(ClampMin="0.0", ClampMax="1.0", UIMin="0.0", UIMax="1.0"))
	float Progress = 1.0f;

	UFUNCTION(BlueprintCallable, Category="Transition|Playback")
	void SetProgress(float InProgress);

	UFUNCTION(BlueprintPure, Category="Transition|Playback")
	float GetProgress() const { return Progress; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Transition|Timing", meta=(ClampMin="0.01", Units="s"))
	float TotalDuration = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Transition|Timing", meta=(ClampMin="0.01", Units="s"))
	float MoveDuration = 0.65f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Transition|Timing", meta=(ClampMin="0.1"))
	float EasingExponent = 3.0f;

	/** Normalized distance from screen center toward the lower edge. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Transition|Start", meta=(ClampMin="0.0", ClampMax="1.0"))
	float ScreenBottomFraction = 0.82f;

	/** 0 automatically uses the average camera depth of the registered actors. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Transition|Start", meta=(ClampMin="0.0", Units="cm"))
	float StartDepth = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Transition|Start", meta=(ClampMin="0.0", ClampMax="1.0"))
	float StartScale = 0.05f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Transition|Start")
	FRotator StartRotationOffset = FRotator::ZeroRotator;

	UFUNCTION(CallInEditor, BlueprintCallable, Category="Transition|Setup", meta=(DisplayName="Capture Current Transforms As Final"))
	void CaptureFinalTransforms();

	UFUNCTION(CallInEditor, BlueprintCallable, Category="Transition|Setup", meta=(DisplayName="Apply Transition Preview"))
	void ApplyTransition();

	UFUNCTION(CallInEditor, BlueprintCallable, Category="Transition|Setup", meta=(DisplayName="Restore Final Transforms"))
	void RestoreFinalTransforms();

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Transition|Setup")
	TArray<FTransform> FinalTransforms;

private:
	FVector ResolveCommonStartLocation() const;
	float LastAppliedProgress = -1.0f;
};
