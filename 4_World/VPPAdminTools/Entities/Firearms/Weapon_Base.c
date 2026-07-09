modded class Weapon_Base
{
	override void EEFired(int muzzleType, int mode, string ammoType)
	{
		super.EEFired (muzzleType, mode, ammoType);
		PlayerBase player = PlayerBase.Cast(GetHierarchyRootPlayer());
		if ( player )
		{
			player.UnlimitedAmmoCheck(this);
		}
	}

	WeaponFSM VPPGetWpnFSM()
	{
		return m_fsm;
	}

	/*
		Spectate ADS: unconditional barrel un-hide. The vanilla HideWeaponBarrel(false)
		re-checks the attached optic at call time, so an optic detached between the
		spectator's hide and restore would leave the barrel invisible on this client.
	*/
	void VPPForceShowBarrel()
	{
		if (!g_Game.IsDedicatedServer() && m_weaponHideBarrelIdx != -1)
			SetSimpleHiddenSelectionState(m_weaponHideBarrelIdx, true);
	}
};