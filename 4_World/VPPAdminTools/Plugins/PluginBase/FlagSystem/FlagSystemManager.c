/*
	FlagSystem: flags kills for review when the killer has less than MinPlaytimeHours
	tracked on THIS server AND the kill is suspicious:
	 - at/above LongRangeKillMeters (any hit zone), or
	 - a head/brain kill (any range)

	Fresh accounts sniping across the map or headshotting everything is the classic
	cheater pattern; regulars with real playtime never trigger it.

	Config:  $profile:VPPAdminTools/ConfigurablePlugins/FlagSystem.json (Enabled 0/1, restart to apply)
	Review:  $profile:VPPAdminTools/FlagSystem/FlagDatabase.json
	Webhook: set DiscordWebhookURL to also post each flag to Discord.
*/
class FlagSystemManager: ConfigurablePlugin
{
	int    Enabled;
	int    MinPlaytimeHours;    //killer below this = "low playtime"
	int    LongRangeKillMeters; //kill at/above this distance
	int    NotifyOnlineAdmins;
	int    ExemptAdmins;        //1 = never flag superadmins/usergroup members
	ref array<string> ExemptIds;
	string DiscordWebhookURL;

	[NonSerialized()]
	private ref VPPATFlagDatabase m_Database;
	[NonSerialized()]
	private ref map<string, VPPATPlayerFlagProfile> m_Index;
	[NonSerialized()]
	private ref map<string, float> m_SessionStart;
	[NonSerialized()]
	private ref map<string, string> m_LastHitZone;
	[NonSerialized()]
	private ref map<string, string> m_LastHitWeapon;
	[NonSerialized()]
	private ref map<string, float> m_LastHitDist;
	[NonSerialized()]
	private ref map<string, float> m_LastHitTime;
	[NonSerialized()]
	private ref Timer m_AutoSave;

	static const float HIT_ATTRIBUTION_WINDOW = 15.0; //seconds a hit stays valid for kill attribution

	static const string DB_PATH = "$profile:VPPAdminTools/FlagSystem/FlagDatabase.json";

	void FlagSystemManager()
	{
		Enabled             = 1;
		MinPlaytimeHours    = 25;
		LongRangeKillMeters = 700;
		NotifyOnlineAdmins  = 1;
		ExemptAdmins        = 1;
		ExemptIds           = new array<string>;
		DiscordWebhookURL   = "";

		JSONPATH = "$profile:VPPAdminTools/ConfigurablePlugins/FlagSystem.json";
	}

	override void OnInit()
	{
		Load();

		//VerifyPermission rejects any name that isn't registered; register even when
		//the system is disabled so the "See Flags" button can report that state.
		GetPermissionManager().AddPermissionType({"PlayerManager:SeeFlags"});

		if (Enabled != 1)
		{
			GetSimpleLogger().Log("[FlagSystem] Disabled via FlagSystem.json");
			return;
		}

		if (!FileExist("$profile:VPPAdminTools/FlagSystem"))
			MakeDirectory("$profile:VPPAdminTools/FlagSystem");

		m_Index         = new map<string, VPPATPlayerFlagProfile>;
		m_SessionStart  = new map<string, float>;
		m_LastHitZone   = new map<string, string>;
		m_LastHitWeapon = new map<string, string>;
		m_LastHitDist   = new map<string, float>;
		m_LastHitTime   = new map<string, float>;

		LoadDatabase();

		VPPATGetEventHandler().GetEventInvoker("OnPlayerConnect").Insert(OnFlagPlayerConnect);
		VPPATGetEventHandler().GetEventInvoker("OnPlayerDisconnected").Insert(OnFlagPlayerDisconnect);

		m_AutoSave = new Timer();
		m_AutoSave.Run(300, this, "FlushAndSave", NULL, true);

		GetSimpleLogger().Log(string.Format("[FlagSystem] Enabled. Flagging kills >= %1m and head/brain kills by players with < %2h server playtime.", LongRangeKillMeters, MinPlaytimeHours));
	}

	bool IsEnabled()
	{
		return Enabled == 1;
	}

	override void Load()
	{
		if (FileExist(JSONPATH))
		{
			JsonFileLoader<FlagSystemManager>.JsonLoadFile(JSONPATH, this);
		}
		else
		{
			GetSimpleLogger().Log("[FlagSystem] Config missing, creating default FlagSystem.json");
			Save();
		}
	}

	override void Save()
	{
		JsonFileLoader<FlagSystemManager>.JsonSaveFile(JSONPATH, this);
	}

	void LoadDatabase()
	{
		m_Database = new VPPATFlagDatabase();
		if (FileExist(DB_PATH))
			JsonFileLoader<VPPATFlagDatabase>.JsonLoadFile(DB_PATH, m_Database);

		if (!m_Database.profiles)
			m_Database.profiles = new array<ref VPPATPlayerFlagProfile>;

		foreach(VPPATPlayerFlagProfile prof : m_Database.profiles)
		{
			if (prof && prof.uid != string.Empty)
				m_Index.Set(prof.uid, prof);
		}
	}

	void SaveDatabase()
	{
		if (m_Database)
			JsonFileLoader<VPPATFlagDatabase>.JsonSaveFile(DB_PATH, m_Database);
	}

	//Periodic flush: bank the playtime of everyone still online, so a server crash or
	//hard restart doesn't wipe the whole session's tracked playtime.
	void FlushAndSave()
	{
		if (!m_SessionStart || !m_Index)
			return;

		float now = GetGame().GetTickTime();

		//snapshot the keys first: writing to the map while iterating it is not safe
		array<string> onlineIds = new array<string>;
		foreach (string sid, float sstart : m_SessionStart)
			onlineIds.Insert(sid);

		foreach (string uid : onlineIds)
		{
			float start;
			if (!m_SessionStart.Find(uid, start))
				continue;

			VPPATPlayerFlagProfile prof;
			if (m_Index.Find(uid, prof))
				prof.playtimeSec = prof.playtimeSec + (now - start);

			m_SessionStart.Set(uid, now); //rebase so the same seconds aren't counted twice
		}
		SaveDatabase();
	}

	void ClearHitData(string uid)
	{
		if (m_LastHitZone)   m_LastHitZone.Remove(uid);
		if (m_LastHitWeapon) m_LastHitWeapon.Remove(uid);
		if (m_LastHitDist)   m_LastHitDist.Remove(uid);
		if (m_LastHitTime)   m_LastHitTime.Remove(uid);
	}

	string NowString()
	{
		int year, month, day, hour, minute, second;
		GetYearMonthDayUTC(year, month, day);
		GetHourMinuteSecondUTC(hour, minute, second);
		return string.Format("%1-%2-%3 %4:%5:%6 UTC", year, month, day, hour, minute, second);
	}

	VPPATPlayerFlagProfile GetOrCreateProfile(string uid, string name)
	{
		VPPATPlayerFlagProfile prof;
		if (m_Index.Find(uid, prof))
			return prof;

		prof = new VPPATPlayerFlagProfile();
		prof.uid = uid;
		prof.name = name;
		prof.firstSeen = NowString();
		m_Database.profiles.Insert(prof);
		m_Index.Set(uid, prof);
		return prof;
	}

	//total tracked playtime including the running session
	float GetPlaytimeHours(string uid)
	{
		float secs = 0;
		VPPATPlayerFlagProfile prof;
		if (m_Index.Find(uid, prof))
			secs = prof.playtimeSec;

		float start;
		if (m_SessionStart.Find(uid, start))
			secs = secs + (GetGame().GetTickTime() - start);

		return secs / 3600;
	}

	//---- connect / disconnect: playtime tracking ----
	void OnFlagPlayerConnect(PlayerBase player, PlayerIdentity identity, bool canceledLogout, bool logAsLogin)
	{
		if (!identity)
			return;

		string uid = identity.GetPlainId();
		VPPATPlayerFlagProfile prof = GetOrCreateProfile(uid, identity.GetName());
		prof.name = identity.GetName();

		if (!m_SessionStart.Contains(uid))
			m_SessionStart.Set(uid, GetGame().GetTickTime());
	}

	void OnFlagPlayerDisconnect(PlayerBase player, PlayerIdentity identity, bool finished)
	{
		if (!identity)
			return;

		string uid = identity.GetPlainId();
		float start;
		if (m_SessionStart.Find(uid, start))
		{
			VPPATPlayerFlagProfile prof;
			if (m_Index.Find(uid, prof))
				prof.playtimeSec = prof.playtimeSec + (GetGame().GetTickTime() - start);

			m_SessionStart.Remove(uid);
			SaveDatabase();
		}

		ClearHitData(uid); //don't leak hit records for players who left
	}

	//---- kill handling (called from PluginAdminLog) ----
	//Records the last hit per victim. Distance is captured HERE (at impact) rather than at
	//death, because the killer can move between the shot landing and the death event.
	void OnPlayerHit(PlayerBase victim, PlayerBase attacker, string dmgZone, string ammo, EntityAI source)
	{
		if (Enabled != 1 || !m_LastHitZone || !victim || !attacker || !victim.GetIdentity())
			return;

		string vuid = victim.GetIdentity().GetPlainId();
		m_LastHitZone.Set(vuid, dmgZone);

		string weapon = ammo;
		if (source && source != attacker)
			weapon = source.GetDisplayName();
		m_LastHitWeapon.Set(vuid, weapon);
		m_LastHitDist.Set(vuid, vector.Distance(victim.GetPosition(), attacker.GetPosition()));
		m_LastHitTime.Set(vuid, GetGame().GetTickTime());
	}

	void OnPlayerKilled(PlayerBase victim, Object source)
	{
		if (Enabled != 1 || !m_Index || !victim || !source || !victim.GetIdentity())
			return;

		PlayerBase killer = PlayerBase.Cast(source);
		if (!killer)
		{
			EntityAI srcEnt = EntityAI.Cast(source);
			if (srcEnt && srcEnt.GetHierarchyRootPlayer())
				killer = PlayerBase.Cast(srcEnt.GetHierarchyRootPlayer());
		}

		if (!killer || killer == victim || !killer.GetIdentity())
			return;

		string kuid = killer.GetIdentity().GetPlainId();
		if (IsExempt(kuid))
			return;

		float hours = GetPlaytimeHours(kuid);
		if (hours >= MinPlaytimeHours)
			return; //players with real playtime never flag

		string vuid = victim.GetIdentity().GetPlainId();
		string zone = "";
		string weapon = "";
		float dist = vector.Distance(killer.GetPosition(), victim.GetPosition());

		//use the recorded hit only if it's recent; otherwise it belongs to an older
		//fight and would mis-attribute the zone/weapon/distance to this death
		float hitTime;
		if (m_LastHitTime.Find(vuid, hitTime) && (GetGame().GetTickTime() - hitTime) <= HIT_ATTRIBUTION_WINDOW)
		{
			m_LastHitZone.Find(vuid, zone);
			m_LastHitWeapon.Find(vuid, weapon);
			m_LastHitDist.Find(vuid, dist); //distance at impact, not at death
		}

		ClearHitData(vuid);

		//suspicious if: long range kill (any zone) OR head/brain kill (any range)
		bool longRange = dist >= LongRangeKillMeters;

		string zl = zone;
		zl.ToLower();
		bool headKill = zl.Contains("head") || zl.Contains("brain");

		if (!longRange && !headKill)
			return;

		string reason = "head/brain kill";
		if (longRange && headKill)
			reason = "long range head/brain kill";
		else if (longRange)
			reason = "long range kill";

		VPPATPlayerFlagProfile prof = GetOrCreateProfile(kuid, killer.GetIdentity().GetName());
		string details = string.Format("%1: killed \"%2\" (%3) at %4m with %5 (zone: %6) with only %7h server playtime",
			reason, victim.GetIdentity().GetName(), vuid, Math.Round(dist), weapon, zone, Math.Round(hours));

		RaiseFlag(prof, details);
		SaveDatabase();
	}

	//Human readable flag report for the "See Flags" button (also fills in the player's name)
	string BuildFlagReport(string uid, out string playerName)
	{
		VPPATPlayerFlagProfile prof;
		if (!m_Index || !m_Index.Find(uid, prof))
			return "No record for this player yet.";

		if (prof.name != string.Empty)
			playerName = prof.name;

		float hours = GetPlaytimeHours(uid);
		string report = string.Format("Steam64: %1\nFirst seen: %2\nTracked playtime: %3h\n", uid, prof.firstSeen, Math.Round(hours));

		if (IsExempt(uid))
			report += "Status: EXEMPT (admin or on the exempt list)\n";

		if (!prof.flags || prof.flags.Count() == 0)
		{
			report += "\nNo flags. Clean so far.";
			return report;
		}

		report += string.Format("\n--- %1 FLAG(S) ---\n", prof.flags.Count());

		//newest first, cap the dialog at the 10 most recent
		int shown = 0;
		for (int i = prof.flags.Count() - 1; i >= 0 && shown < 10; i--)
		{
			VPPATFlagEntry entry = prof.flags.Get(i);
			if (!entry)
				continue;

			report += string.Format("[%1]\n%2\n\n", entry.time, entry.details);
			shown++;
		}

		if (prof.flags.Count() > shown)
			report += string.Format("(+%1 older flag(s), see FlagDatabase.json)", prof.flags.Count() - shown);

		return report;
	}

	bool IsExempt(string uid)
	{
		if (ExemptIds && ExemptIds.Find(uid) != -1)
			return true;

		if (ExemptAdmins == 1 && GetPermissionManager())
		{
			if (GetPermissionManager().IsSuperAdmin(uid) || GetPermissionManager().HasUserGroup(uid))
				return true;
		}
		return false;
	}

	void RaiseFlag(VPPATPlayerFlagProfile prof, string details)
	{
		VPPATFlagEntry entry = new VPPATFlagEntry();
		entry.time    = NowString();
		entry.details = details;
		prof.flags.Insert(entry);
		while (prof.flags.Count() > 100)
			prof.flags.RemoveOrdered(0);

		GetSimpleLogger().Log(string.Format("[FlagSystem] FLAG on \"%1\" (steamid=%2): %3", prof.name, prof.uid, details));

		if (DiscordWebhookURL != string.Empty && GetWebHooksManager())
		{
			PlayerFlagMessage msg = new PlayerFlagMessage(prof.uid, prof.name, "SUSPICIOUS_KILL", details);
			GetWebHooksManager().GetConnectionManager().Post(DiscordWebhookURL, msg.BuildMessage(false));
		}

		if (NotifyOnlineAdmins == 1)
		{
			array<Man> players = new array<Man>;
			GetGame().GetWorld().GetPlayerList(players);
			foreach(Man man : players)
			{
				if (!man || !man.GetIdentity())
					continue;

				string aid = man.GetIdentity().GetPlainId();
				if (GetPermissionManager().IsSuperAdmin(aid) || GetPermissionManager().HasUserGroup(aid))
					GetPermissionManager().NotifyPlayer(aid, string.Format("[Flag] %1: suspicious kill (low playtime)", prof.name), NotifyTypes.NOTIFY);
			}
		}
	}
};

FlagSystemManager GetFlagSystem()
{
	return FlagSystemManager.Cast(GetPluginManager().GetPluginByType(FlagSystemManager));
};
