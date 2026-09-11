#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LanternCloner.generated.h"

class UHierarchicalInstancedStaticMeshComponent;
class UMaterialInterface;
class USceneComponent;
class UStaticMesh;

UENUM(BlueprintType)
enum class ELanternClonerMode : uint8
{
	Linear
};

UENUM(BlueprintType)
enum class ELanternCloneMode : uint8
{
	Random,
	Shuffle
};

USTRUCT(BlueprintType)
struct AMHARON_LANTAN_API FLanternVariantDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Identity")
	FName VariantName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Identity", meta=(ClampMin="0"))
	int32 ShapeId = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Identity", meta=(ClampMin="0"))
	int32 ColorId = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Identity", meta=(ClampMin="0.0"))
	float Weight = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Body")
	TObjectPtr<UStaticMesh> BodyMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Body")
	TObjectPtr<UMaterialInterface> BodyMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Body")
	FTransform BodyLocalTransform = FTransform::Identity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tassel")
	TObjectPtr<UStaticMesh> TasselMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tassel")
	TObjectPtr<UMaterialInterface> TasselMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tassel")
	FTransform TasselLocalTransform = FTransform::Identity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Frame")
	TObjectPtr<UStaticMesh> FrameMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Frame")
	TObjectPtr<UMaterialInterface> FrameMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Frame")
	FTransform FrameLocalTransform = FTransform::Identity;
};

UCLASS(BlueprintType, Blueprintable)
class AMHARON_LANTAN_API ALanternCloner : public AActor
{
	GENERATED_BODY()

public:
	ALanternCloner();
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void PostLoad() override;
	virtual void PostRegisterAllComponents() override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostEditMove(bool bFinished) override;
#endif

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Lantern Cloner")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lantern Cloner|Variants")
	TArray<FLanternVariantDefinition> LanternVariants;

	UPROPERTY(meta=(DeprecatedProperty, DeprecationMessage="LanternCloner now always creates one column per Actor"))
	int32 ColumnCount = 1;

	UPROPERTY(meta=(DeprecatedProperty, DeprecationMessage="LanternCloner now always creates one column per Actor"))
	int32 DepthCount = 1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Lantern Cloner|Cloner")
	ELanternClonerMode Mode = ELanternClonerMode::Linear;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lantern Cloner|Cloner")
	ELanternCloneMode CloneMode = ELanternCloneMode::Shuffle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lantern Cloner|Cloner", meta=(DisplayName="Seed"))
	int32 RandomSeed = 1337;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lantern Cloner|Cloner", meta=(DisplayName="Clone Count", ClampMin="1", UIMin="1"))
	int32 VerticalCount = 24;

	UPROPERTY(meta=(DeprecatedProperty))
	float HorizontalSpacing = 100.0f;

	UPROPERTY(meta=(DeprecatedProperty))
	float DepthSpacing = 150.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lantern Cloner|Cloner", meta=(ClampMin="0.0", UIMin="0.0"))
	float VerticalSpacing = 95.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lantern Cloner|Randomization", meta=(DisplayName="Scale Random", ClampMin="0.0", ClampMax="0.5", UIMin="0.0", UIMax="0.2"))
	float ScaleRandom = 0.05f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lantern Cloner|Randomization", meta=(ClampMin="0.0"))
	float RandomYawRange = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lantern Cloner|Randomization", meta=(ClampMin="0.0"))
	float RandomPitchRange = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lantern Cloner|Randomization", meta=(ClampMin="0.0"))
	float RandomRollRange = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lantern Cloner|Randomization")
	FVector RandomPositionOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lantern Cloner|Randomization")
	bool bAvoidConsecutiveSameShape = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lantern Cloner|Randomization")
	bool bAvoidConsecutiveSameColor = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lantern Cloner|Preview")
	bool bUsePreviewVerticalCount = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lantern Cloner|Preview", meta=(ClampMin="1", EditCondition="bUsePreviewVerticalCount"))
	int32 PreviewVerticalCount = 24;

	/** Preview is explicit and capped across ALL columns/depth layers. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lantern Cloner|Preview", meta=(ClampMin="1", ClampMax="256"))
	int32 PreviewMaxLanternCount = 64;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lantern Cloner|Rendering")
	bool bCastShadows = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lantern Cloner|Generation")
	bool bGenerationEnabled = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Lantern Cloner|Stats")
	int32 GeneratedLanternCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Lantern Cloner|Stats")
	int32 GeneratedMeshInstanceCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Lantern Cloner|Stats")
	int32 GeneratedHISMGroupCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Lantern Cloner|Stats")
	TArray<int32> GeneratedVariantCounts;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Lantern Cloner|Stats")
	int32 MaxConsecutiveSameShapeRun = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Lantern Cloner|Stats")
	int32 MaxConsecutiveSameColorRun = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Lantern Cloner|Stats")
	int32 ForegroundCenterCandidateCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Lantern Cloner|Stats")
	int32 GeneratedForegroundCenterCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Lantern Cloner|Stats")
	int32 GeneratedMidFarCenterCount = 0;

	UFUNCTION(CallInEditor, BlueprintCallable, Category="Lantern Cloner|Generation")
	void Generate();

	UFUNCTION(CallInEditor, BlueprintCallable, Category="Lantern Cloner|Generation")
	void Regenerate();

	UFUNCTION(CallInEditor, BlueprintCallable, Category="Lantern Cloner|Generation")
	void Clear();

	UFUNCTION(CallInEditor, BlueprintCallable, Category="Lantern Cloner|Generation")
	void Preview();

	/** Read-only diagnostic for editor regression checks; never builds or changes data. */
	UFUNCTION(BlueprintPure, Category="Lantern Cloner|Stats")
	bool IsGeneratedDataConsistent() const;

private:
	UPROPERTY(Transient)
	TArray<TObjectPtr<UHierarchicalInstancedStaticMeshComponent>> GeneratedComponents;

	bool bUpdatingInstances = false;
	bool CanUpdateInstances() const;
	void RebuildInstances(bool bPreview = false);
	void ClearGeneratedComponents();
	UHierarchicalInstancedStaticMeshComponent* FindOrCreateBucket(UStaticMesh* Mesh, UMaterialInterface* Material, TMap<FString, UHierarchicalInstancedStaticMeshComponent*>& Buckets);
	int32 PopVariantFromShuffleBag(FRandomStream& Stream, TArray<int32>& Bag, int32 PreviousVariantIndex);
	int32 PickRandomVariant(FRandomStream& Stream) const;
	void RefillShuffleBag(FRandomStream& Stream, TArray<int32>& Bag, int32 PreviousVariantIndex) const;
};
