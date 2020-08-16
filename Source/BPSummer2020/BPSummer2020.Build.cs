// Fill out your copyright notice in the Description page of Project Settings.

using System;
using System.IO;
using UnrealBuildTool;

public class BPSummer2020 : ModuleRules
{
	public BPSummer2020(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "Paper2D" });
        PrivateDependencyModuleNames.AddRange(new string[] {  });

        var base_path = Path.Combine(ModuleDirectory, "../ggpo");
        var ggpo_includes = Path.Combine(base_path, "src/include");
        var ggpo_lib = Path.Combine(base_path, "build/lib/x64/Release/GGPO.lib");

        //Console.WriteLine(ggpo_includes);
        //Console.WriteLine(ggpo_lib);

        PublicIncludePaths.Add(ggpo_includes);
        PublicAdditionalLibraries.Add(ggpo_lib);

        // Uncomment if you are using Slate UI
        // PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

        // Uncomment if you are using online features
        // PrivateDependencyModuleNames.Add("OnlineSubsystem");

        // To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
    }
}
