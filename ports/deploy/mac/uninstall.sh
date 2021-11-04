appName=AkVirtualCamera

if [[ "$*" == *--no-gui* ]]; then
    if [ "$EUID" -ne 0 ]; then
        echo "The uninstall script must be run as root" 1>&2

        exit -1
    fi
else
    answer=$(osascript -e "button returned of (display dialog \"Uninstall ${appName}?\" with icon caution buttons {\"Yes\", \"No\"} default button 2)")

    if [ "$answer" == No ]; then
        echo "Uninstall not executed" 1>&2

        exit -1
    fi

    if [ "$EUID" -ne 0 ]; then
        osascript -e "do shell script \"$0 --no-gui\" with administrator privileges"

        exit $?
    fi
fi

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

# Ending message
endMessage="${appName} successfully uninstalled"

if [[ "$*" == *--no-gui* ]]; then
    echo "${endMessage}"
else
    osascript -e "display dialog \"${endMessage}\" buttons {\"Ok\"} default button 1"
fi
