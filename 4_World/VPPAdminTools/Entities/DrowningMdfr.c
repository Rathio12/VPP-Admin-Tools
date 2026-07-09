/*
	Godmoded players must never drown.

	The vanilla DrowningMdfr drains health/shock via DIRECT AddHealth() calls
	(OnTick), which bypass SetAllowDamage — so godmode alone does not protect
	against it. This matters for admins in godmode generally, and specifically
	for the spectate system, whose frozen admin body is parked under the target
	and can end up under water.

	Runs server-side (ModifiersManager), where GodModeStatus() is authoritative.
*/
modded class DrowningMdfr
{
	override bool ActivateCondition(PlayerBase player)
	{
		if (player && player.GodModeStatus())
			return false;
		return super.ActivateCondition(player);
	}

	override bool DeactivateCondition(PlayerBase player)
	{
		//force an already-active drowning to end the moment godmode turns on
		if (player && player.GodModeStatus())
			return true;
		return super.DeactivateCondition(player);
	}

	override void OnTick(PlayerBase player, float deltaT)
	{
		if (player && player.GodModeStatus())
			return;
		super.OnTick(player, deltaT);
	}
};
