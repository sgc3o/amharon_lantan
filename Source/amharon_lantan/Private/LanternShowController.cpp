#include "LanternShowController.h"

#include "Cloner/CEClonerActor.h"
#include "Components/SceneComponent.h"
#include "Effector/CEEffectorActor.h"

ALanternShowController::ALanternShowController()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

#if WITH_EDITOR
	bRunConstructionScriptOnDrag = false;
#endif
}

void ALanternShowController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	RefreshDiagnostics();
}

bool ALanternShowController::ShouldTickIfViewportsOnly() const
{
	return true;
}

void ALanternShowController::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RefreshDiagnostics();
}

#if WITH_EDITOR
void ALanternShowController::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	RefreshDiagnostics();
}
#endif

void ALanternShowController::SetAnimationTime(float InAnimationTime)
{
	AnimationTime = FMath::Clamp(InAnimationTime, 0.0f, FMath::Max(0.0f, ShowEndTime));
	RefreshDiagnostics();
}

void ALanternShowController::SetSwayAmount(float InSwayAmount)
{
	SwayAmount = FMath::Clamp(InSwayAmount, 0.0f, 1.0f);
	RefreshDiagnostics();
}

bool ALanternShowController::ConfigureStep2Sway(ACEClonerActor* InCloner, ACEEffectorActor* InEffector)
{
	if (!IsValid(InCloner) || !IsValid(InEffector))
	{
		return false;
	}

	if (IsValid(SwayTestCloner) && IsValid(SwayEffector)
		&& (SwayTestCloner != InCloner || SwayEffector != InEffector))
	{
		SwayTestCloner->UnlinkEffector(SwayEffector);
	}

	SwayTestCloner = InCloner;
	SwayEffector = InEffector;
	if (!SwayTestCloner->IsEffectorLinked(SwayEffector))
	{
		SwayTestCloner->LinkEffector(SwayEffector);
	}

	RefreshDiagnostics();
	return bSwayBindingValid;
}

void ALanternShowController::RefreshDiagnostics()
{
	const float SafeEndTime = FMath::Max(0.0f, ShowEndTime);
	AnimationTime = FMath::Clamp(AnimationTime, 0.0f, SafeEndTime);
	NormalizedShowTime = SafeEndTime > 0.0f ? AnimationTime / SafeEndTime : 0.0f;

	float SectionStartTime = ShowStartTime;
	if (AnimationTime < ShowStartTime)
	{
		CurrentSection = ELanternShowSection::BeforeShow;
		SectionStartTime = 0.0f;
	}
	else if (AnimationTime < FirstMotionTime)
	{
		CurrentSection = ELanternShowSection::Intro;
		SectionStartTime = ShowStartTime;
	}
	else if (AnimationTime < CalmTime)
	{
		CurrentSection = ELanternShowSection::FirstMotion;
		SectionStartTime = FirstMotionTime;
	}
	else if (AnimationTime < AwakeningTime)
	{
		CurrentSection = ELanternShowSection::Calm;
		SectionStartTime = CalmTime;
	}
	else if (AnimationTime < PreStillnessTime)
	{
		CurrentSection = ELanternShowSection::Awakening;
		SectionStartTime = AwakeningTime;
	}
	else if (AnimationTime < StillnessTime)
	{
		CurrentSection = ELanternShowSection::PreStillness;
		SectionStartTime = PreStillnessTime;
	}
	else if (AnimationTime < PreFlightTime)
	{
		CurrentSection = ELanternShowSection::Stillness;
		SectionStartTime = StillnessTime;
	}
	else if (AnimationTime < GlowTime)
	{
		CurrentSection = ELanternShowSection::PreFlight;
		SectionStartTime = PreFlightTime;
	}
	else if (AnimationTime < FlightTime)
	{
		CurrentSection = ELanternShowSection::Glow;
		SectionStartTime = GlowTime;
	}
	else if (AnimationTime <= SafeEndTime)
	{
		CurrentSection = ELanternShowSection::Flight;
		SectionStartTime = FlightTime;
	}
	else
	{
		CurrentSection = ELanternShowSection::AfterShow;
		SectionStartTime = SafeEndTime;
	}

	SecondsIntoCurrentSection = FMath::Max(0.0f, AnimationTime - SectionStartTime);

	SwayAmount = FMath::Clamp(SwayAmount, 0.0f, 1.0f);
	RotationAmount = 0.0f;
	WindAmount = 0.0f;
	EmissionAmount = 0.0f;
	FloatProgress = 0.0f;
	FlightProgress = 0.0f;

	ApplyStep2Sway();
}

void ALanternShowController::ApplyStep2Sway()
{
	bSwayBindingValid = IsValid(SwayTestCloner)
		&& IsValid(SwayEffector)
		&& SwayTestCloner->IsEffectorLinked(SwayEffector);

	if (!bSwayBindingValid)
	{
		EffectiveSwayAmount = 0.0f;
		return;
	}

	// Absolute-time evaluation only: no accumulated DeltaSeconds and no
	// per-frame random calls. Re-evaluating a frame produces the same Pan.
	const float SafeStartupTime = FMath::Max(SwayStartupBlendTime, UE_SMALL_NUMBER);
	const float StartupWeight = FMath::SmoothStep(0.0f, SafeStartupTime, AnimationTime);
	EffectiveSwayAmount = SwayAmount * StartupWeight;

	// Moving a 3D noise field through stable particle positions supplies stable
	// per-particle phase/amplitude variation. Unequal axis rates avoid lockstep.
	const float PhaseCycles = AnimationTime * FMath::Max(0.01f, BaseSwayFrequency);
	const FVector AbsolutePan = FVector(PhaseCycles, PhaseCycles * 0.83f, PhaseCycles * 1.17f);

	SwayEffector->SetEnabled(true);
	SwayEffector->SetType(ECEClonerEffectorType::Unbound);
	SwayEffector->SetMode(ECEClonerEffectorMode::NoiseField);
	SwayEffector->SetMagnitude(EffectiveSwayAmount);
	SwayEffector->SetLocationStrength(FVector::ZeroVector);
	SwayEffector->SetRotationStrength(FRotator(0.0f, YawAmplitude, RollAmplitude));
	SwayEffector->SetScaleStrength(FVector::ZeroVector);
	SwayEffector->SetFrequency(FMath::Max(0.001f, NoiseSpatialFrequency));
	SwayEffector->SetPan(AbsolutePan);
}
