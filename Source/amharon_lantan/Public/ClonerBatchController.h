#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ClonerBatchController.generated.h"

class ACEClonerActor;
class USceneComponent;

/**
 * Central, explicit count control for Motion Design Cloners.
 * Nothing is changed during construction or property editing; ApplyCloneCount is
 * the only operation that mutates the target cloners.
 */
UCLASS(BlueprintType, Blueprintable)
class AMHARON_LANTAN_API AClonerBatchController : public AActor
{
	GENERATED_BODY()

public:
	AClonerBatchController();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Cloner Batch")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Cloner Batch|1. Count", meta=(ClampMin="1", UIMin="1", DisplayName="Global Clone Count"))
	int32 GlobalCloneCount = 24;

	/** Explicit assignments always work, even if Outliner names change later. */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category="Cloner Batch|2. Targets")
	TArray<TObjectPtr<ACEClonerActor>> FrontLineCloners;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category="Cloner Batch|2. Targets")
	TArray<TObjectPtr<ACEClonerActor>> BackLineCloners;

	/** Finds cloners by Actor tag, Outliner folder, actor label, or attachment parent. */
	UFUNCTION(CallInEditor, BlueprintCallable, Category="Cloner Batch|3. Actions", meta=(DisplayName="Discover FrontLine / BackLine Cloners"))
	void DiscoverCloners();

	/** Applies Global Clone Count to all supported targets, then forces a cloner refresh. */
	UFUNCTION(CallInEditor, BlueprintCallable, Category="Cloner Batch|3. Actions", meta=(DisplayName="Apply Clone Count To All"))
	void ApplyCloneCount();

	/** Read-only check; safe to use at any time. */
	UFUNCTION(CallInEditor, BlueprintCallable, Category="Cloner Batch|3. Actions", meta=(DisplayName="Validate Target Counts"))
	void ValidateTargetCounts();

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Cloner Batch|4. Status")
	int32 TargetClonerCount = 0;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Cloner Batch|4. Status")
	int32 MatchingClonerCount = 0;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Cloner Batch|4. Status")
	int32 UnsupportedClonerCount = 0;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Cloner Batch|4. Status", meta=(MultiLine="true"))
	FString LastStatus;

private:
	static bool IsInNamedGroup(const ACEClonerActor* Cloner, const FString& GroupName);
	static bool SetSupportedLayoutCount(ACEClonerActor* Cloner, int32 NewCount);
	static bool GetSupportedLayoutCount(const ACEClonerActor* Cloner, int32& OutCount);
	void GetUniqueTargets(TArray<ACEClonerActor*>& OutTargets) const;
};
