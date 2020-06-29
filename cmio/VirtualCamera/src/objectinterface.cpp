/* akvirtualcamera, virtual camera for Mac and Windows.
 * Copyright (C) 2020  Gonzalo Exequiel Pedone
 *
 * akvirtualcamera is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * akvirtualcamera is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with akvirtualcamera. If not, see <http://www.gnu.org/licenses/>.
 *
 * Web-Site: http://webcamoid.github.io/
 */

#include "objectinterface.h"
#include "utils.h"
#include "VCamUtils/src/logger/logger.h"

AkVCam::ObjectInterface::ObjectInterface():
    m_objectID(0),
    m_classID(0)
{

}

AkVCam::ObjectInterface::~ObjectInterface()
{

}

AkVCam::ObjectProperties AkVCam::ObjectInterface::properties() const
{
    return this->m_properties;
}

AkVCam::ObjectProperties &AkVCam::ObjectInterface::properties()
{
    return this->m_properties;
}

void AkVCam::ObjectInterface::setProperties(const ObjectProperties &properties)
{
    this->m_properties = properties;
}

void AkVCam::ObjectInterface::updateProperties(const ObjectProperties &properties)
{
    this->m_properties.update(properties);
}

CMIOObjectPropertyAddress AkVCam::ObjectInterface::address(CMIOObjectPropertySelector selector,
                                                           CMIOObjectPropertyScope scope,
                                                           CMIOObjectPropertyElement element)
{
    return CMIOObjectPropertyAddress {selector, scope, element};
}

void AkVCam::ObjectInterface::show()
{
    AkLogFunction();
    AkLogDebug() << "STUB" << std::endl;
}

Boolean AkVCam::ObjectInterface::hasProperty(const CMIOObjectPropertyAddress *address)
{
    AkLogFunction();

    if (!this->m_properties.getProperty(address->mSelector)) {
        AkLogWarning() << "Unknown property "
                       << enumToString(address->mSelector)
                       << std::endl;

        return false;
    }

    AkLogInfo() << "Found property "
                << enumToString(address->mSelector)
                << std::endl;

    return true;
}

OSStatus AkVCam::ObjectInterface::isPropertySettable(const CMIOObjectPropertyAddress *address,
                                                     Boolean *isSettable)
{
    AkLogFunction();

    if (!this->m_properties.getProperty(address->mSelector)) {
        AkLogWarning() << "Unknown property "
                       << enumToString(address->mSelector)
                       << std::endl;

        return kCMIOHardwareUnknownPropertyError;
    }

    bool settable = this->m_properties.isSettable(address->mSelector);

    if (isSettable)
        *isSettable = settable;

    AkLogInfo() << "Is property "
                << enumToString(address->mSelector)
                << " settable? "
                << (settable? "YES": "NO")
                << std::endl;

    return kCMIOHardwareNoError;
}

OSStatus AkVCam::ObjectInterface::getPropertyDataSize(const CMIOObjectPropertyAddress *address,
                                                      UInt32 qualifierDataSize,
                                                      const void *qualifierData,
                                                      UInt32 *dataSize)
{
    AkLogFunction();
    AkLogInfo() << "Getting property size "
                << enumToString(address->mSelector)
                << std::endl;

    if (!this->m_properties.getProperty(address->mSelector,
                                        qualifierDataSize,
                                        qualifierData,
                                        0,
                                        dataSize)) {
        AkLogWarning() << "Unknown property "
                       << enumToString(address->mSelector)
                       << std::endl;

        return kCMIOHardwareUnknownPropertyError;
    }

    return kCMIOHardwareNoError;
}

OSStatus AkVCam::ObjectInterface::getPropertyData(const CMIOObjectPropertyAddress *address,
                                                  UInt32 qualifierDataSize,
                                                  const void *qualifierData,
                                                  UInt32 dataSize,
                                                  UInt32 *dataUsed,
                                                  void *data)
{
    AkLogFunction();
    AkLogInfo() << "Getting property"
                << enumToString(address->mSelector)
                << std::endl;

    if (!this->m_properties.getProperty(address->mSelector,
                                        qualifierDataSize,
                                        qualifierData,
                                        dataSize,
                                        dataUsed,
                                        data)) {
        AkLogWarning() << "Unknown property "
                       << enumToString(address->mSelector)
                       << std::endl;

        return kCMIOHardwareUnknownPropertyError;
    }

    return kCMIOHardwareNoError;
}

OSStatus AkVCam::ObjectInterface::setPropertyData(const CMIOObjectPropertyAddress *address,
                                                  UInt32 qualifierDataSize,
                                                  const void *qualifierData,
                                                  UInt32 dataSize,
                                                  const void *data)
{
    AkLogFunction();
    AkLogInfo() << "Setting property "
                << enumToString(address->mSelector)
                << std::endl;
    UNUSED(qualifierDataSize)
    UNUSED(qualifierData)

    if (!this->m_properties.setProperty(address->mSelector,
                                        dataSize,
                                        data)) {
        AkLogWarning() << "Unknown property "
                       << enumToString(address->mSelector)
                       << std::endl;

        return kCMIOHardwareUnknownPropertyError;
    }

    return kCMIOHardwareNoError;
}
