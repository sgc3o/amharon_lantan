#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ClonerLanternBakeTool.generated.h"

class ACEClonerActor;
class USceneComponent;
class ULevelSequence;

USTRUCT(BlueprintType)
struct FLanternFlightEntry
{
	GENERATED_BODY()

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly)
	TObjectPtr<AActor> Lantern;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly)
	FTransform StartRelativeTransform;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly)
	FVector TargetRelativeLocation = FVector::ZeroVector;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly)
	FVector DriftDirection = FVector::RightVector;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly)
	float DriftAmplitude = 0.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly)
	float DriftCycles = 1.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly)
	float DriftPhase = 0.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly)
	float RotationPhase = 0.0f;
};

/**
 * Parent created for one baked Cloner snapshot. Moving this actor moves all
 * generated lantern Blueprint actors while keeping every lantern independent.
 */
UCLASS(BlueprintType)
class AMHARON_LANTAN_API ALanternBakeGroup : public AActor
{
	GENERATED_BODY()

public:
	ALanternBakeGroup();
	virtual void Tick(float DeltaSeconds) override;
	virtual bool ShouldTickIfViewportsOnly() const override { return true; }

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Cloner Bake")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Cloner Bake")
	TObjectPtr<ACEClonerActor> SourceCloner;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Cloner Bake")
	TArray<TObjectPtr<AActor>> BakedLanterns;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Cloner Bake")
	int32 BakedLanternCount = 0;

	/** Sequencer drives this from 0 to 1. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Interp, Category="Lantern Flight", meta=(ClampMin="0.0", ClampMax="1.0"))
	float FlightProgress = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lantern Flight")
	int32 FlightSeed = 5833;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lantern Flight|Destination")
	FVector2D TargetXRange = FVector2D(-400.0, 400.0);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lantern Flight|Destination")
	FVector2D TargetYRange = FVector2D(-1000.0, -850.0);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lantern Flight|Destination")
	FVector2D TargetZLowRange = FVector2D(200.0, 250.0);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lantern Flight|Destination")
	FVector2D TargetZHighRange = FVector2D(250.0, 400.0);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lantern Flight|Destination", meta=(ClampMin="0.0", ClampMax="1.0"))
	float TargetZLowRatio = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lantern Flight|Motion", meta=(ClampMin="0.001", ClampMax="0.95"))
	float EaseInRatio = 0.12f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lantern Flight|Motion")
	FVector2D DriftAmplitudeRange = FVector2D(10.0, 35.0);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lantern Flight|Motion")
	FVector2D DriftCyclesRange = FVector2D(0.75, 2.0);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lantern Flight|Motion", meta=(ClampMin="0.0"))
	float RotationSwayDegrees = 6.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Lantern Flight")
	TArray<FLanternFlightEntry> FlightEntries;

	/** Rebuilds deterministic destinations from the lanterns' current positions. */
	UFUNCTION(CallInEditor, BlueprintCallable, Category="Lantern Flight")
	void InitializeFlight();

	/** Deletes only the actors created by this bake. The operation supports Undo. */
	UFUNCTION(CallInEditor, BlueprintCallable, Category="Cloner Bake")
	void DeleteThisBake();

private:
	void ApplyFlight();
	float LastAppliedProgress = -1.0f;
};

/**
 * Reusable UE 5.4 editor tool which snapshots a Motion Design Cloner into
 * individual Blueprint actors. Put one tool in the level and change
 * Target Cloner whenever another Cloner needs to be baked.
 */
UCLASS(BlueprintType)
class AMHARON_LANTAN_API AClonerLanternBakeTool : public AActor
{
	GENERATED_BODY()

public:
	AClonerLanternBakeTool();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Cloner Bake")
	TObjectPtr<USceneComponent> SceneRoot;

	/** Motion Design Cloner to snapshot. Use the eyedropper in the Details panel. */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category="Cloner Bake", meta=(DisplayName="Target Cloner"))
	TObjectPtr<ACEClonerActor> TargetCloner;

	/** Hides the source Cloner after a successful bake, but never deletes it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Cloner Bake")
	bool bHideSourceClonerAfterBake = true;

	/** Attach generated lanterns to a movable group actor. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Cloner Bake")
	bool bCreateMovableBakeGroup = true;

	/** Folder used in the World Outliner for all generated groups and lanterns. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Cloner Bake")
	FName OutputFolderRoot = TEXT("ClonerBakes");

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Cloner Bake")
	TObjectPtr<ALanternBakeGroup> LastBakeGroup;

	/** Sequence which receives the automatic 5833F -> 9000F flight track. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lantern Flight")
	TObjectPtr<ULevelSequence> TargetSequence;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lantern Flight")
	bool bCreateFlightProgressTrack = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lantern Flight")
	int32 FlightStartFrame = 5833;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lantern Flight")
	int32 FlightEndFrame = 9000;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Cloner Bake", meta=(MultiLine="true"))
	FString LastResult = TEXT("Target Clonerを指定して Bake Target Cloner を押してください。");

	/** Captures the currently displayed clone transforms and creates individual BP actors. */
	UFUNCTION(CallInEditor, BlueprintCallable, Category="Cloner Bake", meta=(DisplayName="Bake Target Cloner"))
	void BakeTargetCloner();

	/** Shows the target Cloner again without changing the baked actors. */
	UFUNCTION(CallInEditor, BlueprintCallable, Category="Cloner Bake")
	void ShowSourceCloner();

	/** Deletes only the most recently created bake. The operation supports Undo. */
	UFUNCTION(CallInEditor, BlueprintCallable, Category="Cloner Bake")
	void DeleteLastBake();

private:
	bool AddFlightTrack(ALanternBakeGroup* Group);
};
