appName=AkVirtualCamera
path=$(realpath "$0")
targetDir=$(dirname "$path")

# Unregister app from packages database
pkgutil --forget org.webcamoidprj.${appName}

resourcesDir=${targetDir}/${appName}.plugin/Contents/Resources

# Remove virtual cameras
"${resourcesDir}/AkVCamManager" remove-devices
"${resourcesDir}/AkVCamManager" update

# Remove symlink
rm -f "/Library/CoreMediaIO/Plug-Ins/DAL/${appName}.plugin"

# Disable service
service=org.webcamoid.cmio.AkVCam.Assistant
daemonPlist=/Library/LaunchDaemons/${service}.plist
launchctl enable "system/${service}"
launchctl bootout system "${daemonPlist}"
rm -f "${daemonPlist}"

# Remove installed files
rm -rf "${targetDir}"
