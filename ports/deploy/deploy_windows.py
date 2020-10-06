#!/usr/bin/env python
# -*- coding: utf-8 -*-

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

import math
import os
import platform
import subprocess # nosec
import sys
import threading
import zipfile

import deploy_base
import tools.binary_pecoff
import tools.qt5


class Deploy(deploy_base.DeployBase, tools.qt5.DeployToolsQt):
    def __init__(self):
        super().__init__()
        self.installDir = os.path.join(self.buildDir, 'ports/deploy/temp_priv')
        self.pkgsDir = os.path.join(self.buildDir, 'ports/deploy/packages_auto/windows')
        self.detectQt(os.path.join(self.buildDir, 'Manager'))
        self.programName = 'AkVirtualCamera'
        self.rootInstallDir = os.path.join(self.installDir, self.programName + '.plugin')
        self.binaryInstallDir = os.path.join(self.rootInstallDir, 'bin')
        self.mainBinary = os.path.join(self.binaryInstallDir, self.programName + '.exe')
        self.programName = os.path.splitext(os.path.basename(self.mainBinary))[0]
        self.programVersion = self.detectVersion(os.path.join(self.rootDir, 'commons.pri'))
        self.detectMake()
        self.binarySolver = tools.binary_pecoff.DeployToolsBinary()
        self.binarySolver.readExcludeList(os.path.join(self.rootDir, 'ports/deploy/exclude.{}.{}.txt'.format(os.name, sys.platform)))
        self.packageConfig = os.path.join(self.rootDir, 'ports/deploy/package_info.conf')
        self.dependencies = []
        self.installerConfig = os.path.join(self.installDir, 'installer/config')
        self.installerPackages = os.path.join(self.installDir, 'installer/packages')
        self.appIcon = os.path.join(self.rootDir, 'share/icons/webcamoid.png')
        self.licenseFile = os.path.join(self.rootDir, 'COPYING')
        self.installerScript = os.path.join(self.rootDir, 'ports/deploy/installscript.windows.qs')
        self.changeLog = os.path.join(self.rootDir, 'ChangeLog')

    def prepare(self):
        print('Executing make install')
        self.makeInstall(self.buildDir)
        self.detectTargetArch()

        if self.qtIFWVersion == '' or int(self.qtIFWVersion.split('.')[0]) < 3:
            appsDir = '@ApplicationsDir@'
        else:
            if self.targetArch == '32bit':
                appsDir = '@ApplicationsDirX86@'
            else:
                appsDir = '@ApplicationsDirX64@'

        self.installerTargetDir = appsDir + '/' + self.programName + '.plugin'
        arch = 'win32' if self.targetArch == '32bit' else 'win64'
        self.outPackage = os.path.join(self.pkgsDir,
                                       'webcamoid-{}-{}.exe'.format(self.programVersion,
                                                                    arch))

        print('Stripping symbols')
        self.binarySolver.stripSymbols(self.installDir)
        print('Removing unnecessary files')
        self.removeUnneededFiles(self.installDir)
        print('\nWritting build system information\n')
        self.writeBuildInfo()

    def removeDebugs(self):
        dbgFiles = set()

        for root, _, files in os.walk(self.libQtInstallDir):
            for f in files:
                if f.endswith('.dll'):
                    fname, ext = os.path.splitext(f)
                    dbgFile = os.path.join(root, '{}d{}'.format(fname, ext))

                    if os.path.exists(dbgFile):
                        dbgFiles.add(dbgFile)

        for f in dbgFiles:
            os.remove(f)

    @staticmethod
    def removeUnneededFiles(path):
        afiles = set()

        for root, _, files in os.walk(path):
            for f in files:
                if f.endswith('.a') \
                    or f.endswith('.static.prl') \
                    or f.endswith('.pdb') \
                    or f.endswith('.lib'):
                    afiles.add(os.path.join(root, f))

        for afile in afiles:
            os.remove(afile)

    def commitHash(self):
        try:
            process = subprocess.Popen(['git', 'rev-parse', 'HEAD'], # nosec
                                        stdout=subprocess.PIPE,
                                        stderr=subprocess.PIPE,
                                        cwd=self.rootDir)
            stdout, _ = process.communicate()

            if process.returncode != 0:
                return ''

            return stdout.decode(sys.getdefaultencoding()).strip()
        except:
            return ''

    @staticmethod
    def sysInfo():
        try:
            process = subprocess.Popen(['cmd', '/c', 'ver'], # nosec
                                        stdout=subprocess.PIPE,
                                        stderr=subprocess.PIPE)
            stdout, _ = process.communicate()
            windowsVersion = stdout.decode(sys.getdefaultencoding()).strip()

            return 'Windows Version: {}\n'.format(windowsVersion)
        except:
            pass

        return ' '.join(platform.uname())

    def writeBuildInfo(self):
        try:
            os.makedirs(self.pkgsDir)
        except:
            pass

        shareDir = os.path.join(self.rootInstallDir, 'share')

        try:
            os.makedirs(shareDir)
        except:
            pass

        depsInfoFile = os.path.join(shareDir, 'build-info.txt')

        # Write repository info.

        with open(depsInfoFile, 'w') as f:
            commitHash = self.commitHash()

            if len(commitHash) < 1:
                commitHash = 'Unknown'

            print('    Commit hash: ' + commitHash)
            f.write('Commit hash: ' + commitHash + '\n')

            buildLogUrl = ''

            if 'TRAVIS_BUILD_WEB_URL' in os.environ:
                buildLogUrl = os.environ['TRAVIS_BUILD_WEB_URL']
            elif 'APPVEYOR_ACCOUNT_NAME' in os.environ and 'APPVEYOR_PROJECT_NAME' in os.environ and 'APPVEYOR_JOB_ID' in os.environ:
                buildLogUrl = 'https://ci.appveyor.com/project/{}/{}/build/job/{}'.format(os.environ['APPVEYOR_ACCOUNT_NAME'],
                                                                                          os.environ['APPVEYOR_PROJECT_SLUG'],
                                                                                          os.environ['APPVEYOR_JOB_ID'])

            if len(buildLogUrl) > 0:
                print('    Build log URL: ' + buildLogUrl)
                f.write('Build log URL: ' + buildLogUrl + '\n')

            print()
            f.write('\n')

    @staticmethod
    def hrSize(size):
        i = int(math.log(size) // math.log(1024))

        if i < 1:
            return '{} B'.format(size)

        units = ['KiB', 'MiB', 'GiB', 'TiB']
        sizeKiB = size / (1024 ** i)

        return '{:.2f} {}'.format(sizeKiB, units[i - 1])

    def printPackageInfo(self, path):
        if os.path.exists(path):
            print('   ',
                  os.path.basename(path),
                  self.hrSize(os.path.getsize(path)))
            print('    sha256sum:', Deploy.sha256sum(path))
        else:
            print('   ',
                  os.path.basename(path),
                  'FAILED')

    def createAppInstaller(self, mutex):
        packagePath = self.createInstaller()

        if not packagePath:
            return

        mutex.acquire()
        print('Created installable package:')
        self.printPackageInfo(self.outPackage)
        mutex.release()

    def package(self):
        mutex = threading.Lock()
        threads = []
        packagingTools = []

        if self.qtIFW != '':
            threads.append(threading.Thread(target=self.createAppInstaller, args=(mutex,)))
            packagingTools += ['Qt Installer Framework']

        if len(packagingTools) > 0:
            print('Detected packaging tools: {}\n'.format(', '.join(packagingTools)))

        for thread in threads:
            thread.start()

        for thread in threads:
            thread.join()
