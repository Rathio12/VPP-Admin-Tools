enum InvisToggleType
{
	PLAYER = 1,
	SCRIPT
};

modded class PlayerBase
{
	private bool    hasGodmode;
	private bool    hasUnlimitedAmmo;
	private bool 	hasFlyinCar;
	bool			m_isInvisible;
	float           m_VPlayerScale;
	bool    		m_VScalePlayer;
	vector 			m_VPlayerPosCache;
	private bool    m_VfreezePlayer;
	private string  m_VPlayerName;
	private string  m_VPlayerSteamId;
	private string  m_VPlayerHashedId;
	private int     m_VPlayerSessionId;

	private InvisToggleType m_invisType;

	override void Init()
	{
		super.Init();
		RegisterNetSyncVariableBool("m_isInvisible");
		RegisterNetSyncVariableBool("m_VfreezePlayer");
		RegisterNetSyncVariableBool("m_VScalePlayer");
		RegisterNetSyncVariableBool("hasGodmode");
		RegisterNetSyncVariableBool("hasUnlimitedAmmo");
		RegisterNetSyncVariableBool("m_VPPATStreamLook");
		RegisterNetSyncVariableFloat("m_VPlayerScale", 0.01, 100.0, 3);
		GetRPCManager().AddRPC( "RPC_PlayerBase", "InvokeReload", this, SingleplayerExecutionType.Client );
	}

	void ~PlayerBase()
	{
		ResetEntityName();
	}

	override void EEKilled( Object killer )
	{
		super.EEKilled(killer);
		ResetEntityName();
	}

	override void OnConnect()
	{
		super.OnConnect();
		if (GetIdentity())
		{
			m_VPlayerName = GetIdentity().GetName();
			m_VPlayerSteamId = GetIdentity().GetPlainId();
			m_VPlayerHashedId = GetIdentity().GetId();
			m_VPlayerSessionId = GetIdentity().GetPlayerId();
			SetName(m_VPlayerSteamId);
		}
	}

	override bool CanBeTargetedByAI(EntityAI ai)
	{
		if (InvisibilityStatus())
		{
			return false;
		}
		return super.CanBeTargetedByAI(ai);
	}

	override void SetActions(out TInputActionMap InputActionMap)
	{
		super.SetActions(InputActionMap);
		AddAction(ActionAdminBaseBuilder, InputActionMap);
	}

	private void ResetEntityName()
	{
		if (GetGame() && !GetGame().IsServer())
			return;

		SetName("");
	}

	void VPPHealPlayer(bool set_max = true, bool repair_items = true)
	{
		if (GetGame().IsServer() || !GetGame().IsMultiplayer())
		{
			GetStomach().ClearContents();

			DamageSystem.ResetAllZones(this);
			GetModifiersManager().ResetAll();
			
			// bleeding sources
			if (m_BleedingManagerServer)
				m_BleedingManagerServer.RemoveAllSources();
			
			// Stats
			if (GetPlayerStats())
			{
				int bloodType = GetStatBloodType().Get();
				GetPlayerStats().ResetAllStats();
				GetStatBloodType().Set(bloodType);
			}

			// Agents
			if (m_AgentPool)
				m_AgentPool.RemoveAllAgents();
			
			if (m_StaminaHandler)
				m_StaminaHandler.SetStamina(CfgGameplayHandler.GetStaminaMax());
			
			// uncon
			if (IsUnconscious())
				DayZPlayerSyncJunctures.SendPlayerUnconsciousness(this, false);
			
			// set max
			if (set_max)
			{
				GetStatWater().Set(GetStatWater().GetMax());
				GetStatEnergy().Set(GetStatEnergy().GetMax());
			}
			
			// fix up inventory (skippable: the heal keybind heals without repairing gear)
			if (repair_items)
				FixAllInventoryItems();
			
			//remove bloody hands
			PluginLifespan moduleLifespan = PluginLifespan.Cast(GetPlugin(PluginLifespan));
			moduleLifespan.UpdateBloodyHandsVisibilityEx(this, eBloodyHandsTypes.CLEAN);
			ClearBloodyHandsPenaltyChancePerAgent(eAgents.SALMONELLA);
			
			if (GetArrowManager())
				GetArrowManager().ClearArrows();
		}
	}

	void HealBrokenLegs()
	{
		float add_health_coef = 1.0;
		eBrokenLegs state = GetBrokenLegs();
		if (state == eBrokenLegs.BROKEN_LEGS)
		{
			AddHealth("LeftLeg","Health",(GetMaxHealth("LeftLeg", "Health")  - GetHealth("LeftLeg", "Health")) * add_health_coef);
			AddHealth("RightLeg","Health",(GetMaxHealth("RightLeg", "Health") - GetHealth("RightLeg", "Health")) * add_health_coef);
			AddHealth("RightFoot","Health",(GetMaxHealth("RightFoot", "Health") - GetHealth("RightFoot", "Health")) * add_health_coef);
			AddHealth("LeftFoot","Health",(GetMaxHealth("LeftFoot", "Health") - GetHealth("LeftFoot", "Health")) * add_health_coef);
		}
	}

	void InvokeReload(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type == CallType.Client)
        {
			if ( !GetGame().IsInventoryOpen() && this )
			{
				EntityAI entity_hands = this.GetHumanInventory().GetEntityInHands();
				
				if ( entity_hands && entity_hands.IsWeapon() )
				{
					Magazine foundMag = Magazine.Cast(entity_hands.GetAttachmentByConfigTypeName( "DefaultMagazine" ));
					if( foundMag && foundMag.IsMagazine())
					{
						foundMag.LocalSetAmmoMax();
					}else{
						this.QuickReloadWeapon( entity_hands );
					}
				}
			}
		}
	}

	override void OnVariablesSynchronized()
	{
		super.OnVariablesSynchronized();
		//Client single instance only (this wont trigger on other local entity for another client. See SurvivorBase.c)
		VPPFreezePlayer(m_VfreezePlayer);
	}

	/*
	override void OnRPC(PlayerIdentity sender, int rpc_type, ParamsReadContext ctx)
	{
		super.OnRPC(sender, rpc_type, ctx);
		if (GetGame().IsDedicatedServer() && sender && rpc_type == VPPATRPCs.RPC_SYNC_FREECAM_POS)
		{
			if (!GetPermissionManager().VerifyPermission(sender.GetPlainId(), "FreeCamera"))
				return;

			vector camPos;
			if (ctx.Read(camPos))
			{
				SetPosition(camPos);
			}
		}
	}
	*/

	//look-stream ingest (SERVER): receives the spectated player's true camera
	//angles via the vanilla entity RPC and hands them to the SpectateManager relay
	override void OnRPC(PlayerIdentity sender, int rpc_type, ParamsReadContext ctx)
	{
		super.OnRPC(sender, rpc_type, ctx);

		if (GetGame().IsDedicatedServer() && sender != null && rpc_type == VPPATRPCs.RPC_SPECTATE_LOOK)
		{
			//only the flagged player may stream, and only about THEMSELVES
			if (!m_VPPATStreamLook)
				return;
			if (GetIdentity() == null)
				return;
			if (sender.GetPlainId() != GetIdentity().GetPlainId())
				return;

			float lookYaw;
			float lookPitch;
			if (!ctx.Read(lookYaw))
				return;
			if (!ctx.Read(lookPitch))
				return;

			if (GetSpectateManager())
				GetSpectateManager().RelayLookData(sender.GetPlainId(), lookYaw, lookPitch);
		}
	}

	void VPPSetScaleValue(float scale)
	{
		if (GetGame().IsServer())
		{
			m_VPlayerScale = scale;
			SetSynchDirty();
			SetScale(scale);
			ScriptRPC rpc = new ScriptRPC();
			rpc.Write(scale);
			rpc.Send(this, VPPATRPCs.RPC_SYNC_SCALEVAL, true, NULL);
		}
	}
	
	void VPPSetInvisibility(bool state, InvisToggleType toggleType = InvisToggleType.PLAYER)
	{
		if (GetGame().IsServer())
		{
			m_invisType = toggleType;
			/*
			if (!state)
			{
				//trigger update network bubble for clients
				SetAllowDamage(false);
				dBodyEnableGravity(this, false);
				vector realPos = GetPosition();
				GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(this.PosRelocation, 1000, false, realPos);
				SetPosition("0 1200 0");
			}
			*/

			m_isInvisible = state;
			SetSynchDirty();
			ScriptRPC rpc = new ScriptRPC();
			rpc.Write(state);
			rpc.Send(this, VPPATRPCs.RPC_SYNC_INVISIBILITY, true, NULL);
		}
	}

	void PosRelocation(vector pos)
	{
		if (!GetGame().IsDedicatedServer() || !GetGame().IsServer())
			return;

		SetPosition(pos);
		dBodyEnableGravity(this, true);
		if (!GodModeStatus())
			SetAllowDamage(true);
	}
	
	void setGodMode(bool trigger)
	{
		hasGodmode = trigger;
		SetAllowDamage(!trigger);
		if (GetGame().IsServer())
			SetSynchDirty();
	}

	string VPlayerGetHashedId() { return m_VPlayerHashedId; }
	string VPlayerGetSteamId() { return m_VPlayerSteamId; }
	string VPlayerGetName() { return m_VPlayerName; }
	int VPlayerGetSessionId() { return m_VPlayerSessionId; }

	void VPPFreezePlayer(bool state)
	{
		if (GetGame().IsServer())
		{
			m_VfreezePlayer = state;
			SetSynchDirty();
			return;
		}
		
		HumanInputController hic = this.GetInputController();
		if (hic)
			hic.SetDisabled( state );
	}

	bool VPPIsFreezeControls()
	{
		return m_VfreezePlayer;
	}
	
	/*
		Removes all active bleeding sources (server side), without the full heal
	*/
	void VPPStopBleeding()
	{
		if (!GetGame().IsServer() && GetGame().IsMultiplayer())
			return;
		
		if (m_BleedingManagerServer)
			m_BleedingManagerServer.RemoveAllSources();
	}
	
	bool GodModeStatus()
	{
		return hasGodmode;
	}
	
	InvisToggleType InvisibilityToggleType()
	{
		return m_invisType;
	}

	bool InvisibilityStatus()
	{
		return m_isInvisible;
	}

	float VPPGetPlayerScale()
	{
		return m_VPlayerScale;
	}
	
	void VPPSetUnlimitedAmmo(bool state)
	{
		hasUnlimitedAmmo = state;
		if (GetGame().IsServer())
			SetSynchDirty();
	}
	
	bool VPPIsUnlimitedAmmo()
	{
		return hasUnlimitedAmmo;
	}
	
	void SetCanFlyCars(bool state)
	{
		hasFlyinCar = state;
	}
	
	bool CanFlyCar()
	{
		return hasFlyinCar;
	}

	void UnlimitedAmmoCheck(Weapon_Base weapon)
	{
		if (VPPIsUnlimitedAmmo())
		{
			Magazine magazine;
			if (GetGame().IsServer())
			{
				magazine = weapon.GetMagazine(weapon.GetCurrentMuzzle());

				if (GetGame().IsMultiplayer() && magazine)
	            	GetGame().RemoteObjectDelete(magazine);

				if(magazine)
					magazine.ServerSetAmmoMax();

				if (GetGame().IsMultiplayer() && magazine)
            		GetGame().RemoteObjectCreate(magazine);
			}
			else
			{
				magazine = weapon.GetMagazine(weapon.GetCurrentMuzzle());
				if(magazine)
				{
					magazine.LocalSetAmmoMax();
				}
			}
		}
	}

	/*Misc functions*/
	vector GetHeadingPosition(int distance)
	{
		float heading = MiscGameplayFunctions.GetHeadingAngle(this);
		vector posModifier = Vector(-(distance * Math.Sin(heading)), 0, distance * Math.Cos(heading));
		return posModifier + GetPosition();
	}

	float GetAimingAngleLR()
	{
		HumanCommandWeapons	hcw = GetCommandModifier_Weapons();
		if (hcw != null)
			return hcw.GetBaseAimingAngleLR();
		return 0.0;
	}
	
	float GetAimingAngleUD()
	{
		HumanCommandWeapons	hcw = GetCommandModifier_Weapons();
		if (hcw != null)
			return hcw.GetBaseAimingAngleUD();
		return 0.0;
	}

	/*
		Spectate: CLIENT-LOCAL attach of the (invisible, frozen) admin body to the
		spectated player's Head bone — VPPBR-proven recipe. The engine then composes
		our transform in the same phase as the target's animation/net interpolation,
		and the spectate camera (expressed via CoordToLocal against this live base)
		rides the target's motion lag-free; script-side bone sampling alone is
		always one frame stale (visible as run-jitter and camera phase-through).
		The SERVER's parked body position is untouched — links are per-machine and
		the network bubble keeps updating off the server-side body.
	*/
	private int  m_VPPATOrigPhysLayer;      //captured pre-link interaction mask
	private bool m_VPPATPhysLayerCaptured;

	//1PP true-look streaming: server flips this net-synced flag while this player
	//is spectated; the player's OWN client (the only machine that has the real
	//camera direction) then samples + ships look angles at 20Hz
	private bool  m_VPPATStreamLook;
	private float m_VPPATLookAcc;
	private float m_VPPATLookDbgAcc;
	private bool  m_VPPATLookDbgActive;

	void VPPATSetLookStream(bool state)
	{
		if (GetGame().IsServer())
		{
			m_VPPATStreamLook = state;
			SetSynchDirty();
		}
	}

	override void CommandHandler(float pDt, int pCurrentCommandID, bool pCurrentCommandFinished)
	{
		super.CommandHandler(pDt, pCurrentCommandID, pCurrentCommandFinished);

		//look-angle stream (VPPBR-proven design): mouse-look/freelook exist ONLY as
		//this client's camera state and never replicate — sample the true camera
		//direction and ship it to the server for relay to the spectating admin
		if (m_VPPATStreamLook && !GetGame().IsDedicatedServer() && IsControlledPlayer() && IsAlive())
		{
			if (VPPSpectateConstants.ADS_DEBUG && !m_VPPATLookDbgActive)
			{
				m_VPPATLookDbgActive = true;
				Print("[VPPADS] look stream: ACTIVE on this client (flag synced)");
			}

			m_VPPATLookAcc = m_VPPATLookAcc + pDt;
			if (m_VPPATLookAcc >= VPPSpectateConstants.LOOK_STREAM_SEC)
			{
				m_VPPATLookAcc = 0.0;
				vector lookAngles = GetGame().GetCurrentCameraDirection().VectorToAngles();

				//vanilla ENTITY RPC, deliberately NOT the CF channel: the CF
				//client->server path requires the sender to hold the admin password
				//and silently rejects RPCs from the (non-admin) spectated player
				ScriptRPC lookRpc = new ScriptRPC();
				lookRpc.Write(lookAngles[0]);
				lookRpc.Write(lookAngles[1]);
				lookRpc.Send(this, VPPATRPCs.RPC_SPECTATE_LOOK, true, NULL);

				if (VPPSpectateConstants.ADS_DEBUG)
				{
					m_VPPATLookDbgAcc = m_VPPATLookDbgAcc + VPPSpectateConstants.LOOK_STREAM_SEC;
					if (m_VPPATLookDbgAcc >= 5.0)
					{
						m_VPPATLookDbgAcc = 0.0;
						Print("[VPPADS] look stream: sending yaw=" + lookAngles[0].ToString() + " pitch=" + lookAngles[1].ToString());
					}
				}
			}
		}
		else
		{
			m_VPPATLookDbgActive = false;
		}
	}

	void VPPATAttachToSpectateTarget(PlayerBase target)
	{
		if (target == null)
			return;

		//GHOST the body inside the target: mirror the pose first, then full-child
		//link at the entity root (pivot -1, positionOnly=false) with an identity
		//local TM — feet-at-feet, turning WITH the target, an exact movement replica
		SetPosition(target.GetPosition());
		SetOrientation(target.GetOrientation());

		vector localTM[4] = {"1 0 0", "0 1 0", "0 0 1", "0 0 0"};
		LinkToLocalSpaceOf(target, localTM);
		//DEBUG: ADS_DEBUG_SHOW_BODY keeps the body renderable to inspect the link
		if (VPPSpectateConstants.ADS_DEBUG_SHOW_BODY)
			ClearFlags(EntityFlags.SOLID, true);
		else
			ClearFlags(EntityFlags.VISIBLE|EntityFlags.SOLID, true);

		//capture the ORIGINAL interaction mask before overriding — a player's real
		//mask carries TERRAIN/BUILDING/etc bits; restoring a hardcoded bare
		//CHARACTER layer broke the jump/fall command's ground detection after exit
		//(infinite fall animation / stuck on jump)
		if (!m_VPPATPhysLayerCaptured)
		{
			m_VPPATOrigPhysLayer = dBodyGetInteractionLayer(this);
			m_VPPATPhysLayerCaptured = true;
		}
		//layer copied verbatim from VPPBR (note: NOCOLLISION is 0, so this equals RAGDOLL)
		dBodySetInteractionLayer(this, PhxInteractionLayers.NOCOLLISION|PhxInteractionLayers.RAGDOLL);
		//ghost physics controller: not solid to the target (vehicles do the same to
		//linked occupants natively)
		PhysicsSetSolid(false);
		target.AddChild(this, -1, false);
	}

	/*
		Cheap link re-assert: the admin body is network-synced, so server position
		corrections arrive while the target moves (server and client disagree on a
		moving target's position in flight) and get reconciled by REWRITING the
		client link's local TM away from identity — a frozen world-frame offset
		that misaligns the whole camera composition until re-asserted. Safe to
		call every frame; one native call.
	*/
	void VPPATReassertSpectateLink(PlayerBase target)
	{
		if (target == null)
			return;

		vector localTM[4] = {"1 0 0", "0 1 0", "0 0 1", "0 0 0"};
		LinkToLocalSpaceOf(target, localTM);
	}

	void VPPATDetachFromSpectateTarget()
	{
		UnlinkFromLocalSpace();
		Object linkParent = Object.Cast(GetParent());
		if (linkParent)
			linkParent.RemoveChild(this);

		//restore local render state; if server-synced invisibility is still active,
		//its RPC handler + per-postframe SetInvisible keep the body hidden anyway
		SetFlags(EntityFlags.VISIBLE|EntityFlags.SOLID, true);

		//restore the EXACT captured interaction mask (never a hardcoded layer)
		if (m_VPPATPhysLayerCaptured)
		{
			dBodySetInteractionLayer(this, m_VPPATOrigPhysLayer);
			m_VPPATPhysLayerCaptured = false;
		}

		//FULL physics-controller recovery — the link natively suspends these (it is
		//the vehicle-attachment mechanism) and nothing re-enables them on unlink:
		//with gravity/simulation dead, a jump starts the fall command and the body
		//never comes down or registers ground contact (infinite fall animation).
		//Vanilla precedent: PhysicsSetSolid(true) is vanilla's own restore when
		//pulling a body out of a vehicle link (DayZPlayerImplement HandleDeath).
		PhysicsSetSolid(true);
		PhysicsEnableGravity(true);
		DisableSimulation(false);
		dBodyActive(this, ActiveState.ACTIVE);
	}

	/*
		Spectate ADS: the engine polls this on the CONTROLLED player to ask which
		optics to draw as 2D overlays (the gate is script-side, not engine-side —
		see vanilla DayZPlayerImplement.OnDrawOptics2D). While spectating, the
		admin's frozen body is still the controlled player, so handing back the
		TARGET's 2D optic makes the real scope overlay (tube + reticle) render
		for the spectator.
	*/
	protected override array<InventoryItem> OnDrawOptics2D()
	{
		if (!GetGame().IsDedicatedServer())
			VPPDayZPlayerCameraSpectate.DbgOverlayHeartbeat(IsControlledPlayer());

		if (!GetGame().IsDedicatedServer() && IsControlledPlayer() && g_Game.IsSpectateMode())
		{
			VPPDayZPlayerCameraSpectate.DbgMarkOverlayPoll();
			ItemOptics spectateOptic = VPPDayZPlayerCameraSpectate.GetActive2DOverlayOptic();
			if (spectateOptic)
			{
				VPPDayZPlayerCameraSpectate.DbgMarkOverlayServe(spectateOptic);
				array<InventoryItem> optics = new array<InventoryItem>;
				optics.Insert(spectateOptic);
				return optics;
			}
			//spectating with no spectate optic to draw: NEVER fall through to super —
			//it would serve the admin's own latched optic/NVGs over the spectate view
			return null;
		}
		return super.OnDrawOptics2D();
	}
};