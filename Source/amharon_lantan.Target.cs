using UnrealBuildTool;
using System.Collections.Generic;

public class amharon_lantanTarget : TargetRules
{
	public amharon_lantanTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_4;

		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			GlobalDefinitions.Add("__has_feature(x)=0");
		}

		ExtraModuleNames.Add("amharon_lantan");
	}
}
