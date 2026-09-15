#include "WingEffectorYLimitComponent.h"

#include "Cloner/CEClonerActor.h"
#include "Effector/CEEffectorActor.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"

UWingEffectorYLimitComponent::UWingEffectorYLimitComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
	bTickInEditor = true;
}

UWingEffectorYLimitComponent* UWingEffectorYLimitComponent::AddToActor(AActor* TargetActor)
{
	if (!IsValid(TargetActor))
	{
		return nullptr;
	}

	if (UWingEffectorYLimitComponent* Existing = TargetActor->FindComponentByClass<UWingEffectorYLimitComponent>())
	{
		return Existing;
	}

	TargetActor->Modify();
	UWingEffectorYLimitComponent* Component = NewObject<UWingEffectorYLimitComponent>(
		TargetActor,
		UWingEffectorYLimitComponent::StaticClass(),
		TEXT("WingEffectorYLimit"),
		RF_Transactional);
	if (!Component)
	{
		return nullptr;
	}

	TargetActor->AddInstanceComponent(Component);
	Component->OnComponentCreated();
	Component->RegisterComponent();
	TargetActor->MarkPackageDirty();
	return Component;
}

void UWingEffectorYLimitComponent::OnRegister()
{
	Super::OnRegister();
	RestartSwayFromCenter();
}

void UWingEffectorYLimitComponent::BeginPlay()
{
	Super::BeginPlay();
	RestartSwayFromCenter();
}

void UWingEffectorYLimitComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	ApplyBoundedSway(DeltaTime);
}

void UWingEffectorYLimitComponent::RestartSwayFromCenter()
{
	SwayElapsedSeconds = 0.0f;
	CurrentYRotation = (MinYRotation + MaxYRotation) * 0.5f;
	CurrentZRotation = (MinZRotation + MaxZRotation) * 0.5f;
}

bool UWingEffectorYLimitComponent::HasForceConfigurationChanged() const
{
	return !FMath::IsNearlyEqual(LastMinYRotation, MinYRotation)
		|| !FMath::IsNearlyEqual(LastMaxYRotation, MaxYRotation)
		|| !FMath::IsNearlyEqual(LastSwayFrequency, SwayFrequency)
		|| bLastEnableZSway != bEnableZSway
		|| !FMath::IsNearlyEqual(LastMinZRotation, MinZRotation)
		|| !FMath::IsNearlyEqual(LastMaxZRotation, MaxZRotation)
		|| !FMath::IsNearlyEqual(LastZSwayFrequency, ZSwayFrequency)
		|| !FMath::IsNearlyEqual(LastZRandomness, ZRandomness);
}

void UWingEffectorYLimitComponent::RememberForceConfiguration()
{
	LastMinYRotation = MinYRotation;
	LastMaxYRotation = MaxYRotation;
	LastSwayFrequency = SwayFrequency;
	bLastEnableZSway = bEnableZSway;
	LastMinZRotation = MinZRotation;
	LastMaxZRotation = MaxZRotation;
	LastZSwayFrequency = ZSwayFrequency;
	LastZRandomness = ZRandomness;
}

void UWingEffectorYLimitComponent::ResetClonerOrientations(ACEEffectorActor* Effector)
{
	Effector->SetOrientationForceEnabled(false);
	Effector->SetOrientationForceRate(0.0f);
	Effector->SetOrientationForceMin(FVector::ZeroVector);
	Effector->SetOrientationForceMax(FVector::ZeroVector);

	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<ACEClonerActor> It(World); It; ++It)
		{
			if (It->IsEffectorLinked(Effector))
			{
				It->ForceUpdateCloner();
			}
		}
	}

	SwayElapsedSeconds = 0.0f;
	RememberForceConfiguration();
	bOrientationForceInitialized = true;
}

void UWingEffectorYLimitComponent::ApplyBoundedSway(float DeltaTime)
{
	ACEEffectorActor* Effector = Cast<ACEEffectorActor>(GetOwner());
	if (!IsValid(Effector) || IsTemplate())
	{
		return;
	}

	if (!bLimitYRotation)
	{
		Effector->SetOrientationForceEnabled(false);
		bOrientationForceInitialized = false;
		return;
	}

	if (!bOrientationForceInitialized || HasForceConfigurationChanged())
	{
		ResetClonerOrientations(Effector);
	}

	const ECEClonerEffectorMode DesiredMode = EffectorMode == EWingEffectorMotionMode::NoiseField
		? ECEClonerEffectorMode::NoiseField
		: ECEClonerEffectorMode::Default;
	if (Effector->GetMode() != DesiredMode)
	{
		Effector->SetMode(DesiredMode);
	}

	SwayElapsedSeconds += FMath::Max(0.0f, DeltaTime);
	const float LowerAngle = FMath::Min(MinYRotation, MaxYRotation);
	const float UpperAngle = FMath::Max(MinYRotation, MaxYRotation);
	const float CenterAngle = (LowerAngle + UpperAngle) * 0.5f;
	const float Amplitude = (UpperAngle - LowerAngle) * 0.5f;
	const float SlowYFrequency = FMath::Max(0.001f, SwayFrequency);
	const float FastYFrequency = SlowYFrequency * FMath::Lerp(1.35f, 2.35f, FMath::Clamp(ZRandomness, 0.0f, 1.0f));
	const float SlowYPhase = UE_TWO_PI * SlowYFrequency * SwayElapsedSeconds;
	const float FastYPhase = UE_TWO_PI * FastYFrequency * SwayElapsedSeconds;
	const float YAmplitudeRadians = FMath::DegreesToRadians(Amplitude);

	const float LowerZAngle = FMath::Min(MinZRotation, MaxZRotation);
	const float UpperZAngle = FMath::Max(MinZRotation, MaxZRotation);
	const float CenterZAngle = bEnableZSway ? (LowerZAngle + UpperZAngle) * 0.5f : 0.0f;
	const float ZAmplitudeRadians = bEnableZSway
		? FMath::DegreesToRadians((UpperZAngle - LowerZAngle) * 0.5f)
		: 0.0f;
	const float SlowZFrequency = FMath::Max(0.001f, ZSwayFrequency);
	const float FastZFrequency = SlowZFrequency * FMath::Lerp(1.55f, 2.65f, FMath::Clamp(ZRandomness, 0.0f, 1.0f));
	const float SlowZPhase = UE_TWO_PI * SlowZFrequency * SwayElapsedSeconds;
	const float FastZPhase = UE_TWO_PI * FastZFrequency * SwayElapsedSeconds;

	// A cosine angular velocity integrates to a sine angle. Multiplying each
	// axis by its frequency makes every curve reach the same requested angular
	// amplitude. Niagara chooses a stable per-clone blend between Min and Max.
	const FVector ForceMin(
		0.0f,
		YAmplitudeRadians * SlowYFrequency * FMath::Cos(SlowYPhase),
		ZAmplitudeRadians * SlowZFrequency * FMath::Cos(SlowZPhase));
	const FVector ForceMax(
		0.0f,
		YAmplitudeRadians * FastYFrequency * FMath::Cos(FastYPhase),
		ZAmplitudeRadians * FastZFrequency * FMath::Cos(FastZPhase));

	Effector->SetRotation(FRotator(CenterAngle, CenterZAngle, 0.0f));
	Effector->SetOrientationForceMin(ForceMin);
	Effector->SetOrientationForceMax(ForceMax);
	Effector->SetOrientationForceRate(1.0f);
	Effector->SetOrientationForceEnabled(true);

	CurrentYRotation = CenterAngle + Amplitude * 0.5f
		* (FMath::Sin(SlowYPhase) + FMath::Sin(FastYPhase));
	CurrentZRotation = CenterZAngle + FMath::RadiansToDegrees(ZAmplitudeRadians) * 0.5f
		* (FMath::Sin(SlowZPhase) + FMath::Sin(FastZPhase));
}
