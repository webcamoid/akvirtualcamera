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

    // Remove old plugin
    if (installer.isInstaller()) {
        component.addOperation("ConsumeOutput",
                               "AkPluginSymlink",
                               "/usr/bin/readlink",
                               "/Library/CoreMediaIO/Plug-Ins/DAL/@Name@.plugin");

        if (installer.value("AkPluginSymlink") == "")
            component.addElevatedOperation("Execute",
                                           "rm",
                                           "-rf",
                                           "/Library/CoreMediaIO/Plug-Ins/DAL/@Name@.plugin");

        component.addElevatedOperation("Execute",
                                       "chmod",
                                       "+x",
                                       "@TargetDir@/@Name@.plugin/Contents/Resources/AkVCamAssistant");
        component.addElevatedOperation("Execute",
                                       "chmod",
                                       "+x",
                                       "@TargetDir@/@Name@.plugin/Contents/Resources/AkVCamManager");
    }

    // Create a symlink to the plugin.
    component.addElevatedOperation("Execute",
                                   "ln",
                                   "-s",
                                   "@TargetDir@/@Name@.plugin",
                                   "/Library/CoreMediaIO/Plug-Ins/DAL/@Name@.plugin",
                                   "UNDOEXECUTE",
                                   "rm",
                                   "-f",
                                   "/Library/CoreMediaIO/Plug-Ins/DAL/@Name@.plugin");

    // Set assistant daemon.
    let daemonPlist = "/Library/LaunchAgents/org.webcamoid.cmio.AkVCam.Assistant.plist"
    let plistContents = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                      + "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" "
                      + "\"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n"
                      + "<plist version=\"1.0\">\n"
                      + "    <dict>\n"
                      + "        <key>Label</key>\n"
                      + "        <string>org.webcamoid.cmio.AkVCam.Assistant</string>\n"
                      + "        <key>ProgramArguments</key>\n"
                      + "        <array>\n"
                      + "            <string>"
                      +                  installer.value("TargetDir")
                      +                  "/AkVirtualCamera.plugin/Contents/Resources/AkVCamAssistant"
                      +             "</string>\n"
                      + "            <string>--timeout</string>\n"
                      + "            <string>300.0</string>\n"
                      + "        </array>\n"
                      + "        <key>MachServices</key>\n"
                      + "        <dict>\n"
                      + "            <key>org.webcamoid.cmio.AkVCam.Assistant</key>\n"
                      + "            <true/>\n"
                      + "        </dict>\n"
                      + "        <key>StandardOutPath</key>\n"
                      + "        <string>/tmp/AkVCamAssistant.log</string>\n"
                      + "        <key>StandardErrorPath</key>\n"
                      + "        <string>/tmp/AkVCamAssistant.log</string>\n"
                      + "    </dict>\n"
                      + "</plist>\n";
    component.addElevatedOperation("Delete", daemonPlist);
    component.addElevatedOperation("AppendFile", daemonPlist, plistContents);

    // Load assistant daemon.
    component.addElevatedOperation("Execute",
                                   "launchctl", "load", "-w", daemonPlist,
                                   "UNDOEXECUTE",
                                   "launchctl", "unload", "-w", daemonPlist);

    if (installer.isUninstaller())
        component.addElevatedOperation("Delete", daemonPlist);

    if (installer.value("TargetDir") != "/Applications/" + installer.value("Name"))
        component.addElevatedOperation("Execute",
                                       "ln",
                                       "-s",
                                       "@TargetDir@",
                                       "/Applications/@Name@",
                                       "UNDOEXECUTE",
                                       "rm",
                                       "-f",
                                       "/Applications/@Name@");
}
