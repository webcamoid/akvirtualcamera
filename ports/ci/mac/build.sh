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

component=AkVirtualCameraSrc

rm -rf "${PWD}/akvirtualcamera-packages"
mkdir -p /tmp/akvirtualcamera-data
cp -rf . /tmp/akvirtualcamera-data/${component}
rm -rf /tmp/akvirtualcamera-data/${component}/.git
rm -rf /tmp/akvirtualcamera-data/${component}/DeployTools/.git

mkdir -p /tmp/installScripts
cat << EOF > /tmp/installScripts/postinstall
#!/bin/sh
set -ex

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

# Install the build dependencies for AkVirtualCamera

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
echo "Building AkVirtualCamera with Cmake"
echo

export MACOSX_DEPLOYMENT_TARGET="10.\$(sw_vers -productVersion | cut -d. -f1)"

INSTALL_PATH=/Applications/AkVirtualCamera
BUILD_PATH=/tmp/build-AkVirtualCamera-Release
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
echo "Packaging AkVirtualCamera"
echo

DT_PATH="/tmp/${component}/DeployTools"
export PYTHONPATH="\${DT_PATH}"

# Only solve the dependencies, do not package into another pkg

python3 "\${DT_PATH}/deploy.py" \
    -r \
    -d "\${INSTALL_PATH}" \
    -c "\${BUILD_PATH}/package_info.conf"

echo "Removing the old plugin"
sudo rm -rf "/Library/CoreMediaIO/Plug-Ins/DAL/AkVirtualCamera.plugin"

resourcesDir="\${INSTALL_PATH}/AkVirtualCamera.plugin/Contents/Resources"

echo "Reseting binary permissions"
chmod a+x "\${resourcesDir}/AkVCamAssistant"
chmod a+x "\${resourcesDir}/AkVCamManager"

echo "Creating a symlink to the plugin"
sudo ln -s "\${INSTALL_PATH}/AkVirtualCamera.plugin" "/Library/CoreMediaIO/Plug-Ins/DAL/AkVirtualCamera.plugin"

echo "Writing the uninstaller"

cat << UNINSTALLER_EOF > "\${INSTALL_PATH}/uninstall.sh"
#!/bin/sh
set -ex

appName=AkVirtualCamera

case "\$*" in
    *--no-gui*)
        if [ "\${EUID}" -ne 0 ]; then
            echo "The uninstall script must be run as root" 1>&2

            exit 1
        fi
        ;;
    *)
        answer=\$(osascript -e "button returned of (display dialog \"Uninstall \${appName}?\" with icon caution buttons {\"Yes\", \"No\"} default button 2)")

        if [ "\${answer}" = "No" ]; then
            echo "Uninstall not executed" 1>&2

            exit 1
        fi

        if [ "\${EUID}" -ne 0 ]; then
            osascript -e "do shell script \"\$0 --no-gui\" with administrator privileges"

            exit \$?
        fi
        ;;
esac

path=\$(realpath "\$0")
targetDir=\$(dirname "\${path}")

# Unregister app from packages database
pkgutil --forget io.github.webcamoid.\${appName}

resourcesDir=\${targetDir}/\${appName}.plugin/Contents/Resources

# Remove virtual cameras
"\${resourcesDir}/AkVCamManager" remove-devices
"\${resourcesDir}/AkVCamManager" update

# Remove symlink
rm -f "/Library/CoreMediaIO/Plug-Ins/DAL/\${appName}.plugin"

# Remove installed files
rm -rf "\${targetDir}"

# Ending message
endMessage="\${appName} successfully uninstalled"

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

echo
echo "AkVirtualCamera is ready to use at \${INSTALL_PATH}"
EOF

verMaj=$(grep commons.cmake | awk '{print $2}' | tr -d ')' | head -n 1)
verMin=$(grep commons.cmake | awk '{print $2}' | tr -d ')' | head -n 1)
verPat=$(grep commons.cmake | awk '{print $2}' | tr -d ')' | head -n 1)
releaseVer=${verMaj}.${verMin}.${verPat}

cat << EOF > /tmp/package_info.conf
[Package]
name = akvirtualcamera
identifier = io.github.webcamoid.AkVirtualCamera
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
name = akvirtualcamera-installer
appName = AkVirtualCamera
targetDir = /tmp
installScript = /tmp/installScripts/postinstall
hideArch = true
EOF

chmod +x /tmp/installScripts/postinstall

PACKAGES_DIR="${PWD}/akvirtualcamera-packages/mac"

python3 DeployTools/deploy.py \
    -d "/tmp/akvirtualcamera-data" \
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
/Applications/AkVirtualCamera/AkVirtualCamera.plugin/Contents/Resources/AkVCamManager --version

echo
echo "Test uninstall"
echo
sudo /Applications/AkVirtualCamera/uninstall.sh --no-gui
