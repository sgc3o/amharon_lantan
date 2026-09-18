#include "ClonerLanternBakeTool.h"

#include "Cloner/CEClonerActor.h"
#include "Cloner/CEClonerComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "LevelSequence.h"
#include "MovieScene.h"
#include "Channels/MovieSceneFloatChannel.h"
#include "Sections/MovieSceneFloatSection.h"
#include "Tracks/MovieSceneFloatTrack.h"
#include "NiagaraComputeExecutionContext.h"
#include "NiagaraDataSet.h"
#include "NiagaraDataSetAccessor.h"
#include "NiagaraEmitterInstance.h"
#include "NiagaraGPUInstanceCountManager.h"
#include "NiagaraGpuComputeDispatchInterface.h"
#include "NiagaraGpuReadbackManager.h"
#include "NiagaraSystemInstance.h"
#include "NiagaraSystemInstanceController.h"
#include "NiagaraTypes.h"
#include "RenderingThread.h"
#include "UObject/ConstructorHelpers.h"

#if WITH_EDITOR
#include "Editor.h"
#include "ScopedTransaction.h"
#endif

DEFINE_LOG_CATEGORY_STATIC(LogClonerLanternBake, Log, All);

namespace ClonerLanternBake
{
	const FName GeneratedActorTag(TEXT("ClonerLanternBakeGenerated"));

#if WITH_EDITOR
	struct FGpuParticleReadback
	{
		FNiagaraDataSet DataSet;
		std::atomic<bool> bComplete = false;
	};

	/**
	 * UE 5.4 ships FNiagaraDataSetReadback publicly, but does not export its
	 * execution functions from the Niagara DLL. This small bridge uses only
	 * exported Niagara/RHI APIs and mirrors the engine's synchronous readback.
	 */
	bool ReadGpuParticleData(
		FNiagaraEmitterInstance& Emitter,
		TSharedPtr<FGpuParticleReadback, ESPMode::ThreadSafe>& OutReadback)
	{
		FNiagaraComputeExecutionContext* GpuContext = Emitter.GetGPUContext();
		FNiagaraSystemInstance* SystemInstance = Emitter.GetParentSystemInstance();
		FNiagaraGpuComputeDispatchInterface* DispatchInterface = SystemInstance ? SystemInstance->GetComputeDispatchInterface() : nullptr;
		if (!GpuContext || !DispatchInterface)
		{
			return false;
		}

		TSharedRef<FGpuParticleReadback, ESPMode::ThreadSafe> Readback = MakeShared<FGpuParticleReadback, ESPMode::ThreadSafe>();
		Readback->DataSet.Init(&Emitter.GetParticleData().GetCompiledData());

		ENQUEUE_RENDER_COMMAND(ClonerLanternBakeReadback)(
			[Readback, DispatchInterface, GpuContext](FRHICommandListImmediate& RHICmdList)
			{
				FNiagaraGpuReadbackManager* ReadbackManager = DispatchInterface->GetGpuReadbackManager();
				FNiagaraDataBuffer* CurrentData = GpuContext->MainDataSet ? GpuContext->MainDataSet->GetCurrentData() : nullptr;
				if (!ReadbackManager || !CurrentData)
				{
					Readback->DataSet.CopyFromGPUReadback(nullptr, nullptr, nullptr, 0, 0, 0, 0, 0);
					Readback->bComplete = true;
					return;
				}

				const uint32 CountOffset = CurrentData->GetGPUInstanceCountBufferOffset();
				if (CountOffset == INDEX_NONE)
				{
					Readback->DataSet.CopyFromGPUReadback(nullptr, nullptr, nullptr, 0, 0, 0, 0, 0);
					Readback->bComplete = true;
					return;
				}

				const FNiagaraGPUInstanceCountManager& CountManager = DispatchInterface->GetGPUInstanceCounterManager();
				FRWBuffer& FloatBuffer = CurrentData->GetGPUBufferFloat();
				FRWBuffer& HalfBuffer = CurrentData->GetGPUBufferHalf();
				FRWBuffer& IntBuffer = CurrentData->GetGPUBufferInt();

				TArray<FNiagaraGpuReadbackManager::FBufferRequest, TInlineAllocator<4>> Buffers;
				Buffers.Emplace(CountManager.GetInstanceCountBuffer().Buffer, CountOffset * sizeof(uint32), sizeof(uint32));
				const int32 FloatIndex = FloatBuffer.NumBytes == 0 ? INDEX_NONE : Buffers.Emplace(FloatBuffer.Buffer, 0, FloatBuffer.NumBytes);
				const int32 HalfIndex = HalfBuffer.NumBytes == 0 ? INDEX_NONE : Buffers.Emplace(HalfBuffer.Buffer, 0, HalfBuffer.NumBytes);
				const int32 IntIndex = IntBuffer.NumBytes == 0 ? INDEX_NONE : Buffers.Emplace(IntBuffer.Buffer, 0, IntBuffer.NumBytes);
				const uint32 FloatStride = CurrentData->GetFloatStride();
				const uint32 HalfStride = CurrentData->GetHalfStride();
				const uint32 IntStride = CurrentData->GetInt32Stride();

				TArray<FRHITransitionInfo, TInlineAllocator<4>> Transitions;
				Transitions.Emplace(Buffers[0].Buffer, ERHIAccess::SRVMask | ERHIAccess::CopySrc, ERHIAccess::CopySrc);
				for (int32 Index = 1; Index < Buffers.Num(); ++Index)
				{
					Transitions.Emplace(Buffers[Index].Buffer, ERHIAccess::SRVMask, ERHIAccess::CopySrc);
				}
				RHICmdList.Transition(Transitions);

				ReadbackManager->EnqueueReadbacks(
					RHICmdList,
					MakeArrayView(Buffers),
					[Readback, FloatIndex, HalfIndex, IntIndex, FloatStride, HalfStride, IntStride]
					(TConstArrayView<TPair<void*, uint32>> BufferData)
					{
						const int32 InstanceCount = *reinterpret_cast<const int32*>(BufferData[0].Key);
						const float* FloatData = FloatIndex == INDEX_NONE ? nullptr : reinterpret_cast<const float*>(BufferData[FloatIndex].Key);
						const FFloat16* HalfData = HalfIndex == INDEX_NONE ? nullptr : reinterpret_cast<const FFloat16*>(BufferData[HalfIndex].Key);
						const int32* IntData = IntIndex == INDEX_NONE ? nullptr : reinterpret_cast<const int32*>(BufferData[IntIndex].Key);
						Readback->DataSet.CopyFromGPUReadback(FloatData, IntData, HalfData, 0, InstanceCount, FloatStride, IntStride, HalfStride);
						Readback->bComplete = true;
					});

				for (FRHITransitionInfo& Transition : Transitions)
				{
					Swap(Transition.AccessBefore, Transition.AccessAfter);
				}
				RHICmdList.Transition(Transitions);
			});

		ENQUEUE_RENDER_COMMAND(ClonerLanternBakeWaitReadback)(
			[DispatchInterface](FRHICommandListImmediate& RHICmdList)
			{
				if (FNiagaraGpuReadbackManager* Manager = DispatchInterface->GetGpuReadbackManager())
				{
					Manager->WaitCompletion(RHICmdList);
				}
			});
		FlushRenderingCommands();

		OutReadback = Readback;
		return Readback->bComplete && Readback->DataSet.GetCurrentData() != nullptr;
	}
#endif

	template<typename TValue>
	FNiagaraDataSetAccessor<TValue> FindAccessor(const FNiagaraDataSet& DataSet, const TCHAR* ShortName)
	{
		FNiagaraDataSetAccessor<TValue> Accessor(DataSet, FName(ShortName));
		if (!Accessor.IsValid())
		{
			Accessor.Init(DataSet, FName(*FString::Printf(TEXT("Particles.%s"), ShortName)));
		}
		return Accessor;
	}

	FString MakeSafeName(const FString& Input)
	{
		FString Result = Input;
		for (TCHAR& Character : Result)
		{
			if (!FChar::IsAlnum(Character) && Character != TEXT('_'))
			{
				Character = TEXT('_');
			}
		}
		return Result;
	}
}

ALanternBakeGroup::ALanternBakeGroup()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SceneRoot->SetMobility(EComponentMobility::Movable);
	RootComponent = SceneRoot;
}

void ALanternBakeGroup::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	ApplyFlight();
}

void ALanternBakeGroup::InitializeFlight()
{
	Modify();
	FlightEntries.Reset();
	FRandomStream Random(FlightSeed);
	for (AActor* Lantern : BakedLanterns)
	{
		if (!IsValid(Lantern))
		{
			continue;
		}

		FLanternFlightEntry& Entry = FlightEntries.AddDefaulted_GetRef();
		Entry.Lantern = Lantern;
		Entry.StartRelativeTransform = Lantern->GetActorTransform().GetRelativeTransform(GetActorTransform());
		const bool bUseLowZ = Random.FRand() < FMath::Clamp(TargetZLowRatio, 0.0f, 1.0f);
		const FVector2D& ZRange = bUseLowZ ? TargetZLowRange : TargetZHighRange;
		Entry.TargetRelativeLocation = FVector(
			Random.FRandRange(TargetXRange.X, TargetXRange.Y),
			Random.FRandRange(TargetYRange.X, TargetYRange.Y),
			Random.FRandRange(ZRange.X, ZRange.Y));
		Entry.DriftDirection = FVector(Random.FRandRange(-1.0f, 1.0f), Random.FRandRange(-1.0f, 1.0f), Random.FRandRange(-0.25f, 0.25f)).GetSafeNormal();
		Entry.DriftAmplitude = Random.FRandRange(DriftAmplitudeRange.X, DriftAmplitudeRange.Y);
		Entry.DriftCycles = Random.FRandRange(DriftCyclesRange.X, DriftCyclesRange.Y);
		Entry.DriftPhase = Random.FRandRange(0.0f, 2.0f * PI);
		Entry.RotationPhase = Random.FRandRange(0.0f, 2.0f * PI);
	}

	FlightProgress = 0.0f;
	LastAppliedProgress = -1.0f;
	ApplyFlight();
}

void ALanternBakeGroup::ApplyFlight()
{
	const float Progress = FMath::Clamp(FlightProgress, 0.0f, 1.0f);
	if (FMath::IsNearlyEqual(Progress, LastAppliedProgress, KINDA_SMALL_NUMBER))
	{
		return;
	}
	LastAppliedProgress = Progress;

	const float EasePart = FMath::Clamp(EaseInRatio, 0.001f, 0.95f);
	const float Travel = Progress < EasePart
		? (Progress * Progress) / (EasePart * (2.0f - EasePart))
		: (2.0f * Progress - EasePart) / (2.0f - EasePart);
	const float SwayEnvelope = 4.0f * Progress * (1.0f - Progress);

	for (const FLanternFlightEntry& Entry : FlightEntries)
	{
		AActor* Lantern = Entry.Lantern;
		if (!IsValid(Lantern))
		{
			continue;
		}

		const float Wave = FMath::Sin(2.0f * PI * Entry.DriftCycles * Progress + Entry.DriftPhase);
		const FVector Location = FMath::Lerp(Entry.StartRelativeTransform.GetLocation(), Entry.TargetRelativeLocation, Travel)
			+ Entry.DriftDirection * Entry.DriftAmplitude * Wave * SwayEnvelope;
		const float RotationWave = FMath::Sin(2.0f * PI * Entry.DriftCycles * Progress + Entry.RotationPhase) * RotationSwayDegrees * SwayEnvelope;
		const FQuat SwayRotation = FRotator(RotationWave * 0.35f, RotationWave, RotationWave * 0.65f).Quaternion();
		Lantern->SetActorRelativeTransform(FTransform(
			SwayRotation * Entry.StartRelativeTransform.GetRotation(),
			Location,
			Entry.StartRelativeTransform.GetScale3D()));
	}
}

void ALanternBakeGroup::DeleteThisBake()
{
#if WITH_EDITOR
	if (!GetWorld())
	{
		return;
	}

	const FScopedTransaction Transaction(NSLOCTEXT("ClonerLanternBake", "DeleteBake", "Delete Cloner Lantern Bake"));
	TArray<TObjectPtr<AActor>> ActorsToDelete = BakedLanterns;
	BakedLanterns.Reset();
	FlightEntries.Reset();
	BakedLanternCount = 0;

	for (AActor* Actor : ActorsToDelete)
	{
		if (IsValid(Actor))
		{
			Actor->Modify();
			GetWorld()->EditorDestroyActor(Actor, true);
		}
	}
	Modify();
#endif
}

AClonerLanternBakeTool::AClonerLanternBakeTool()
{
	PrimaryActorTick.bCanEverTick = false;
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SceneRoot->SetMobility(EComponentMobility::Movable);
	RootComponent = SceneRoot;

	static ConstructorHelpers::FObjectFinder<ULevelSequence> DefaultSequence(TEXT("/Game/Lantan.Lantan"));
	if (DefaultSequence.Succeeded())
	{
		TargetSequence = DefaultSequence.Object;
	}
}

bool AClonerLanternBakeTool::AddFlightTrack(ALanternBakeGroup* Group)
{
#if WITH_EDITOR
	if (!bCreateFlightProgressTrack || !IsValid(TargetSequence) || !IsValid(Group) || FlightEndFrame <= FlightStartFrame)
	{
		return false;
	}

	TargetSequence->Modify();
	UMovieScene* MovieScene = TargetSequence->GetMovieScene();
	if (!MovieScene)
	{
		return false;
	}
	MovieScene->Modify();
	FGuid Binding = TargetSequence->FindBindingFromObject(Group, Group->GetWorld());
	if (!Binding.IsValid())
	{
		Binding = MovieScene->AddPossessable(Group->GetActorLabel(), Group->GetClass());
		TargetSequence->BindPossessableObject(Binding, *Group, Group->GetWorld());
	}
	if (!Binding.IsValid())
	{
		return false;
	}

	UMovieSceneFloatTrack* Track = MovieScene->AddTrack<UMovieSceneFloatTrack>(Binding);
	if (!Track)
	{
		return false;
	}
	Track->Modify();
	Track->SetPropertyNameAndPath(GET_MEMBER_NAME_CHECKED(ALanternBakeGroup, FlightProgress), TEXT("FlightProgress"));
	UMovieSceneFloatSection* Section = Cast<UMovieSceneFloatSection>(Track->CreateNewSection());
	Track->AddSection(*Section);
	Section->Modify();
	Section->SetRange(TRange<FFrameNumber>::All());

	const FFrameRate DisplayRate = MovieScene->GetDisplayRate();
	const FFrameRate TickResolution = MovieScene->GetTickResolution();
	const FFrameNumber StartTick = FFrameRate::TransformTime(FFrameTime(FlightStartFrame), DisplayRate, TickResolution).RoundToFrame();
	const FFrameNumber EndTick = FFrameRate::TransformTime(FFrameTime(FlightEndFrame), DisplayRate, TickResolution).RoundToFrame();
	FMovieSceneFloatChannel& Channel = Section->GetChannel();
	Channel.SetDefault(0.0f);
	Channel.AddLinearKey(StartTick, 0.0f);
	Channel.AddLinearKey(EndTick, 1.0f);
	TargetSequence->MarkPackageDirty();
	return true;
#else
	return false;
#endif
}

void AClonerLanternBakeTool::BakeTargetCloner()
{
#if WITH_EDITOR
	LastResult.Reset();
	if (!IsValid(TargetCloner))
	{
		LastResult = TEXT("失敗: Target Clonerが未指定です。");
		UE_LOG(LogClonerLanternBake, Warning, TEXT("%s"), *LastResult);
		return;
	}

	UCEClonerComponent* ClonerComponent = TargetCloner->FindComponentByClass<UCEClonerComponent>();
	if (!IsValid(ClonerComponent))
	{
		LastResult = TEXT("失敗: Target ClonerにCloner Componentがありません。");
		UE_LOG(LogClonerLanternBake, Warning, TEXT("%s"), *LastResult);
		return;
	}

	TArray<AActor*> SourceActors;
	TargetCloner->GetAttachedActors(SourceActors, true, false);
	SourceActors.RemoveAll([](const AActor* Actor)
	{
		return !IsValid(Actor) || Actor->ActorHasTag(ClonerLanternBake::GeneratedActorTag);
	});
	if (SourceActors.IsEmpty())
	{
		LastResult = TEXT("失敗: Cloner直下に複製元のBP Actorがありません。");
		UE_LOG(LogClonerLanternBake, Warning, TEXT("%s"), *LastResult);
		return;
	}

	FNiagaraSystemInstanceControllerPtr Controller = ClonerComponent->GetSystemInstanceController();
	FNiagaraSystemInstance* SystemInstance = Controller.IsValid() ? Controller->GetSystemInstance_Unsafe() : nullptr;
	if (!SystemInstance)
	{
		// A freshly loaded editor map may not have received its first viewport tick yet.
		// Initialize exactly one frame only in that case; an already visible Cloner is
		// never advanced, so the button remains a snapshot of the state on screen.
		ClonerComponent->Activate(true);
		ClonerComponent->AdvanceSimulation(1, 1.0f / 30.0f);
		Controller = ClonerComponent->GetSystemInstanceController();
		SystemInstance = Controller.IsValid() ? Controller->GetSystemInstance_Unsafe() : nullptr;
	}
	if (!SystemInstance)
	{
		LastResult = TEXT("失敗: ClonerのNiagaraシミュレーションがまだ開始されていません。Viewportを一度更新してから再実行してください。");
		UE_LOG(LogClonerLanternBake, Warning, TEXT("%s"), *LastResult);
		return;
	}

	const FNiagaraDataSet* ParticleDataSet = nullptr;
	FNiagaraEmitterInstance* ParticleEmitter = nullptr;
	TSharedPtr<ClonerLanternBake::FGpuParticleReadback, ESPMode::ThreadSafe> ParticleReadback;
	int32 ParticleCount = 0;
	for (const FNiagaraEmitterInstanceRef& EmitterRef : SystemInstance->GetEmitters())
	{
		FNiagaraEmitterInstance& Emitter = EmitterRef.Get();
		const FNiagaraDataSet* Candidate = nullptr;
		TSharedPtr<ClonerLanternBake::FGpuParticleReadback, ESPMode::ThreadSafe> CandidateReadback;
		if (Emitter.GetSimTarget() == ENiagaraSimTarget::GPUComputeSim)
		{
			// UE 5.4 Motion Design Cloners normally simulate on the GPU. The
			// public Niagara readback utility copies the current GPU particle
			// buffer into a CPU dataset while preserving its compiled layout.
			LastResult = TEXT("GPU上のCloner配置を読み戻しています...");
			if (ClonerLanternBake::ReadGpuParticleData(Emitter, CandidateReadback))
			{
				Candidate = &CandidateReadback->DataSet;
			}
		}
		else
		{
			Candidate = &Emitter.GetParticleData();
		}

		const FNiagaraDataBuffer* CurrentData = Candidate ? Candidate->GetCurrentData() : nullptr;
		const int32 CandidateCount = CurrentData ? static_cast<int32>(CurrentData->GetNumInstances()) : 0;
		const FNiagaraDataSetAccessor<FNiagaraPosition> PositionAccessor =
			Candidate ? ClonerLanternBake::FindAccessor<FNiagaraPosition>(*Candidate, TEXT("Position")) : FNiagaraDataSetAccessor<FNiagaraPosition>();
		if (CandidateCount > ParticleCount && PositionAccessor.IsValid())
		{
			ParticleDataSet = Candidate;
			ParticleEmitter = &Emitter;
			ParticleReadback = CandidateReadback;
			ParticleCount = CandidateCount;
		}
	}

	if (!ParticleDataSet || !ParticleEmitter || ParticleCount <= 0)
	{
		LastResult = TEXT("失敗: GPU読み戻し後のクローンが0個です。Clonerを表示・更新してから再実行してください。");
		UE_LOG(LogClonerLanternBake, Warning, TEXT("%s"), *LastResult);
		return;
	}

	const auto PositionReader = ClonerLanternBake::FindAccessor<FNiagaraPosition>(*ParticleDataSet, TEXT("Position")).GetReader(*ParticleDataSet);
	const auto OrientationReader = ClonerLanternBake::FindAccessor<FQuat4f>(*ParticleDataSet, TEXT("MeshOrientation")).GetReader(*ParticleDataSet);
	const auto ScaleReader = ClonerLanternBake::FindAccessor<FVector3f>(*ParticleDataSet, TEXT("Scale")).GetReader(*ParticleDataSet);
	const auto MeshIndexReader = ClonerLanternBake::FindAccessor<int32>(*ParticleDataSet, TEXT("MeshIndex")).GetReader(*ParticleDataSet);
	if (!PositionReader.IsValid())
	{
		LastResult = TEXT("失敗: NiagaraのPosition属性を読み出せませんでした。");
		UE_LOG(LogClonerLanternBake, Warning, TEXT("%s"), *LastResult);
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		LastResult = TEXT("失敗: 編集中のWorldを取得できませんでした。");
		return;
	}

	const FScopedTransaction Transaction(NSLOCTEXT("ClonerLanternBake", "Bake", "Bake Cloner To Lantern Blueprints"));
	Modify();
	TargetCloner->Modify();

	const FString ClonerName = ClonerLanternBake::MakeSafeName(TargetCloner->GetActorLabel());
	const FString UniqueSuffix = FGuid::NewGuid().ToString(EGuidFormats::Short);
	const FString GroupLabel = FString::Printf(TEXT("Baked_%s_%s"), *ClonerName, *UniqueSuffix);
	const FName BakeFolderPath(*FString::Printf(TEXT("%s/%s"), *OutputFolderRoot.ToString(), *GroupLabel));

	FActorSpawnParameters GroupSpawnParameters;
	GroupSpawnParameters.Name = MakeUniqueObjectName(World->PersistentLevel, ALanternBakeGroup::StaticClass(), FName(*GroupLabel));
	GroupSpawnParameters.ObjectFlags = RF_Transactional;
	GroupSpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ALanternBakeGroup* Group = World->SpawnActor<ALanternBakeGroup>(ALanternBakeGroup::StaticClass(), TargetCloner->GetActorTransform(), GroupSpawnParameters);
	if (!Group)
	{
		LastResult = TEXT("失敗: Bake Groupを作成できませんでした。");
		return;
	}
	Group->Modify();
	Group->SetActorLabel(GroupLabel, true);
	Group->SetFolderPath(BakeFolderPath);
	Group->SourceCloner = TargetCloner;
	Group->Tags.AddUnique(ClonerLanternBake::GeneratedActorTag);

	const FTransform ComponentTransform = ClonerComponent->GetComponentTransform();
	const bool bLocalSpace = ParticleEmitter->IsLocalSpace();
	const FNiagaraLWCConverter LwcConverter = SystemInstance->GetLWCConverter(false);
	int32 SpawnedCount = 0;
	int32 InvalidMeshIndexCount = 0;

	for (int32 ParticleIndex = 0; ParticleIndex < ParticleCount; ++ParticleIndex)
	{
		const int32 MeshIndex = MeshIndexReader.IsValid() ? MeshIndexReader.GetSafe(ParticleIndex, 0) : ParticleIndex % SourceActors.Num();
		if (!SourceActors.IsValidIndex(MeshIndex))
		{
			++InvalidMeshIndexCount;
			continue;
		}

		AActor* SourceActor = SourceActors[MeshIndex];
		if (!IsValid(SourceActor) || !SourceActor->GetClass())
		{
			continue;
		}

		const FNiagaraPosition SimulationPosition = PositionReader.GetSafe(ParticleIndex, FNiagaraPosition(FVector3f::ZeroVector));
		const FVector WorldPosition = bLocalSpace
			? ComponentTransform.TransformPosition(FVector(SimulationPosition))
			: LwcConverter.ConvertSimulationPositionToWorld(SimulationPosition);
		FQuat ParticleRotation = FQuat(OrientationReader.IsValid()
			? OrientationReader.GetSafe(ParticleIndex, FQuat4f::Identity)
			: FQuat4f::Identity);
		FVector ParticleScale = FVector(ScaleReader.IsValid()
			? ScaleReader.GetSafe(ParticleIndex, FVector3f::OneVector)
			: FVector3f::OneVector);
		if (bLocalSpace)
		{
			ParticleRotation = ComponentTransform.GetRotation() * ParticleRotation;
			ParticleScale *= ComponentTransform.GetScale3D();
		}

		const FTransform SourceRelative = SourceActor->GetActorTransform().GetRelativeTransform(TargetCloner->GetActorTransform());
		const FTransform SourceShape(FQuat(SourceRelative.GetRotation()), FVector::ZeroVector, SourceRelative.GetScale3D());
		const FTransform ParticleWorld(ParticleRotation, WorldPosition, ParticleScale);
		const FTransform SpawnTransform = SourceShape * ParticleWorld;

		FActorSpawnParameters SpawnParameters;
		SpawnParameters.Name = MakeUniqueObjectName(World->PersistentLevel, SourceActor->GetClass(), FName(*FString::Printf(TEXT("%s_Baked"), *SourceActor->GetActorLabel())));
		SpawnParameters.ObjectFlags = RF_Transactional;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AActor* BakedActor = World->SpawnActor<AActor>(SourceActor->GetClass(), SpawnTransform, SpawnParameters);
		if (!BakedActor)
		{
			continue;
		}

		BakedActor->Modify();
		BakedActor->SetActorHiddenInGame(false);
		BakedActor->SetIsTemporarilyHiddenInEditor(false);
		BakedActor->SetFolderPath(BakeFolderPath);
		BakedActor->SetActorLabel(FString::Printf(TEXT("%s_%03d"), *SourceActor->GetActorLabel(), ParticleIndex + 1), true);
		BakedActor->Tags.AddUnique(ClonerLanternBake::GeneratedActorTag);
		if (bCreateMovableBakeGroup)
		{
			BakedActor->AttachToActor(Group, FAttachmentTransformRules::KeepWorldTransform);
		}
		Group->BakedLanterns.Add(BakedActor);
		++SpawnedCount;
	}

	if (SpawnedCount <= 0)
	{
		World->EditorDestroyActor(Group, true);
		LastResult = TEXT("失敗: 個別BPを1個も生成できませんでした。Mesh Indexと複製元BPを確認してください。");
		UE_LOG(LogClonerLanternBake, Warning, TEXT("%s"), *LastResult);
		return;
	}

	Group->BakedLanternCount = SpawnedCount;
	Group->InitializeFlight();
	AddFlightTrack(Group);
	LastBakeGroup = Group;
	if (bHideSourceClonerAfterBake)
	{
		TargetCloner->SetActorHiddenInGame(true);
		TargetCloner->SetIsTemporarilyHiddenInEditor(true);
	}

	LastResult = FString::Printf(
		TEXT("成功: %s から %d 個のBPを生成しました。出力: %s%s"),
		*TargetCloner->GetActorLabel(),
		SpawnedCount,
		*BakeFolderPath.ToString(),
		InvalidMeshIndexCount > 0 ? *FString::Printf(TEXT(" / 無効なMesh Index: %d"), InvalidMeshIndexCount) : TEXT(""));
	UE_LOG(LogClonerLanternBake, Display, TEXT("%s"), *LastResult);
	Group->MarkPackageDirty();
	MarkPackageDirty();
#else
	LastResult = TEXT("この機能はUnreal Editor専用です。");
#endif
}

void AClonerLanternBakeTool::ShowSourceCloner()
{
#if WITH_EDITOR
	if (IsValid(TargetCloner))
	{
		const FScopedTransaction Transaction(NSLOCTEXT("ClonerLanternBake", "ShowSource", "Show Source Cloner"));
		TargetCloner->Modify();
		TargetCloner->SetActorHiddenInGame(false);
		TargetCloner->SetIsTemporarilyHiddenInEditor(false);
		LastResult = FString::Printf(TEXT("%s を再表示しました。"), *TargetCloner->GetActorLabel());
	}
	else
	{
		LastResult = TEXT("Target Clonerが未指定です。");
	}
#endif
}

void AClonerLanternBakeTool::DeleteLastBake()
{
#if WITH_EDITOR
	if (!IsValid(LastBakeGroup) || !GetWorld())
	{
		LastResult = TEXT("削除できるLast Bake Groupがありません。");
		return;
	}

	const FScopedTransaction Transaction(NSLOCTEXT("ClonerLanternBake", "DeleteLastBake", "Delete Last Cloner Lantern Bake"));
	ALanternBakeGroup* GroupToDelete = LastBakeGroup;
	Modify();
	LastBakeGroup = nullptr;
	GroupToDelete->DeleteThisBake();
	GroupToDelete->Modify();
	GetWorld()->EditorDestroyActor(GroupToDelete, true);
	LastResult = TEXT("直前のBake Groupを削除しました。必要ならUndoで戻せます。");
#endif
}
