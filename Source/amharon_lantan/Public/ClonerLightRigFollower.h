#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ClonerLightRigFollower.generated.h"

class UPointLightComponent;

/**
 * Base actor for a light-only rig that follows the transform delta of the
 * Blueprint variable named TargetCloner while preserving its initial offset.
 */
UCLASS(Blueprintable)
class AMHARON_LANTAN_API AClonerLightRigFollower : public AActor
{
	GENERATED_BODY()

public:
	AClonerLightRigFollower();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual bool ShouldTickIfViewportsOnly() const override;

	/** Allows the rig to be repositioned temporarily without removing it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Cloner Follow")
	bool bFollowTargetCloner = true;

	/** Optional C++ fallback. The existing BP TargetCloner variable takes priority. */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category="Cloner Follow")
	TObjectPtr<AActor> FollowTargetOverride = nullptr;

	/** Re-captures the current offset on the next editor or runtime tick. */
	UFUNCTION(CallInEditor, BlueprintCallable, Category="Cloner Follow")
	void RecaptureFollowOffset();

	/** Builds a line of point lights from the Blueprint's template light. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Light Cloner")
	bool bEnableLightCloner = true;

	/** Total visible lights, including the template light at index zero. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Light Cloner", meta=(ClampMin="1", UIMin="1", UIMax="100"))
	int32 LightCloneCount = 20;

	/** Distance between adjacent lights in centimetres. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Light Cloner", meta=(ClampMin="0.0", UIMin="0.0"))
	float LightCloneSpacing = 75.0f;

	/** Local-space line direction. The default creates a line along positive Z. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Light Cloner")
	FVector LightCloneAxis = FVector::UpVector;

	/** Existing Blueprint component used as the visual template and first light. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Light Cloner")
	FName TemplateLightComponentName = TEXT("PointLight_Front_01Top");

	/** Enables independent, smoothly interpolated intensity flicker per light. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Light Flicker")
	bool bEnableLightFlicker = true;

	/** Absolute minimum intensity used by the flicker. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Light Flicker", meta=(ClampMin="0.0", UIMin="0.0"))
	float FlickerMinimumIntensity = 0.0f;

	/** Absolute maximum intensity used by the flicker. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Light Flicker", meta=(ClampMin="0.0", UIMin="0.0"))
	float FlickerMaximumIntensity = 5.0f;

	/** Minimum difference between consecutive random target intensities. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Light Flicker", meta=(ClampMin="0.0", UIMin="0.0"))
	float FlickerMinimumDifference = 2.0f;

	/** Seconds spent easing from one target intensity to the next. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Light Flicker", meta=(ClampMin="0.01", UIMin="0.01"))
	float FlickerTransitionDuration = 1.0f;

	/** Fixed seed keeps Movie Render Queue output repeatable between runs. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Light Flicker")
	int32 FlickerRandomSeed = 1337;

protected:
	virtual void BeginPlay() override;

private:
	TWeakObjectPtr<AActor> CachedTarget;
	FTransform RelativeToTarget = FTransform::Identity;
	FTransform LastAppliedWorldTransform = FTransform::Identity;
	FTransform LastTargetWorldTransform = FTransform::Identity;
	bool bHasCapturedOffset = false;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UPointLightComponent>> GeneratedLightClones;

	struct FLightFlickerState
	{
		TWeakObjectPtr<UPointLightComponent> Light;
		FRandomStream RandomStream;
		float StartIntensity = 0.0f;
		float TargetIntensity = 0.0f;
		float ElapsedTime = 0.0f;
	};

	TArray<FLightFlickerState> LightFlickerStates;

	AActor* ResolveTargetCloner() const;
	void UpdateFollowTransform();
	void RebuildLightClones();
	void InitializeLightFlicker(UPointLightComponent* TemplateLight);
	void UpdateLightFlicker(float DeltaSeconds);
	float ChooseNextFlickerTarget(float CurrentIntensity, FRandomStream& RandomStream) const;
};
