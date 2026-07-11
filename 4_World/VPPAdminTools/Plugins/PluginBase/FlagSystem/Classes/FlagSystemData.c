class VPPATFlagEntry
{
	string time;
	string details;
};

class VPPATPlayerFlagProfile
{
	string uid;
	string name;
	string firstSeen;
	float  playtimeSec;
	ref array<ref VPPATFlagEntry> flags;

	void VPPATPlayerFlagProfile()
	{
		flags = new array<ref VPPATFlagEntry>;
	}
};

class VPPATFlagDatabase
{
	ref array<ref VPPATPlayerFlagProfile> profiles;

	void VPPATFlagDatabase()
	{
		profiles = new array<ref VPPATPlayerFlagProfile>;
	}
};
