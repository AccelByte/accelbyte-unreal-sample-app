// Copyright (c) 2018 - 2022 AccelByte Inc. All Rights Reserved.
// This is licensed software from AccelByte Inc, for limitations
// and restrictions contact your company contract manager.

using UnrealBuildTool;
using Tools.DotNETCommon;

public class AccelByteUe4SdkDemo : ModuleRules
{
	public AccelByteUe4SdkDemo(ReadOnlyTargetRules Target) : base(Target)
	{
#if UE_4_24_OR_LATER
		bLegacyPublicIncludePaths = false;
		DefaultBuildSettings = BuildSettingsVersion.V2;
#else
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
#endif
	
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core", 
				"CoreUObject", 
				"Engine", 
				"InputCore", 
				"AccelByteUe4Sdk", 
				"Json", 
				"JsonUtilities", 
				"OnlineSubsystem",
				"OnlineSubsystemUtils",
				"Http", 
				"WebSockets"
			});
			
        PrivateDependencyModuleNames.AddRange(new string[] {  });

        
        if (Target.Type != TargetType.Server)
        {
	        // Get Native Subsystem Name from Ini
	        string NativePlatformService = "";
	        DirectoryReference ProjectDir = Target.ProjectFile == null ? null : Target.ProjectFile.Directory;
	        ConfigHierarchy EngineIni = ConfigCache.ReadHierarchy(ConfigHierarchyType.Engine, ProjectDir, Target.Platform);
	        
	        if (EngineIni != null)
	        {
		        EngineIni.GetString("OnlineSubsystem", "NativePlatformService", out NativePlatformService);
	        }

	        if (Target.Platform == UnrealTargetPlatform.AllDesktop)
	        {
		        if (NativePlatformService == "Steam")
		        {
			        PrivateDependencyModuleNames.Add("OnlineSubsystemSteam");
		        }
	        }
	        else if (Target.Platform == UnrealTargetPlatform.XboxOne)
	        {
		        PrivateDependencyModuleNames.Add("OnlineSubsystemGDK");
	        }
	        else if (Target.Platform == UnrealTargetPlatform.PS4 || NativePlatformService == "PS4")
	        {
		        PrivateDependencyModuleNames.Add("OnlineSubsystemPS4");
	        }
	        else if (NativePlatformService == "PS5")
	        {
		        PrivateDependencyModuleNames.Add("OnlineSubsystemPS5");
	        }
	        else if (Target.Platform == UnrealTargetPlatform.Android)
	        {
		        PrivateDependencyModuleNames.Add("OnlineSubsystemGoogle");
		        PrivateDependencyModuleNames.Add("OnlineSubsystemGooglePlay");
	        }
	        else if (Target.Platform == UnrealTargetPlatform.IOS)
	        {
		        PrivateDependencyModuleNames.Add("OnlineSubsystemApple");
		        PrivateDependencyModuleNames.Add("OnlineSubsystemIOS");
	        }
	        else
	        {
		        PrivateDependencyModuleNames.Add("OnlineSubsystemNull");
	        }
        }
        else
        {
	        PrivateDependencyModuleNames.Add("OnlineSubsystemNull");
        }

        // Uncomment if you are using Slate UI
        // PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
    }
}
