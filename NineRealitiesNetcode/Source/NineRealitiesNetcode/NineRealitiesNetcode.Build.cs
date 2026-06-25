using UnrealBuildTool;

public class NineRealitiesNetcode : ModuleRules
{
	public NineRealitiesNetcode(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PrecompileForTargets = PrecompileTargetsType.Any;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"Sockets",
			"Networking",
			"OnlineSubsystem",
			"OnlineSubsystemUtils",
			"GameplayTags",
			"Projects"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"CoreOnline"
		});

		// UE5.5+ optimized networking headers
		PublicSystemIncludePaths.AddRange(new string[]
		{
			"$(PluginDir)/Source/NineRealitiesNetcode/Public",
			"$(PluginDir)/Source/NineRealitiesNetcode/Public/Core",
			"$(PluginDir)/Source/NineRealitiesNetcode/Public/Pipeline",
			"$(PluginDir)/Source/NineRealitiesNetcode/Public/UE6"
		});

		// Enable stricter checks for production netcode
		bUseUnity = false;
		bEnableExceptions = true;

		// UE6 forward compatibility: use C++20 features available in UE5.5+
		CppStandard = CppStandardVersion.Cpp20;

		// Define version macros for conditional UE6 migration paths
		PublicDefinitions.Add("N1_NETCODE_VERSION_MAJOR=3");
		PublicDefinitions.Add("N1_NETCODE_VERSION_MINOR=0");
		PublicDefinitions.Add("N1_NETCODE_VERSION_PATCH=0");
		PublicDefinitions.Add("N1_UE5_5_OR_LATER=1");
		PublicDefinitions.Add("N1_UE6_READY=1");
	}
}