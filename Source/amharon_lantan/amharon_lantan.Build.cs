using UnrealBuildTool;

public class amharon_lantan : ModuleRules
{
	public amharon_lantan(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "ClonerEffector", "LevelSequence", "MovieScene" });

		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.AddRange(new[] { "UnrealEd", "LevelSequenceEditor" });
		}
	}
}
