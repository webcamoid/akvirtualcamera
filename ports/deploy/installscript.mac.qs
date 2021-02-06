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

    // Remove virtual cameras
    if (installer.isUninstaller()) {
        component.addOperation("Execute",
                               "@TargetDir@/@Name@.plugin/Contents/Resources/AkVCamManager",
                               "remove-devices");
        component.addOperation("Execute",
                               "@TargetDir@/@Name@.plugin/Contents/Resources/AkVCamManager",
                               "update");
    }

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
                                       "a+x",
                                       "@TargetDir@/@Name@.plugin/Contents/Resources/AkVCamAssistant");
        component.addElevatedOperation("Execute",
                                       "chmod",
                                       "a+x",
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
    let service = "org.webcamoid.cmio.AkVCam.Assistant"
    let daemonPlist = "/Library/LaunchDaemons/" + service + ".plist"
    let plistContents = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                      + "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" "
                      + "\"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n"
                      + "<plist version=\"1.0\">\n"
                      + "    <dict>\n"
                      + "        <key>Label</key>\n"
                      + "        <string>" + service + "</string>\n"
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
                      + "            <key>" + service + "</key>\n"
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
    if (installer.isUninstaller())
        component.addElevatedOperation("Execute",
                                       "launchctl", "enable", "system/" + service);

    component.addElevatedOperation("Execute",
                                   "launchctl", "bootstrap", "system", daemonPlist,
                                   "UNDOEXECUTE",
                                   "launchctl", "bootout", "system", daemonPlist);

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
