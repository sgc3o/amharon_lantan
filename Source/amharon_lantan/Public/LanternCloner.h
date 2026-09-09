#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LanternCloner.generated.h"

class UHierarchicalInstancedStaticMeshComponent;
class UMaterialInterface;
class USceneComponent;
class UStaticMesh;

UENUM(BlueprintType)
enum class ELanternHeightMode : uint8
{
	VerticalSpacing,
	TotalHeight
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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Lantern Cloner")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lantern Cloner|Variants")
	TArray<FLanternVariantDefinition> LanternVariants;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lantern Cloner|Layout", meta=(ClampMin="1", UIMin="1"))
	int32 ColumnCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lantern Cloner|Layout", meta=(ClampMin="1", UIMin="1"))
	int32 DepthCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lantern Cloner|Layout", meta=(ClampMin="1", UIMin="1"))
	int32 VerticalCount = 24;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lantern Cloner|Layout", meta=(ClampMin="1.0"))
	float HorizontalSpacing = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lantern Cloner|Layout", meta=(ClampMin="1.0"))
	float DepthSpacing = 150.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lantern Cloner|Layout", meta=(ClampMin="1.0"))
	float VerticalSpacing = 95.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lantern Cloner|Layout")
	ELanternHeightMode HeightMode = ELanternHeightMode::VerticalSpacing;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lantern Cloner|Layout", meta=(ClampMin="1.0", EditCondition="HeightMode == ELanternHeightMode::TotalHeight"))
	float TotalHeight = 2185.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lantern Cloner|Layout", meta=(ClampMin="0.0", ClampMax="1.0"))
	float Density = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lantern Cloner|Foreground Gap")
	bool bEnableForegroundCenterGap = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lantern Cloner|Foreground Gap", meta=(ClampMin="1", EditCondition="bEnableForegroundCenterGap"))
	int32 ForegroundDepthLayerCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lantern Cloner|Foreground Gap", meta=(ClampMin="0.0", EditCondition="bEnableForegroundCenterGap"))
	float ForegroundGapHalfWidth = 75.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lantern Cloner|Foreground Gap", meta=(ClampMin="0.0", ClampMax="1.0", EditCondition="bEnableForegroundCenterGap"))
	float ForegroundCenterDensity = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lantern Cloner|Foreground Gap", meta=(EditCondition="bEnableForegroundCenterGap"))
	bool bHardExcludeForegroundCenter = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lantern Cloner|Randomization")
	int32 RandomSeed = 1337;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lantern Cloner|Randomization", meta=(ClampMin="0.01"))
	float RandomScaleMin = 0.9f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lantern Cloner|Randomization", meta=(ClampMin="0.01"))
	float RandomScaleMax = 1.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lantern Cloner|Randomization", meta=(ClampMin="0.0"))
	float RandomYawRange = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lantern Cloner|Randomization", meta=(ClampMin="0.0"))
	float RandomPitchRange = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lantern Cloner|Randomization", meta=(ClampMin="0.0"))
	float RandomRollRange = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lantern Cloner|Randomization")
	FVector RandomPositionOffset = FVector(8.0f, 8.0f, 12.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lantern Cloner|Randomization")
	bool bAvoidConsecutiveSameShape = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lantern Cloner|Randomization")
	bool bAvoidConsecutiveSameColor = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lantern Cloner|Preview")
	bool bUsePreviewVerticalCount = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lantern Cloner|Preview", meta=(ClampMin="1", EditCondition="bUsePreviewVerticalCount"))
	int32 PreviewVerticalCount = 24;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lantern Cloner|Rendering")
	bool bCastShadows = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lantern Cloner|Generation")
	bool bGenerationEnabled = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category="Lantern Cloner|Stats")
	int32 GeneratedLanternCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category="Lantern Cloner|Stats")
	int32 GeneratedMeshInstanceCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category="Lantern Cloner|Stats")
	int32 GeneratedHISMGroupCount = 0;

	UFUNCTION(CallInEditor, BlueprintCallable, Category="Lantern Cloner|Generation")
	void Generate();

	UFUNCTION(CallInEditor, BlueprintCallable, Category="Lantern Cloner|Generation")
	void Regenerate();

	UFUNCTION(CallInEditor, BlueprintCallable, Category="Lantern Cloner|Generation")
	void Clear();

private:
	UPROPERTY(Transient)
	TArray<TObjectPtr<UHierarchicalInstancedStaticMeshComponent>> GeneratedComponents;

	void RebuildInstances();
	void ClearGeneratedComponents();
	UHierarchicalInstancedStaticMeshComponent* FindOrCreateBucket(UStaticMesh* Mesh, UMaterialInterface* Material, TMap<FString, UHierarchicalInstancedStaticMeshComponent*>& Buckets);
	int32 PopVariantFromShuffleBag(FRandomStream& Stream, TArray<int32>& Bag, int32 PreviousVariantIndex);
	void RefillShuffleBag(FRandomStream& Stream, TArray<int32>& Bag) const;
	float GetEffectiveVerticalSpacing(int32 EffectiveVerticalCount) const;
};
