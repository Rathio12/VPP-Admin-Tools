/*
	Spectate Tools submenu — pick a player, spectate in 1PP or 3PP.
	Thin UI over VPPSpectateClientHandler (the engine): the menu only selects
	targets, requests start/stop/switch, and mirrors live state when reopened.
*/
class MenuSpectateTools extends AdminHudSubMenu
{
	private bool m_Loaded;

	//header
	private TextWidget   m_TxtSubTitle;

	//players list
	private TextWidget   m_TxtPlayerCount;
	private EditBoxWidget m_SearchInputBox;
	private GridSpacerWidget m_ParentGrid;
	private ref array<ref SpectatePlayerEntry> m_Entries;
	private ref array<ref CustomGridSpacer>    m_DataGrids;
	private ref CustomGridSpacer               m_LastGrid;

	//control rail
	private TextWidget   m_TxtTargetName;
	private TextWidget   m_TxtTargetId;
	private TextWidget   m_TxtTargetSession;
	private ButtonWidget m_BtnCopyId;
	private ButtonWidget m_BtnFirstPerson;
	private ButtonWidget m_BtnThirdPerson;
	private TextWidget   m_TxtBtn1PP;
	private TextWidget   m_TxtBtn3PP;
	private ButtonWidget m_BtnSpectate;
	private TextWidget   m_TxtSpectate;
	private ButtonWidget m_BtnStopSpectate;
	private ImageWidget  m_ImgStatusDot;
	private TextWidget   m_TxtStatus;

	//death-grace preset dropdown
	private Widget                m_GraceDropHost;
	private ref VPPDropDownMenu   m_GraceDropDown;
	private ref array<int>        m_GraceValues;

	//state
	private string m_SelectedId;
	private string m_SelectedName;
	private int    m_SelectedSession;
	private float  m_ListRefreshTick;
	private string m_ListSignature;
	private string m_LastSearchText;

	void MenuSpectateTools()
	{
		m_Entries     = new array<ref SpectatePlayerEntry>;
		m_DataGrids   = new array<ref CustomGridSpacer>;
		m_GraceValues = {0, 5, 15, 30, 60}; //seconds; 0 = immediate exit
	}

	override void OnCreate(Widget RootW)
	{
		super.OnCreate(RootW);
		M_SUB_WIDGET  = CreateWidgets(VPPATUIConstants.MenuSpectateTools);
		M_SUB_WIDGET.SetHandler(this);
		m_TitlePanel  = Widget.Cast( M_SUB_WIDGET.FindAnyWidget( "Header") );
		m_closeButton = ButtonWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "BtnClose") );

		m_TxtSubTitle     = TextWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "TxtSubTitle") );
		m_TxtPlayerCount  = TextWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "TxtPlayerCount") );
		m_SearchInputBox  = EditBoxWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "SearchInputBox") );
		m_ParentGrid      = GridSpacerWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "ParentGrid") );

		m_TxtTargetName    = TextWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "TxtTargetName") );
		m_TxtTargetId      = TextWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "TxtTargetId") );
		m_TxtTargetSession = TextWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "TxtTargetSession") );
		m_BtnCopyId        = ButtonWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "BtnCopyId") );
		m_BtnFirstPerson   = ButtonWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "BtnFirstPerson") );
		m_BtnThirdPerson   = ButtonWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "BtnThirdPerson") );
		m_TxtBtn1PP        = TextWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "TxtBtn1PP") );
		m_TxtBtn3PP        = TextWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "TxtBtn3PP") );
		m_BtnSpectate      = ButtonWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "BtnSpectate") );
		m_TxtSpectate      = TextWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "TxtSpectate") );
		m_BtnStopSpectate  = ButtonWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "BtnStopSpectate") );
		m_ImgStatusDot     = ImageWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "ImgStatusDot") );
		m_TxtStatus        = TextWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "TxtStatus") );

		//death-grace preset dropdown (host declared LAST in the rail for z-order)
		m_GraceDropHost = Widget.Cast( M_SUB_WIDGET.FindAnyWidget( "GraceDropDownHost") );
		int savedGrace = GetSpectateClient().GetDeathGrace();
		m_GraceDropDown = new VPPDropDownMenu(m_GraceDropHost, GraceLabelFor(savedGrace));
		foreach (int graceVal : m_GraceValues)
		{
			m_GraceDropDown.AddElement(GraceLabelFor(graceVal));
		}
		m_GraceDropDown.m_OnSelectItem.Insert(OnSelectGrace);
		int savedIdx = m_GraceValues.Find(savedGrace);
		if (savedIdx == -1)
			savedIdx = m_GraceValues.Find(VPPSpectateConstants.DEATH_GRACE_DEFAULT);
		m_GraceDropDown.SetIndex(savedIdx);

		//engine events keep the rail live even while the menu is open
		GetSpectateClient().m_OnSpectateStarted.Insert(OnSpectateStartedEvt);
		GetSpectateClient().m_OnSpectateStopped.Insert(OnSpectateStoppedEvt);
		GetSpectateClient().m_OnPerspectiveChanged.Insert(OnPerspectiveChangedEvt);

		GetPlayerListManager().RequestForceSync();

		m_Loaded = true;
		RebuildList();
		UpdateViewButtons();
		RefreshControlRail();
	}

	override void OnAdminHudOpened()
	{
		super.OnAdminHudOpened();
		if (!m_Loaded)
			return;

		GetPlayerListManager().RequestForceSync();
		RefreshControlRail();
	}

	override void OnUpdate(float timeslice)
	{
		super.OnUpdate(timeslice);
		if (!IsSubMenuVisible() || !m_Loaded)
			return;

		//search change triggers a rebuild
		string searchText = m_SearchInputBox.GetText();
		if (searchText != m_LastSearchText)
		{
			m_LastSearchText = searchText;
			RebuildList();
		}

		//1s membership diff against the live player list
		m_ListRefreshTick += timeslice;
		if (m_ListRefreshTick >= 1.0)
		{
			m_ListRefreshTick = 0.0;
			string sig = BuildListSignature();
			if (sig != m_ListSignature)
				RebuildList();
		}
	}

	//---------------------------------------------------------------------------------
	// List
	//---------------------------------------------------------------------------------
	protected string BuildListSignature()
	{
		string sig = "";
		array<ref VPPUser> users = GetPlayerListManager().GetUsers();
		foreach (VPPUser user : users)
		{
			sig = sig + user.GetUserId() + ";";
		}
		return sig;
	}

	protected void RebuildList()
	{
		m_Entries.Clear();
		m_DataGrids.Clear();
		m_LastGrid = new CustomGridSpacer(m_ParentGrid);
		m_DataGrids.Insert(m_LastGrid);

		string selfId = "";
		if (GetGame().GetPlayer() && GetGame().GetPlayer().GetIdentity())
			selfId = GetGame().GetPlayer().GetIdentity().GetPlainId();

		string filter = m_LastSearchText;
		filter.ToLower();

		bool selectionStillOnline = false;
		int total = 0;
		array<ref VPPUser> users = GetPlayerListManager().GetUsers();
		foreach (VPPUser user : users)
		{
			total++;
			if (user.GetUserId() == m_SelectedId)
				selectionStillOnline = true;

			if (filter != "")
			{
				string lowered = user.GetUserName();
				lowered.ToLower();
				if (!lowered.Contains(filter))
					continue;
			}

			if (m_Entries.Count() > 0 && (m_Entries.Count() % 100) == 0)
			{
				m_LastGrid = new CustomGridSpacer(m_ParentGrid);
				m_DataGrids.Insert(m_LastGrid);
			}

			bool isSelf = (user.GetUserId() == selfId);
			SpectatePlayerEntry entry = new SpectatePlayerEntry(m_LastGrid.GetGrid(), this, user.GetUserName(), user.GetUserId(), isSelf);
			m_LastGrid.AddWidget(entry.m_EntryBox);
			m_Entries.Insert(entry);

			if (user.GetUserId() == m_SelectedId)
			{
				entry.SetSelected(true);
				m_SelectedSession = user.GetSessionId();
			}
		}

		m_ListSignature = BuildListSignature();
		m_ParentGrid.Update();

		m_TxtPlayerCount.SetText(total.ToString());
		m_TxtSubTitle.SetText(total.ToString() + " " + Widget.TranslateString("#VSTR_SPECTATE_ONLINE_COUNT"));

		//selected player left: clear the stale selection so the rail can't act on it
		if (m_SelectedId != "" && !selectionStillOnline)
		{
			m_SelectedId      = "";
			m_SelectedName    = "";
			m_SelectedSession = 0;
			UpdateTargetCard();
			RefreshControlRail();
		}
	}

	//row body click
	void OnRowSelected(string userId)
	{
		m_SelectedId = userId;
		m_SelectedName = "";
		m_SelectedSession = 0;

		foreach (SpectatePlayerEntry entry : m_Entries)
		{
			bool match = (entry.GetUserId() == userId);
			entry.SetSelected(match);
			if (match)
				m_SelectedName = entry.GetUserName();
		}

		array<ref VPPUser> users = GetPlayerListManager().GetUsers();
		foreach (VPPUser user : users)
		{
			if (user.GetUserId() == userId)
			{
				m_SelectedSession = user.GetSessionId();
				break;
			}
		}

		UpdateTargetCard();
		RefreshControlRail();
	}

	//row eye button: select + start immediately
	void OnRowSpectate(string userId)
	{
		OnRowSelected(userId);
		StartSpectateFlow();
	}

	//---------------------------------------------------------------------------------
	// Actions
	//---------------------------------------------------------------------------------
	protected void StartSpectateFlow()
	{
		if (m_SelectedId == "")
			return;

		string selfId = "";
		if (GetGame().GetPlayer() && GetGame().GetPlayer().GetIdentity())
			selfId = GetGame().GetPlayer().GetIdentity().GetPlainId();

		if (m_SelectedId == selfId)
		{
			GetVPPUIManager().DisplayNotification("#VSTR_NOTIFY_SPECTATE_SELF");
			return;
		}

		GetSpectateClient().RequestSpectate(m_SelectedId);
		GetVPPUIManager().DisplayNotification("#VSTR_NOTIFY_SPECTATE_REQ");

		//auto-hide the toolbar so the camera view is unobstructed.
		//HideMenu quirk: if the Object Manager submenu is visible the first call
		//only hides THAT — call again while still showing.
		VPPAdminHud hud = GetToolbarMenu();
		if (hud && hud.IsShowing())
		{
			hud.HideMenu();
			if (hud.IsShowing())
				hud.HideMenu();
		}
	}

	protected void UpdateTargetCard()
	{
		if (m_SelectedId == "")
		{
			m_TxtTargetName.SetText(Widget.TranslateString("#VSTR_SPECTATE_NO_TARGET"));
			m_TxtTargetId.SetText("");
			m_TxtTargetSession.SetText("");
			return;
		}

		m_TxtTargetName.SetText(m_SelectedName);
		m_TxtTargetId.SetText(m_SelectedId);
		m_TxtTargetSession.SetText(Widget.TranslateString("#VSTR_SPECTATE_SESSION") + " " + m_SelectedSession.ToString());
	}

	protected void UpdateViewButtons()
	{
		if (GetSpectateClient().IsFirstPerson())
		{
			m_TxtBtn1PP.SetColor(ARGB(255, 232, 163, 61));   //accent = active
			m_TxtBtn3PP.SetColor(ARGB(255, 154, 160, 166));
		}
		else
		{
			m_TxtBtn1PP.SetColor(ARGB(255, 154, 160, 166));
			m_TxtBtn3PP.SetColor(ARGB(255, 232, 163, 61));
		}
	}

	void RefreshControlRail()
	{
		VPPSpectateClientHandler client = GetSpectateClient();

		if (client.IsSpectating())
		{
			m_ImgStatusDot.SetColor(ARGB(255, 76, 175, 80));

			string targetName = ResolveName(client.GetTargetId());
			string mode = "3PP";
			if (client.IsFirstPerson())
				mode = "1PP";
			m_TxtStatus.SetText(Widget.TranslateString("#VSTR_SPECTATE_STATUS_ACTIVE") + " " + targetName + " (" + mode + ")");

			m_BtnStopSpectate.Enable(true);

			//primary button becomes SWITCH when a different row is selected
			if (m_SelectedId != "" && m_SelectedId != client.GetTargetId())
			{
				m_TxtSpectate.SetText(Widget.TranslateString("#VSTR_SPECTATE_BTN_SWITCH"));
				m_BtnSpectate.Enable(true);
			}
			else
			{
				m_TxtSpectate.SetText(Widget.TranslateString("#VSTR_SPECTATE_BTN_SWITCH"));
				m_BtnSpectate.Enable(false);
			}
		}
		else
		{
			m_ImgStatusDot.SetColor(ARGB(255, 92, 97, 102));
			m_TxtStatus.SetText(Widget.TranslateString("#VSTR_SPECTATE_STATUS_IDLE"));
			m_BtnStopSpectate.Enable(false);

			m_TxtSpectate.SetText(Widget.TranslateString("#VSTR_SPECTATE_BTN_START"));

			string selfId = "";
			if (GetGame().GetPlayer() && GetGame().GetPlayer().GetIdentity())
				selfId = GetGame().GetPlayer().GetIdentity().GetPlainId();
			m_BtnSpectate.Enable(m_SelectedId != "" && m_SelectedId != selfId);
		}
	}

	protected string GraceLabelFor(int seconds)
	{
		if (seconds <= 0)
			return Widget.TranslateString("#VSTR_SPECTATE_GRACE_IMMEDIATE");
		return seconds.ToString() + "s";
	}

	//dropdown select-callback MUST manage index/text/close itself (VPPDropDownMenu contract)
	void OnSelectGrace(int index)
	{
		if (index < 0 || index >= m_GraceValues.Count())
			return;

		m_GraceDropDown.SetIndex(index);
		m_GraceDropDown.SetText(GraceLabelFor(m_GraceValues[index]));
		m_GraceDropDown.Close();

		GetSpectateClient().SetDeathGrace(m_GraceValues[index]);
	}

	protected string ResolveName(string userId)
	{
		array<ref VPPUser> users = GetPlayerListManager().GetUsers();
		foreach (VPPUser user : users)
		{
			if (user.GetUserId() == userId)
				return user.GetUserName();
		}
		return userId;
	}

	//---------------------------------------------------------------------------------
	// Engine events
	//---------------------------------------------------------------------------------
	void OnSpectateStartedEvt(string targetId)
	{
		if (m_Loaded)
			RefreshControlRail();
	}

	void OnSpectateStoppedEvt()
	{
		if (m_Loaded)
			RefreshControlRail();
	}

	void OnPerspectiveChangedEvt(bool firstPerson)
	{
		if (!m_Loaded)
			return;
		UpdateViewButtons();
		RefreshControlRail();
	}

	//---------------------------------------------------------------------------------
	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (w == m_BtnSpectate)
		{
			StartSpectateFlow();
			return true;
		}

		if (w == m_BtnStopSpectate)
		{
			GetSpectateClient().RequestExit();
			return true;
		}

		if (w == m_BtnFirstPerson)
		{
			GetSpectateClient().SetPerspective(true);
			return true;
		}

		if (w == m_BtnThirdPerson)
		{
			GetSpectateClient().SetPerspective(false);
			return true;
		}

		if (w == m_BtnCopyId)
		{
			if (m_SelectedId != "")
			{
				GetGame().CopyToClipboard(m_SelectedId);
				GetVPPUIManager().DisplayNotification("#VSTR_NOTIFY_SUCCESS_COPY_TOCLIPBOARD");
			}
			return true;
		}

		return super.OnClick(w, x, y, button);
	}
};
