using UnrealBuildTool;

public class NineRealitiesNetcodeEditor : ModuleRules
{
	public NineRealitiesNetcodeEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PrecompileForTargets = PrecompileTargetsType.Editor;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"Slate",
			"SlateCore",
			"EditorSubsystem",
			"UnrealEd",
			"ToolMenus",
			"Projects"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"NineRealitiesNetcode"
		});
	}
}