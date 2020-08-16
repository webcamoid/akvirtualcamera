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
import shutil
import subprocess # nosec
import sys
import threading
import time

import deploy_base
import tools.binary_mach
import tools.qt5


class Deploy(deploy_base.DeployBase, tools.qt5.DeployToolsQt):
    def __init__(self):
        super().__init__()
        self.installDir = os.path.join(self.buildDir, 'ports/deploy/temp_priv')
        self.pkgsDir = os.path.join(self.buildDir, 'ports/deploy/packages_auto', self.targetSystem)
        self.detectQt(os.path.join(self.buildDir, 'Manager'))
        self.programName = 'AkVirtualCamera'
        self.rootInstallDir = os.path.join(self.installDir, 'Applications')
        self.appBundleDir = os.path.join(self.rootInstallDir, self.programName + '.plugin')
        self.execPrefixDir = os.path.join(self.appBundleDir, 'Contents')
        self.binaryInstallDir = os.path.join(self.execPrefixDir, 'MacOS')
        self.mainBinary = os.path.join(self.binaryInstallDir, self.programName)
        self.programVersion = self.detectVersion(os.path.join(self.rootDir, 'commons.pri'))
        self.detectMake()
        self.binarySolver = tools.binary_mach.DeployToolsBinary()
        self.binarySolver.readExcludeList(os.path.join(self.rootDir, 'ports/deploy/exclude.{}.{}.txt'.format(os.name, sys.platform)))
        self.packageConfig = os.path.join(self.rootDir, 'ports/deploy/package_info.conf')
        self.dependencies = []
        self.installerConfig = os.path.join(self.installDir, 'installer/config')
        self.installerPackages = os.path.join(self.installDir, 'installer/packages')
        self.appIcon = os.path.join(self.rootDir, 'share/TestFrame/webcamoid.png')
        self.licenseFile = os.path.join(self.rootDir, 'COPYING')
        self.installerTargetDir = '@ApplicationsDir@/' + self.programName
        self.installerScript = os.path.join(self.rootDir, 'ports/deploy/installscript.mac.qs')
        self.changeLog = os.path.join(self.rootDir, 'ChangeLog')
        self.outPackage = os.path.join(self.pkgsDir,
                                   '{}-{}.dmg'.format(self.programName,
                                                      self.programVersion))

    def prepare(self):
        print('Executing make install')
        self.makeInstall(self.buildDir, self.installDir)
        self.detectTargetArch()
        print('Stripping symbols')
        self.binarySolver.stripSymbols(self.installDir)
        print('Resetting file permissions')
        self.binarySolver.resetFilePermissions(self.rootInstallDir,
                                               self.binaryInstallDir)
        print('\nWritting build system information\n')
        self.writeBuildInfo()
        print('\nSigning bundle\n')
        self.signPackage(self.appBundleDir)

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
        process = subprocess.Popen(['sw_vers'], # nosec
                                    stdout=subprocess.PIPE,
                                    stderr=subprocess.PIPE)
        stdout, _ = process.communicate()

        return stdout.decode(sys.getdefaultencoding()).strip()

    def writeBuildInfo(self):
        try:
            os.makedirs(self.pkgsDir)
        except:
            pass

        resourcesDir = os.path.join(self.execPrefixDir, 'Resources')

        try:
            os.makedirs(resourcesDir)
        except:
            pass

        depsInfoFile = os.path.join(resourcesDir, 'build-info.txt')

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

        # Write host info.

        info = self.sysInfo()

        with open(depsInfoFile, 'a') as f:
            for line in info.split('\n'):
                if len(line) > 0:
                    print('    ' + line)
                    f.write(line + '\n')

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

    @staticmethod
    def dirSize(path):
        size = 0

        for root, _, files in os.walk(path):
            for f in files:
                fpath = os.path.join(root, f)

                if not os.path.islink(fpath):
                    size += os.path.getsize(fpath)

        return size
    
    def signPackage(self, package):
        process = subprocess.Popen(['codesign', # nosec
                                    '--force',
                                    '--sign',
                                    '-',
                                    package],
                                    stdout=subprocess.PIPE,
                                    stderr=subprocess.PIPE)
        process.communicate()

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
