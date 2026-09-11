using UnrealBuildTool;

public class amharon_lantan : ModuleRules
{
	public amharon_lantan(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "ClonerEffector" });

		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.Add("UnrealEd");
		}
	}
}
