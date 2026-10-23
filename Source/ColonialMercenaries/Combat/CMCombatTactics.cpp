// Copyright (c) Colonial Mercenaries contributors. All rights reserved.
#include "Combat/CMCombatTactics.h"

bool UCMCombatTactics::WantsReposition_Implementation(
	bool bCanShoot, float TimeSinceLastMove, float DistanceToThreat, float WeaponRange) const
{
	return false;
}

FVector UCMCombatTactics::ChoosePosition_Implementation(
	FVector SelfPosition, FVector ThreatPosition, FVector DutyCenter,
	float LeashRadius, float WeaponRange, int32 SideSeed, bool bCanShoot) const
{
	return SelfPosition;
}

float UCMCombatTactics::GetShotSpreadDegrees_Implementation(
	float DistanceToTarget, float WeaponRange, bool bMoving) const
{
	return 2.0f;
}

float UCMCombatTactics::GetRepositionCooldown_Implementation() const
{
	return 4.5f;
}

float UCMCombatTactics::GetNoShotRepositionCooldown_Implementation() const
{
	return 1.5f;
}

bool UCMRiflemanCombatTactics::WantsReposition_Implementation(
	bool bCanShoot, float TimeSinceLastMove, float DistanceToThreat, float WeaponRange) const
{
	// Move immediately when blocked; if firing, shift position between bursts,
	// rather than continuously strafing or interrupting every single shot.
	return !bCanShoot || TimeSinceLastMove >= FMath::Max(RepositionCooldown, 0.5f);
}

FVector UCMRiflemanCombatTactics::ChoosePosition_Implementation(
	FVector SelfPosition, FVector ThreatPosition, FVector DutyCenter,
	float LeashRadius, float WeaponRange, int32 SideSeed, bool bCanShoot) const
{
	const FVector Away = (SelfPosition - ThreatPosition).GetSafeNormal2D();
	const FVector Side = FVector(-Away.Y, Away.X, 0.0f)
		* ((SideSeed & 1) ? 1.0f : -1.0f);
	const float Range = FMath::Max(WeaponRange, 100.0f);
	const FVector EnemyToSelf = SelfPosition - ThreatPosition;
	const float Distance = EnemyToSelf.Size2D();

	// Stepping to the side seeks a different firing lane. Close only when
	// blocked or clearly outside the preferred effective engagement range.
	const float DesiredStandoff = Range * FMath::Clamp(PreferredRangeRatio, 0.2f, 0.95f);
	// Previously the NPC could approach only 180 cm every ~4.5 seconds,
	// often giving the appearance of standing still when attacked by a
	// player firing from beyond the NPC's short weapon range.
	// Close decisively when out of range, but remain within the leash.
	const float MaximumApproach = bCanShoot ? 180.0f : 900.0f;
	const float RadialAdjustment = FMath::Clamp(
		DesiredStandoff - Distance, -MaximumApproach, 180.0f);
	FVector Position = SelfPosition + Side * FMath::Max(LateralMoveDistance, 0.0f)
		+ Away * RadialAdjustment;
	Position.Z = SelfPosition.Z;

	// Commands define a hard leash. Navigation further projects to NavMesh.
	const FVector Offset = Position - DutyCenter;
	const float MaxDistance = FMath::Max(LeashRadius, 150.0f);
	if (Offset.Size2D() > MaxDistance)
	{
		Position = DutyCenter + Offset.GetSafeNormal2D() * MaxDistance;
		Position.Z = SelfPosition.Z;
	}
	return Position;
}

float UCMRiflemanCombatTactics::GetRepositionCooldown_Implementation() const
{
	return FMath::Max(RepositionCooldown, 0.5f);
}

float UCMRiflemanCombatTactics::GetNoShotRepositionCooldown_Implementation() const
{
	return FMath::Max(NoShotRepositionCooldown, 0.3f);
}

float UCMRiflemanCombatTactics::GetShotSpreadDegrees_Implementation(
	float DistanceToTarget, float WeaponRange, bool bMoving) const
{
	const float RangeAlpha = FMath::Clamp(
		DistanceToTarget / FMath::Max(WeaponRange, 1.0f), 0.0f, 1.0f);
	const float Baseline = FMath::Lerp(
		FMath::Max(CloseRangeSpreadDegrees, 0.0f),
		FMath::Max(LongRangeSpreadDegrees, 0.0f), RangeAlpha);
	return FMath::Clamp(Baseline
		+ (bMoving ? FMath::Max(MovingSpreadPenaltyDegrees, 0.0f) : 0.0f),
		0.0f, 30.0f);
}
