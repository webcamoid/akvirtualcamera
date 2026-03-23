#!/bin/sh

set -e

if [ ! -z "${GITHUB_SHA}" ]; then
    export GIT_COMMIT_HASH="${GITHUB_SHA}"
elif [ ! -z "${CIRRUS_CHANGE_IN_REPO}" ]; then
    export GIT_COMMIT_HASH="${CIRRUS_CHANGE_IN_REPO}"
fi

export GIT_BRANCH_NAME=$(git rev-parse --abbrev-ref HEAD 2>/dev/null)

if [ -z "${GIT_BRANCH_NAME}" ]; then
    if [ ! -z "${GITHUB_REF_NAME}" ]; then
        export GIT_BRANCH_NAME="${GITHUB_REF_NAME}"
    elif [ ! -z "${CIRRUS_BRANCH}" ]; then
        export GIT_BRANCH_NAME="${CIRRUS_BRANCH}"
    else
        export GIT_BRANCH_NAME=master
    fi
fi

brew update
brew upgrade
brew install makeself

# Distribute a static version of DeployTools with akvirtualcamera source code

git clone https://github.com/webcamoid/DeployTools.git

appName=AkVirtualCamera
appNameLower=$(echo "${appName}" | tr '[:upper:]' '[:lower:]')
component=${appName}Src

rm -rf "${PWD}/packages"
mkdir -p /tmp/${appNameLower}-data
cp -rf . /tmp/${appNameLower}-data/${component}
rm -rf /tmp/${appNameLower}-data/${component}/.git
rm -rf /tmp/${appNameLower}-data/${component}/DeployTools/.git

mkdir -p /tmp/installScripts
cat << EOF > /tmp/installScripts/postinstall
#!/bin/sh

# Install XCode command line tools and homebrew

if [ ! -d /Applications/Xcode.app ]; then
    echo "Installing XCode command line tools"
    echo
    xcode-select --install
    echo
fi

if ! command -v brew >/dev/null 2>&1; then
    echo "Installing Homebrew"
    echo
    /bin/bash -c "\$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
    echo
fi

# Install the build dependencies for ${appName}

echo "Updating Homebrew database"
echo
brew update
brew upgrade
echo
echo "Installing dependencies"
echo
brew install \
    ccache \
    cmake \
    p7zip \
    pkg-config \
    python

echo
echo "Building ${appName} with Cmake"
echo

export MACOSX_DEPLOYMENT_TARGET="\$(sw_vers -productVersion | cut -d. -f1-2)"

INSTALL_PATH=/Applications/${appName}
BUILD_PATH=/tmp/build-${appName}-Release
mkdir -p "\${BUILD_PATH}"
cmake \
    -S /tmp/${component} \
    -B "\${BUILD_PATH}" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="\${INSTALL_PATH}" \
    -DGIT_COMMIT_HASH="${GIT_COMMIT_HASH}" \
    -DCMAKE_C_COMPILER_LAUNCHER=ccache \
    -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
    -DCMAKE_OBJCXX_COMPILER_LAUNCHER=ccache \
    -DDAILY_BUILD=${DAILY_BUILD}
cmake --build "\${BUILD_PATH}" --parallel \$(sysctl -n hw.ncpu)
cmake --install "\${BUILD_PATH}"

echo
echo "Packaging ${appName}"
echo

DT_PATH="/tmp/${component}/DeployTools"
export PYTHONPATH="\${DT_PATH}"

# Only solve the dependencies, do not package into another pkg

python3 "\${DT_PATH}/deploy.py" \
    -r \
    -d "\${INSTALL_PATH}" \
    -c "\${BUILD_PATH}/package_info.conf"

echo "Removing the old plugin"
sudo rm -rf "/Library/CoreMediaIO/Plug-Ins/DAL/${appName}.plugin"

resourcesDir="\${INSTALL_PATH}/${appName}.plugin/Contents/Resources"

echo "Reseting binary permissions"
chmod a+x "\${resourcesDir}/AkVCamAssistant"
chmod a+x "\${resourcesDir}/AkVCamManager"

echo "Creating a symlink to the plugin"
sudo ln -s "\${INSTALL_PATH}/${appName}.plugin" "/Library/CoreMediaIO/Plug-Ins/DAL/${appName}.plugin"

# Configure the CMIO extension

MIN_VERSION="12.3"

version_gte() {
    awk -v v1="\$1" -v v2="\$2" 'BEGIN {
        split(v1, a, ".")
        split(v2, b, ".")

        for (i = 1; i <= 3; i++) {
            if (a[i]+0 > b[i]+0) {
                print 1

                exit
            }

            if (a[i]+0 < b[i]+0) {
                print 0

                exit
            }
        }

        print 1
    }'
}

CURRENT="\$(sw_vers -productVersion)"
cmioExtensionSupported=false

if [ "\$(version_gte "\$CURRENT" "\$MIN_VERSION")" = "1" ]; then
    cmioExtensionSupported=true
fi

developerMode=disabled

if systemextensionsctl developer | grep -q "enabled"; then
    developerMode=enabled
fi

showExtensionMessage=false

if [ "\${cmioExtensionSupported}" = "true" ]; then
    if [ "\${developerMode}" = "enabled" ]; then
        systemextensionsctl install \${INSTALL_PATH}/${appName}CX.app/Contents/Library/SystemExtensions/io.github.webcamoid.${appName}CX.Extension.systemextension
    else
        showExtensionMessage=true
    fi
fi

echo "Writing the uninstaller"

cat << 'UNINSTALLER_EOF' > "\${INSTALL_PATH}/uninstall.sh"
#!/bin/sh

case "\$*" in
    *--no-gui*)
        if [ "\$(id -u)" -ne 0 ]; then
            echo "The uninstall script must be run as root" 1>&2

            exit 1
        fi
        ;;
    *)
        answer=\$(osascript -e "button returned of (display dialog \"Uninstall ${appName}?\" with icon caution buttons {\"Yes\", \"No\"} default button 2)")

        if [ "\${answer}" = "No" ]; then
            echo "Uninstall not executed" 1>&2

            exit 1
        fi

        if [ "\$(id -u)" -ne 0 ]; then
            osascript -e "do shell script \"\$0 --no-gui\" with administrator privileges"

            exit \$?
        fi
        ;;
esac

path=\$(realpath "\$0")
targetDir=\$(dirname "\${path}")

resourcesDir=\${targetDir}/${appName}.plugin/Contents/Resources

# Remove virtual cameras
"\${resourcesDir}/AkVCamManager" remove-devices
"\${resourcesDir}/AkVCamManager" update

# Remove symlink
rm -f "/Library/CoreMediaIO/Plug-Ins/DAL/${appName}.plugin"

# Remove CMIO extension if installed

MIN_VERSION="12.3"

version_gte() {
    awk -v v1="\$1" -v v2="\$2" 'BEGIN {
        split(v1, a, ".")
        split(v2, b, ".")

        for (i = 1; i <= 3; i++) {
            if (a[i]+0 > b[i]+0) {
                print 1

                exit
            }

            if (a[i]+0 < b[i]+0) {
                print 0

                exit
            }
        }

        print 1
    }'
}

CURRENT="\$(sw_vers -productVersion)"

if [ "\$(version_gte "\$CURRENT" "\$MIN_VERSION")" = "1" ]; then
    extensionBundle="io.github.webcamoid.${appName}CX.Extension"
    extensionTeamId="ABCDEF1234"  # Reemplazar con el Team ID real

    if systemextensionsctl list | grep -q "\${extensionBundle}"; then
        systemextensionsctl uninstall "\${extensionTeamId}" "\${extensionBundle}"
    fi
fi

# Remove installed files
rm -rf "\${targetDir}"

# Ending message
endMessage="${appName} successfully uninstalled"

case "\$*" in
    *--no-gui*)
        echo "\${endMessage}"
        ;;
    *)
        osascript -e "display dialog \"\${endMessage}\" buttons {\"Ok\"} default button 1"
        ;;
esac
UNINSTALLER_EOF

chmod +x "\${INSTALL_PATH}/uninstall.sh"

if [ "\${showExtensionMessage}" = "true" ]; then
    echo
    echo "The CMIO extension can't be installed because the developer mode is disbled."
    echo "For enabling the developer mode do as follow:"
    echo
    echo "1. Restart in Recovery Mode as follow:"
    echo "    Apple Silicon: Keep pressed the powr button until it shows 'Loading startup options'"
    echo "    Intel: Press Command + R on start"
    echo "2. Open the Terminal from the Utilities menu."
    echo "3. Run the command: 'systemextensionsctl developer on'"
    echo "4. Restart normally."
    echo "5. Then install the extension with: 'systemextensionsctl install \${INSTALL_PATH}/${appName}CX.app/Contents/Library/SystemExtensions/io.github.webcamoid.${appName}CX.Extension.systemextension'"
fi

echo
echo "${appName} is ready to use at \${INSTALL_PATH}"
EOF

verMaj=$(grep VER_MAJ commons.cmake | awk '{print $2}' | tr -d ')' | head -n 1)
verMin=$(grep VER_MIN commons.cmake | awk '{print $2}' | tr -d ')' | head -n 1)
verPat=$(grep VER_PAT commons.cmake | awk '{print $2}' | tr -d ')' | head -n 1)
releaseVer=${verMaj}.${verMin}.${verPat}

cat << EOF > /tmp/package_info.conf
[Package]
name = ${appNameLower}
identifier = io.github.webcamoid.${appName}
targetPlatform = mac
sourcesDir = ${PWD}
targetArch = any
version = ${releaseVer}
outputFormats = Makeself
dailyBuild = ${DAILY_BUILD}
buildType = Release
writeBuildInfo = false

[Git]
hideCommitCount = true

[Makeself]
name = ${appNameLower}-installer
appName = ${appName}
targetDir = /tmp
installScript = /tmp/installScripts/postinstall
hideArch = true
EOF

chmod +x /tmp/installScripts/postinstall

PACKAGES_DIR="${PWD}/packages/mac"

python3 DeployTools/deploy.py \
    -d "/tmp/${appNameLower}-data" \
    -c "/tmp/package_info.conf" \
    -o "${PACKAGES_DIR}"

echo
echo "Testing the package install"
echo

chmod +x "${PACKAGES_DIR}"/*.run
"${PACKAGES_DIR}"/*.run --accept

echo
echo "Test AkVCamManager"
echo
/Applications/${appNameLower}/${appNameLower}.plugin/Contents/Resources/AkVCamManager --version

echo
echo "Test uninstall"
echo
sudo /Applications/${appNameLower}/uninstall.sh --no-gui
