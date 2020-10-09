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
        // Remove virtual cameras
        if (installer.isUninstaller()) {
            let managerPath =
                installer.value("TargetDir")
                + "/"
                + archs[i]
                + "/AkVCamManager.exe";
            component.addOperation("Execute",
                                   managerPath, "remove-devices");
            component.addElevatedOperation("Execute",
                                           managerPath, "update");
        }

        let assistantPath =
            installer.value("TargetDir")
            + "/"
            + archs[i]
            + "/AkVCamAssistant.exe";

        // Load assistant daemon.
        if (installer.fileExists(assistantPath))
            component.addElevatedOperation("Execute",
                                           assistantPath, "--install",
                                           "UNDOEXECUTE",
                                           assistantPath, "--uninstall");
    }
}
