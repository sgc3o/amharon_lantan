using UnrealBuildTool;
using System.Collections.Generic;

public class amharon_lantanEditorTarget : TargetRules
{
	public amharon_lantanEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_4;
		bOverrideBuildEnvironment = true;

		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			GlobalDefinitions.Add("__has_feature(x)=0");
		}

		ExtraModuleNames.Add("amharon_lantan");
	}
}
