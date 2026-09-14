#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LanternShowController.generated.h"

class USceneComponent;
class ACEClonerActor;
class ACEEffectorActor;

/** Named sections are diagnostic only in STEP 1. */
UENUM(BlueprintType)
enum class ELanternShowSection : uint8
{
	BeforeShow,
	Intro,
	FirstMotion,
	Calm,
	Awakening,
	PreStillness,
	Stillness,
	PreFlight,
	Glow,
	Flight,
	AfterShow
};

/**
 * Sequencer-facing master clock for the lantern show.
 *
 * STEP 1 intentionally has no references to cloners, Niagara systems,
 * effectors, or materials. It only owns editable cue times and diagnostic
 * values derived deterministically from AnimationTime.
 */
UCLASS(BlueprintType, Blueprintable)
class AMHARON_LANTAN_API ALanternShowController : public AActor
{
	GENERATED_BODY()

public:
	ALanternShowController();

	virtual void Tick(float DeltaSeconds) override;
	virtual bool ShouldTickIfViewportsOnly() const override;
	virtual void OnConstruction(const FTransform& Transform) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Lantern Show")
	TObjectPtr<USceneComponent> SceneRoot;

	/** Master show clock in seconds. Keyframe this property in Sequencer. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Interp, BlueprintSetter=SetAnimationTime, BlueprintGetter=GetAnimationTime,
		Category="Lantern Show|1. Playback", meta=(ClampMin="0.0", ClampMax="320.0", UIMin="0.0", UIMax="320.0", Units="s"))
	float AnimationTime = 0.0f;

	UFUNCTION(BlueprintCallable, Category="Lantern Show|1. Playback")
	void SetAnimationTime(float InAnimationTime);

	UFUNCTION(BlueprintPure, Category="Lantern Show|1. Playback")
	float GetAnimationTime() const { return AnimationTime; }

	/** Manual STEP 2 sway weight. Future steps may derive this from AnimationTime. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Interp, BlueprintSetter=SetSwayAmount, BlueprintGetter=GetSwayAmount,
		Category="Lantern Show|1. Playback", meta=(ClampMin="0.0", ClampMax="1.0", UIMin="0.0", UIMax="1.0"))
	float SwayAmount = 0.0f;

	UFUNCTION(BlueprintCallable, Category="Lantern Show|1. Playback")
	void SetSwayAmount(float InSwayAmount);

	UFUNCTION(BlueprintPure, Category="Lantern Show|1. Playback")
	float GetSwayAmount() const { return SwayAmount; }

	/** Explicitly binds the STEP 2 effector to one cloner. Never discovers targets. */
	UFUNCTION(BlueprintCallable, Category="Lantern Show|4. STEP 2 Sway")
	bool ConfigureStep2Sway(ACEClonerActor* InCloner, ACEEffectorActor* InEffector);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lantern Show|2. Cue Times", meta=(Units="s"))
	float ShowStartTime = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lantern Show|2. Cue Times", meta=(Units="s"))
	float FirstMotionTime = 29.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lantern Show|2. Cue Times", meta=(Units="s"))
	float CalmTime = 65.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lantern Show|2. Cue Times", meta=(Units="s"))
	float AwakeningTime = 70.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lantern Show|2. Cue Times", meta=(Units="s"))
	float PreStillnessTime = 133.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lantern Show|2. Cue Times", meta=(Units="s"))
	float StillnessTime = 134.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lantern Show|2. Cue Times", meta=(Units="s"))
	float PreFlightTime = 137.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lantern Show|2. Cue Times", meta=(Units="s"))
	float GlowTime = 148.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lantern Show|2. Cue Times", meta=(Units="s"))
	float FlightTime = 150.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lantern Show|2. Cue Times", meta=(ClampMin="0.0", Units="s"))
	float ShowEndTime = 320.0f;

	/** Diagnostic values updated during Sequencer scrubbing and editor ticks. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Lantern Show|3. Debug")
	ELanternShowSection CurrentSection = ELanternShowSection::Intro;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Lantern Show|3. Debug", meta=(ClampMin="0.0", ClampMax="1.0"))
	float NormalizedShowTime = 0.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Lantern Show|3. Debug", meta=(Units="s"))
	float SecondsIntoCurrentSection = 0.0f;

	/** Explicit STEP 2 references. No actor is discovered or linked automatically. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Lantern Show|4. STEP 2 Sway")
	TObjectPtr<ACEClonerActor> SwayTestCloner;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Lantern Show|4. STEP 2 Sway")
	TObjectPtr<ACEEffectorActor> SwayEffector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lantern Show|4. STEP 2 Sway", meta=(ClampMin="0.0", ClampMax="10.0", Units="deg"))
	float YawAmplitude = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lantern Show|4. STEP 2 Sway", meta=(ClampMin="0.0", ClampMax="10.0", Units="deg"))
	float RollAmplitude = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lantern Show|4. STEP 2 Sway", meta=(ClampMin="0.01", ClampMax="2.0", Units="Hz"))
	float BaseSwayFrequency = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lantern Show|4. STEP 2 Sway", meta=(ClampMin="0.001", ClampMax="2.0"))
	float NoiseSpatialFrequency = 0.5f;

	/** Makes AnimationTime == 0 an exact no-op, then eases into the requested weight. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lantern Show|4. STEP 2 Sway", meta=(ClampMin="0.001", ClampMax="5.0", Units="s"))
	float SwayStartupBlendTime = 0.5f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Lantern Show|4. STEP 2 Sway")
	float EffectiveSwayAmount = 0.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Lantern Show|4. STEP 2 Sway")
	bool bSwayBindingValid = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Lantern Show|5. Future Outputs")
	float RotationAmount = 0.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Lantern Show|5. Future Outputs")
	float WindAmount = 0.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Lantern Show|5. Future Outputs")
	float EmissionAmount = 0.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Lantern Show|5. Future Outputs")
	float FloatProgress = 0.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Lantern Show|5. Future Outputs")
	float FlightProgress = 0.0f;

private:
	void RefreshDiagnostics();
	void ApplyStep2Sway();
};
