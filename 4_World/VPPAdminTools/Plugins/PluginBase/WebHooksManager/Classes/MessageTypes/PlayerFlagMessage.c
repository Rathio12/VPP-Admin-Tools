class PlayerFlagMessage: WebHookMessageBase
{
	private string m_uid;
	private string m_name;
	private string m_flagType;
	private string m_details;

	void PlayerFlagMessage(string uid, string name, string flagType, string details)
	{
		m_uid      = uid;
		m_name     = name;
		m_flagType = flagType;
		m_details  = details;
		AddEmbed();
		SetContent();
	}

	override void SetContent(string str = string.Empty)
	{
		content = string.Format("**Player Flagged:** [%1](<https://steamcommunity.com/profiles/%2>) **%3** %4", m_name, m_uid, m_flagType, m_details);
	}

	override WbEmbed AddEmbed()
	{
		WbEmbed embed = new WbEmbed("Player Flag Report:", 15158332, "", "", ""); //red
		WbField field;

		field = embed.AddField();
		field.SetName("Player:");
		field.SetValue(m_name);
		field.Inline(true);
		//---
		field = embed.AddField();
		field.SetName("Steam64ID:");
		field.SetValue(m_uid + " [Steam Profile](https://steamcommunity.com/profiles/" + m_uid + ")");
		field.Inline(true);
		//---
		field = embed.AddField();
		field.SetName("Flag:");
		field.SetValue(m_flagType);
		field.Inline(false);
		//---
		field = embed.AddField();
		field.SetName("Details:");
		field.SetValue(m_details);
		field.Inline(false);

		embeds.Insert( embed );
		return embed;
	}
};
