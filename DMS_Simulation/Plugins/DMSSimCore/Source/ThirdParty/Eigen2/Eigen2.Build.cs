using UnrealBuildTool;

public class Eigen2 : ModuleRules
{
    public Eigen2(ReadOnlyTargetRules Target) : base(Target)
    {
        Type = ModuleType.External;
		
		PublicIncludePaths.Add(ModuleDirectory);
        PublicIncludePaths.Add( ModuleDirectory + "/Eigen/" );
        PublicDefinitions.Add("EIGEN_MPL2_ONLY");
		ShadowVariableWarningLevel = WarningLevel.Off;
	}
}
