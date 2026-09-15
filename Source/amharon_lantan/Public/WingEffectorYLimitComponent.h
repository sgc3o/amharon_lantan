#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WingEffectorYLimitComponent.generated.h"

class ACEEffectorActor;

UENUM(BlueprintType)
enum class EWingEffectorMotionMode : uint8
{
	Default UMETA(DisplayName="Default"),
	NoiseField UMETA(DisplayName="Noise Field")
};

/**
 * Drives the Effector orientation force with zero-mean angular velocity waves.
 * Niagara gives every clone a deterministic random blend of the two waves, so
 * motion stays smooth and individual while its integrated angle stays bounded.
 */
UCLASS(ClassGroup=(Lantern), BlueprintType, Blueprintable,
	meta=(BlueprintSpawnableComponent, DisplayName="Wing Effector Y Rotation Limit"))
class AMHARON_LANTAN_API UWingEffectorYLimitComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWingEffectorYLimitComponent();

	/** Adds one persistent limiter to an actor, or returns its existing limiter. */
	UFUNCTION(BlueprintCallable, Category="Wing Effector|Y Limit",
		meta=(DisplayName="Add Wing Effector Y Limit"))
	static UWingEffectorYLimitComponent* AddToActor(AActor* TargetActor);

	virtual void OnRegister() override;
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Enables or disables the bounded Y-axis rotation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Wing Effector|Y Rotation Limit")
	bool bLimitYRotation = true;

	/** Effector transform mode used together with the bounded orientation force. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Wing Effector|Motion Mode",
		meta=(DisplayName="Effector Mode"))
	EWingEffectorMotionMode EffectorMode = EWingEffectorMotionMode::Default;

	/** Lowest Pitch contribution produced by this effector. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Interp, Category="Wing Effector|Y Rotation Limit",
		meta=(DisplayName="Min Y Rotation", Units="deg", ClampMin="-180.0", ClampMax="180.0", UIMin="-30.0", UIMax="0.0"))
	float MinYRotation = -5.0f;

	/** Highest Pitch contribution produced by this effector. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Interp, Category="Wing Effector|Y Rotation Limit",
		meta=(DisplayName="Max Y Rotation", Units="deg", ClampMin="-180.0", ClampMax="180.0", UIMin="0.0", UIMax="30.0"))
	float MaxYRotation = 5.0f;

	/** Number of complete left-right-left sway cycles per second. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Interp, Category="Wing Effector|Y Rotation Limit",
		meta=(DisplayName="Sway Frequency", Units="Hz", ClampMin="0.001", ClampMax="10.0", UIMin="0.05", UIMax="2.0"))
	float SwayFrequency = 0.2f;

	/** Restarts the bounded sway from the center angle. */
	UFUNCTION(CallInEditor, BlueprintCallable, Category="Wing Effector|Y Rotation Limit",
		meta=(DisplayName="Restart Sway From Center"))
	void RestartSwayFromCenter();

	/** Current Pitch value sent to the Effector Noise Field. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Wing Effector|Y Rotation Limit",
		meta=(DisplayName="Current Y Rotation", Units="deg"))
	float CurrentYRotation = 0.0f;

	/** Enables a smaller bounded orientation-force rotation around Unreal's Z axis (Yaw). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Wing Effector|Z Rotation Limit")
	bool bEnableZSway = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Interp, Category="Wing Effector|Z Rotation Limit",
		meta=(DisplayName="Min Z Rotation", Units="deg", ClampMin="-180.0", ClampMax="180.0", UIMin="-15.0", UIMax="0.0"))
	float MinZRotation = -1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Interp, Category="Wing Effector|Z Rotation Limit",
		meta=(DisplayName="Max Z Rotation", Units="deg", ClampMin="-180.0", ClampMax="180.0", UIMin="0.0", UIMax="15.0"))
	float MaxZRotation = 1.5f;

	/** Base speed of the Z orientation-force sway. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Interp, Category="Wing Effector|Z Rotation Limit",
		meta=(DisplayName="Z Sway Frequency", Units="Hz", ClampMin="0.001", ClampMax="10.0", UIMin="0.02", UIMax="1.0"))
	float ZSwayFrequency = 0.1f;

	/** Controls the frequency difference between the two per-lantern force curves. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Interp, Category="Wing Effector|Z Rotation Limit",
		meta=(DisplayName="Randomness", ClampMin="0.0", ClampMax="1.0", UIMin="0.0", UIMax="1.0"))
	float ZRandomness = 0.7f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Wing Effector|Z Rotation Limit",
		meta=(DisplayName="Current Z Rotation", Units="deg"))
	float CurrentZRotation = 0.0f;

private:
	void ApplyBoundedSway(float DeltaTime);
	void ResetClonerOrientations(ACEEffectorActor* Effector);
	bool HasForceConfigurationChanged() const;
	void RememberForceConfiguration();

	float SwayElapsedSeconds = 0.0f;
	bool bOrientationForceInitialized = false;
	float LastMinYRotation = 0.0f;
	float LastMaxYRotation = 0.0f;
	float LastSwayFrequency = 0.0f;
	bool bLastEnableZSway = false;
	float LastMinZRotation = 0.0f;
	float LastMaxZRotation = 0.0f;
	float LastZSwayFrequency = 0.0f;
	float LastZRandomness = 0.0f;
};
