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

float UCMCombatTactics::GetRepositionCooldown_Implementation() const
{
	return 4.5f;
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
	const float RadialAdjustment = FMath::Clamp(
		DesiredStandoff - Distance, -180.0f, 180.0f);
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
