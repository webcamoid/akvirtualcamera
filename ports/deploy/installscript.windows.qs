function Component()
{
}

Component.prototype.beginInstallation = function()
{
    component.beginInstallation();
}

Component.prototype.createOperations = function()
{
    component.createOperations();
    let archs = ["x86", "x64"];

    for (let i in archs) {
        let assistantPath =
            installer.value("TargetDir")
            + "/"
            + archs[i]
            + "/AkVCamAssistant.exe";

        if (!installer.fileExists(assistantPath))
            continue;

        // Load assistant daemon.
        component.addElevatedOperation("Execute",
                                       assistantPath, "--install",
                                       "UNDOEXECUTE",
                                       assistantPath, "--uninstall");
    }
}
