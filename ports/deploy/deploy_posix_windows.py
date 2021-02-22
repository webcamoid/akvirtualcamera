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
import subprocess # nosec
import sys
import threading
import zipfile

from WebcamoidDeployTools import DTDeployBase
from WebcamoidDeployTools import DTQt5
from WebcamoidDeployTools import DTBinaryPecoff


class Deploy(DTDeployBase.DeployBase, DTQt5.Qt5Tools):
    def __init__(self):
        super().__init__()
        rootDir = os.path.normpath(os.path.join(os.path.dirname(os.path.realpath(__file__)), '../..'))
        self.setRootDir(rootDir)
        self.targetSystem = 'posix_windows'
        self.installDir = os.path.join(self.buildDir, 'ports/deploy/temp_priv')
        self.pkgsDir = os.path.join(self.buildDir, 'ports/deploy/packages_auto/windows')
        self.detectQtIFW()
        self.detectQtIFWVersion()
        self.programName = 'AkVirtualCamera'
        self.adminRights = True
        self.packageConfig = os.path.join(self.buildDir, 'package_info.conf')
        self.rootInstallDir = os.path.join(self.installDir, self.programName + '.plugin')
        self.binaryInstallDir = os.path.join(self.rootInstallDir, 'bin')
        self.mainBinary = os.path.join(self.binaryInstallDir, self.programName + '.exe')
        self.programName = os.path.splitext(os.path.basename(self.mainBinary))[0]
        self.detectMake()
        self.binarySolver = DTBinaryPecoff.PecoffBinaryTools()
        self.binarySolver.readExcludes(os.name, sys.platform)
        self.dependencies = []
        self.installerConfig = os.path.join(self.installDir, 'installer/config')
        self.installerPackages = os.path.join(self.installDir, 'installer/packages')
        self.licenseFile = os.path.join(self.rootDir, 'COPYING')
        self.installerScript = os.path.join(self.rootDir, 'ports/deploy/installscript.windows.qs')
        self.changeLog = os.path.join(self.rootDir, 'ChangeLog')
        self.targetArch = '64bit' if 'x86_64' in self.qtInstallBins else '32bit'

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

    def prepare(self):
        if self.targetArch == '32bit':
            self.binarySolver.sysBinsPath = ['/usr/i686-w64-mingw32/bin']
        else:
            self.binarySolver.sysBinsPath = ['/usr/x86_64-w64-mingw32/bin']

        self.binarySolver.detectStrip()

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
                                       '{}-{}-{}.exe'.format(self.programName,
                                                             self.programVersion(),
                                                             arch))

        print('Stripping symbols')
        self.binarySolver.stripSymbols(self.installDir)
        print('Removing unnecessary files')
        self.removeUnneededFiles(self.installDir)
        print('\nWritting build system information\n')
        self.writeBuildInfo()

    @staticmethod
    def sysInfo():
        info = ''

        for f in os.listdir('/etc'):
            if f.endswith('-release'):
                with open(os.path.join('/etc' , f)) as releaseFile:
                    info += releaseFile.read()

        return info

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
            commitHash = self.gitCommitHash(self.rootDir)

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

        # Write host info.

        info = self.sysInfo()

        with open(depsInfoFile, 'a') as f:
            for line in info.split('\n'):
                if len(line) > 0:
                    print('    ' + line)
                    f.write(line + '\n')

            print()
            f.write('\n')

        # Write Wine version and emulated system info.

        process = subprocess.Popen(['wine', '--version'], # nosec
                                   stdout=subprocess.PIPE,
                                   stderr=subprocess.PIPE)
        stdout, _ = process.communicate()
        wineVersion = stdout.decode(sys.getdefaultencoding()).strip()

        with open(depsInfoFile, 'a') as f:
            print('    Wine Version: {}'.format(wineVersion))
            f.write('Wine Version: {}\n'.format(wineVersion))

        process = subprocess.Popen(['wine', 'cmd', '/c', 'ver'], # nosec
                                   stdout=subprocess.PIPE,
                                   stderr=subprocess.PIPE)
        stdout, _ = process.communicate()
        fakeWindowsVersion = stdout.decode(sys.getdefaultencoding()).strip()

        if len(fakeWindowsVersion) < 1:
            fakeWindowsVersion = 'Unknown'

        with open(depsInfoFile, 'a') as f:
            print('    Windows Version: {}'.format(fakeWindowsVersion))
            f.write('Windows Version: {}\n'.format(fakeWindowsVersion))
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
