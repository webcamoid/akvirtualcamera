#!/bin/bash

# Webcamoid, webcam capture application.
# Copyright (C) 2017  Gonzalo Exequiel Pedone
#
# Webcamoid is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Webcamoid is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Webcamoid. If not, see <http://www.gnu.org/licenses/>.
#
# Web-Site: http://webcamoid.github.io/

if [ "${TRAVIS_OS_NAME}" = linux ]; then
    EXEC='sudo ./root.x86_64/bin/arch-chroot root.x86_64'
fi

cd ports/deploy
git clone https://github.com/webcamoid/DeployTools.git
cd ../..

DEPLOYSCRIPT=deployscript.sh

if [ "${TRAVIS_OS_NAME}" = linux ]; then
    sudo mount --bind root.x86_64 root.x86_64
    sudo mount --bind $HOME root.x86_64/$HOME

    cat << EOF > ${DEPLOYSCRIPT}
#!/bin/sh

export LC_ALL=C
export HOME=$HOME
export PATH="$TRAVIS_BUILD_DIR/.local/bin:\$PATH"
export PYTHONPATH="\$PWD/ports/deploy/DeployTools"
export WINEPREFIX=/opt/.wine
cd $TRAVIS_BUILD_DIR
EOF

    if [ ! -z "${DAILY_BUILD}" ]; then
        cat << EOF >> ${DEPLOYSCRIPT}
export DAILY_BUILD=1
EOF
    fi

    cat << EOF >> ${DEPLOYSCRIPT}
python ports/deploy/deploy.py
EOF
    chmod +x ${DEPLOYSCRIPT}
    sudo cp -vf ${DEPLOYSCRIPT} root.x86_64/$HOME/

    ${EXEC} bash $HOME/${DEPLOYSCRIPT}
    sudo umount root.x86_64/$HOME
    sudo umount root.x86_64
elif [ "${TRAVIS_OS_NAME}" = osx ]; then
    ${EXEC} python3 ports/deploy/deploy.py
fi
