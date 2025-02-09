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

#include <iostream>
#include <fstream>
#include <sstream>
#include <Foundation/Foundation.h>

#include "hacks.h"
#include "PlatformUtils/src/utils.h"

int AkVCam::disableLibraryValidation(const std::vector<std::string> &args)
{
    if (args.size() < 1) {
        std::cerr << "Not enough arguments." << std::endl;

        return -1;
    }

    if (!fileExists(args[0])) {
        std::cerr << "No such file or directory." << std::endl;

        return -1;
    }

    static const std::string entitlementsXml = "/tmp/entitlements.xml";
    static const std::string dlv =
            "com.apple.security.cs.disable-library-validation";

    if (readEntitlements(args[0], entitlementsXml)) {
        auto nsdlv = [NSString
                     stringWithCString: dlv.c_str()
                     encoding: [NSString defaultCStringEncoding]];
        auto nsEntitlementsXml = [NSString
                                 stringWithCString: entitlementsXml.c_str()
                                 encoding: [NSString defaultCStringEncoding]];
        auto nsEntitlementsXmlUrl = [NSURL
                                    fileURLWithPath: nsEntitlementsXml];
        NSError *error = nil;
        auto entitlements =
                [[NSXMLDocument alloc]
                initWithContentsOfURL: nsEntitlementsXmlUrl
                options: 0
                error: &error];
        [nsEntitlementsXmlUrl release];
        const std::string xpath =
                "/plist/dict/key[contains(text(), \"" + dlv + "\")]";
        auto nsxpath = [NSString
                       stringWithCString: xpath.c_str()
                       encoding: [NSString defaultCStringEncoding]];
        auto nodes = [entitlements nodesForXPath: nsxpath error: &error];
        [nsxpath release];

        if ([nodes count] < 1) {
            auto key = [[NSXMLElement alloc] initWithName: @"key" stringValue: nsdlv];
            [(NSXMLElement *) entitlements.rootElement.children[0] addChild: key];
            [key release];
            auto value = [[NSXMLElement alloc] initWithName: @"true"];
            [(NSXMLElement *) entitlements.rootElement.children[0] addChild: value];
            [value release];
        } else {
            for (NSXMLNode *node: nodes) {
                auto value = [[NSXMLElement alloc] initWithName: @"true"];
                [(NSXMLElement *) node.parent replaceChildAtIndex: node.nextSibling.index withNode: value];
                [value release];
            }
        }

        auto data = [entitlements XMLDataWithOptions: NSXMLNodePrettyPrint
                    | NSXMLNodeCompactEmptyElement];
        [data writeToFile: nsEntitlementsXml atomically: YES];
        [data release];
        [entitlements release];
        [nsEntitlementsXml release];
        [nsdlv release];
    } else {
        std::ofstream entitlements(entitlementsXml);

        if (entitlements.is_open()) {
            entitlements << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << std::endl;
            entitlements << "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">" << std::endl;
            entitlements << "<plist version=\"1.0\">" << std::endl;
            entitlements << "\t<dict>" << std::endl;
            entitlements << "\t\t<key>" << dlv << "</key>" << std::endl;
            entitlements << "\t\t<true/>" << std::endl;
            entitlements << "\t</dict>" << std::endl;
            entitlements << "</plist>" << std::endl;
            entitlements.close();
        }
    }

    auto cmd = "codesign --entitlements \""
               + entitlementsXml
               + "\" -f -s - \""
               + args[0]
               + "\"";
    int result = system(cmd.c_str());
    std::remove(entitlementsXml.c_str());

    return result;
}
