// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemy/Enemy.h"
#include "AIController.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Perception/PawnSensingComponent.h"
#include "Components/AttributeComponent.h"
#include "HUD/HealthBarComponent.h"
#include "WEapons/Weapon.h"


AEnemy::AEnemy()
{
 	
	PrimaryActorTick.bCanEverTick = true;

	GetMesh()->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetMesh()->SetGenerateOverlapEvents(true);

	HealthBarWidget = CreateDefaultSubobject<UHealthBarComponent>(TEXT("HealthBar"));
	HealthBarWidget->SetupAttachment(GetRootComponent());

	GetCharacterMovement()->bOrientRotationToMovement = true;
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	PawnSensing = CreateDefaultSubobject<UPawnSensingComponent>(TEXT("PawnSensing"));
	PawnSensing->SightRadius = 4000.f;
	PawnSensing->SetPeripheralVisionAngle(45.f);
}

void AEnemy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (IsDead()) return;

	if (EnemyState > EEnemyState::EES_Patrolling)
	{
		CheckCombatTarget();
	}
	else
	{
		CheckPatrolTarget();
	}
	UE_LOG(LogTemp, Warning, TEXT("1"));
	
}

float AEnemy::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	HandleDamage(DamageAmount);
	CombatTarget = EventInstigator->GetPawn();
	ChaseTarget();
	return DamageAmount;
	UE_LOG(LogTemp, Warning, TEXT("2"));
}

void AEnemy::Destroyed()
{
	if (EquippedWeapon)
	{
		EquippedWeapon->Destroy();
	}
	UE_LOG(LogTemp, Warning, TEXT("3"));
}

void AEnemy::GetHit_Implementation(const FVector& ImpactPoint)
{
	ShowHealthBar();
	if (IsAlive())
	{
		DirectionalHitReact(ImpactPoint);
	}
	else Die();
	PlayHitSound(ImpactPoint);
	SpawnHitParticles(ImpactPoint);
	UE_LOG(LogTemp, Warning, TEXT("4"));
}

void AEnemy::BeginPlay()
{
	Super::BeginPlay();
	
	if(PawnSensing) PawnSensing->OnSeePawn.AddDynamic(this, &AEnemy::PawnSeen);
	InitializeEnemy();
	StartPatrolling();
	UE_LOG(LogTemp, Warning, TEXT("5"));
}

void AEnemy::Die()
{
	EnemyState = EEnemyState::EES_Dead;
	PlayDeathMontage();
	ClearAttackTimer();
	HideHealthBar();
	DisableCapsule();
	SetLifeSpan(DeathLifeSpan);
	GetCharacterMovement()->bOrientRotationToMovement = false;
	UE_LOG(LogTemp, Warning, TEXT("6"));
}

void AEnemy::Attack()
{
	EnemyState = EEnemyState::EES_Engaged;
	Super::Attack();
	PlayAttackMontage();

	GetWorldTimerManager().SetTimer(AttackEndTimer, this, &AEnemy::AttackEnd, AttackMax);
	UE_LOG(LogTemp, Warning, TEXT("7"));
}

bool AEnemy::CanAttack()
{
	bool bCanAttack =
		IsInsideAttackRadius() &&
		!IsAttacking() &&
		!IsEngaged() &&
		!IsDead();
	return bCanAttack;
	UE_LOG(LogTemp, Warning, TEXT("8"));
}

void AEnemy::AttackEnd()
{
	EnemyState = EEnemyState::EES_NoState;
	CheckCombatTarget();
	UE_LOG(LogTemp, Warning, TEXT("9"));
}

void AEnemy::HandleDamage(float DamageAmount)
{
	Super::HandleDamage(DamageAmount);
	if (Attributes && HealthBarWidget)
	{
		HealthBarWidget->SetHealthPercent(Attributes->GetHealthPercent());
	}
	UE_LOG(LogTemp, Warning, TEXT("10"));
}

int32 AEnemy::PlayDeathMontage()
{
	const int32 Selection = Super::PlayDeathMontage();
	TEnumAsByte<EDeathPose> Pose(Selection);
	if (Pose < EDeathPose::EDP_MAX)
	{
		DeathPose = Pose;
	}

	return Selection;
	UE_LOG(LogTemp, Warning, TEXT("11"));
}

void AEnemy::InitializeEnemy()
{
	EnemyController = Cast<AAIController>(GetController());
	MoveToTarget(PatrolTarget);
	HideHealthBar();
	SpawnDefaultWeapon();
	UE_LOG(LogTemp, Warning, TEXT("12"));
}

void AEnemy::CheckPatrolTarget()
{
	if (InTargetRange(PatrolTarget, PatrolRadius))
	{
		PatrolTarget = ChoosePatrolTarget();
		const float WaitTime = FMath::RandRange(PatrolWaitMin, PatrolWaitMax);
		GetWorldTimerManager().SetTimer(PatrolTimer, this, &AEnemy::PatrolTimerFinished, WaitTime);
	}
	UE_LOG(LogTemp, Warning, TEXT("13"));
}

void AEnemy::CheckCombatTarget()
{
	if (IsOutsideCombatRadius())
	{
		LoseInterest();
		if (!IsEngaged()) StartPatrolling();
	}
	else if (IsOutsideAttackRadius() && !IsChasing())
	{
		if (!IsEngaged()) ChaseTarget();
	}
	else if (CanAttack())
	{
		StartAttackTimer();
	}
	UE_LOG(LogTemp, Warning, TEXT("14"));
}

void AEnemy::PatrolTimerFinished()
{
	MoveToTarget(PatrolTarget);
	UE_LOG(LogTemp, Warning, TEXT("15"));
}

void AEnemy::HideHealthBar()
{
	if (HealthBarWidget)
	{
		HealthBarWidget->SetVisibility(false);
	}
	UE_LOG(LogTemp, Warning, TEXT("16"));
}

void AEnemy::ShowHealthBar()
{
	if (HealthBarWidget)
	{
		HealthBarWidget->SetVisibility(true);
	}
	UE_LOG(LogTemp, Warning, TEXT("17"));
}

void AEnemy::LoseInterest()
{
	CombatTarget = nullptr;
	HideHealthBar();
	UE_LOG(LogTemp, Warning, TEXT("18"));
}

void AEnemy::StartPatrolling()
{
	EnemyState = EEnemyState::EES_Patrolling;
	GetCharacterMovement()->MaxWalkSpeed = PatrollingSpeed;
	MoveToTarget(PatrolTarget);
	UE_LOG(LogTemp, Warning, TEXT("19"));
}

void AEnemy::ChaseTarget()
{
	EnemyState = EEnemyState::EES_Chasing;
	GetCharacterMovement()->MaxWalkSpeed = ChasingSpeed;
	MoveToTarget(CombatTarget);
	UE_LOG(LogTemp, Warning, TEXT("20"));
}

bool AEnemy::IsOutsideCombatRadius()
{
	return !InTargetRange(CombatTarget, CombatRadius);
	UE_LOG(LogTemp, Warning, TEXT("21"));
}

bool AEnemy::IsOutsideAttackRadius()
{
	return !InTargetRange(CombatTarget, AttackRadius);
	UE_LOG(LogTemp, Warning, TEXT("22"));
}

bool AEnemy::IsInsideAttackRadius()
{
	return InTargetRange(CombatTarget, AttackRadius);
	UE_LOG(LogTemp, Warning, TEXT("22"));
}

bool AEnemy::IsChasing()
{
	return EnemyState == EEnemyState::EES_Chasing;
	UE_LOG(LogTemp, Warning, TEXT("23"));
}

bool AEnemy::IsAttacking()
{
	return EnemyState == EEnemyState::EES_Attacking;
	UE_LOG(LogTemp, Warning, TEXT("24"));
}

bool AEnemy::IsDead()
{
	return EnemyState == EEnemyState::EES_Dead;
	UE_LOG(LogTemp, Warning, TEXT("25"));
}

bool AEnemy::IsEngaged()
{
	return EnemyState == EEnemyState::EES_Engaged;
	UE_LOG(LogTemp, Warning, TEXT("26"));
}

void AEnemy::ClearPatrolTimer()
{
	GetWorldTimerManager().ClearTimer(PatrolTimer);
	UE_LOG(LogTemp, Warning, TEXT("27"));
}

void AEnemy::StartAttackTimer()
{
	EnemyState = EEnemyState::EES_Attacking;
	const float AttackTime = FMath::RandRange(AttackMin, AttackMax);
	GetWorldTimerManager().SetTimer(AttackTimer, this, &AEnemy::Attack, AttackTime);
	UE_LOG(LogTemp, Warning, TEXT("28"));
}

void AEnemy::ClearAttackTimer()
{
	GetWorldTimerManager().ClearTimer(AttackTimer);
	GetWorldTimerManager().ClearTimer(AttackEndTimer);
	UE_LOG(LogTemp, Warning, TEXT("29"));
}

bool AEnemy::InTargetRange(AActor* Target, double Radius)
{
	if (Target == nullptr) return false;
	const double DistangeToTarget = (Target->GetActorLocation() - GetActorLocation()).Size();
	return DistangeToTarget <= Radius;
	UE_LOG(LogTemp, Warning, TEXT("30"));
}

void AEnemy::MoveToTarget(AActor* Target)
{
	FAIMoveRequest MoveRequest;
	MoveRequest.SetGoalActor(Target);
	MoveRequest.SetAcceptanceRadius(50.f);

	EnemyController->MoveTo(MoveRequest);
	UE_LOG(LogTemp, Warning, TEXT("31"));

}

AActor* AEnemy::ChoosePatrolTarget()
{
		TArray<AActor*> ValidTargets;
		for (AActor* Target : PatrolTargets)
		{
			if (Target != PatrolTarget)
			{
				ValidTargets.AddUnique(Target);
			}
		}
		const int32 NumPatrolTargets = ValidTargets.Num();
		if (NumPatrolTargets > 0)
		{
			const int32 TargetSelection = FMath::RandRange(0, NumPatrolTargets - 1);
			return ValidTargets[TargetSelection];
		}
	return nullptr;
	UE_LOG(LogTemp, Warning, TEXT("32"));
}

void AEnemy::SpawnDefaultWeapon()
{
	UWorld* World = GetWorld();
	if (World && WeaponClass)
	{
		AWeapon* DefaultWeapon = World->SpawnActor<AWeapon>(WeaponClass);
		DefaultWeapon->Equip(GetMesh(), FName("RightHandSocket"), this, this);
		EquippedWeapon = DefaultWeapon;
	}
	UE_LOG(LogTemp, Warning, TEXT("33"));
}

void AEnemy::PawnSeen(APawn* SeenPawn)
{
	if (SeenPawn == nullptr) return;

	const bool bShouldChaseTarget =
		EnemyState != EEnemyState::EES_Dead &&
		EnemyState != EEnemyState::EES_Chasing &&
		EnemyState < EEnemyState::EES_Attacking &&
		SeenPawn->ActorHasTag(FName("SlashCharacter"));

	if (bShouldChaseTarget)
	{
		ClearPatrolTimer();
		CombatTarget = SeenPawn;
		ChaseTarget();
	}
	UE_LOG(LogTemp, Warning, TEXT("34"));
}




