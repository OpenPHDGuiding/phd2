//
//  Services.h
//  ATIKOSXDrivers
//
//  Created by Nick Kitchener on 18/12/2014.
//  Copyright (c) 2014 ATIK. All rights reserved.
//

#ifndef ATIKOSXDrivers_ATIKLinuxServices_h
#define ATIKOSXDrivers_ATIKLinuxServices_h

#include <map>
#include <iostream>
#include "ServiceInterface.h"

// map pairs are (service identifier, protocol) and corresponding iterator
typedef std::map<std::string, std::string> ServiceIdentityList;
typedef ServiceIdentityList::iterator ServiceIdentityListIterator;

// map pairs are (busaddress, modelstring)
typedef std::map<uint16_t,std::string> PotentialDeviceList;
typedef PotentialDeviceList::iterator PotentialDeviceListIterator;

class Services {
public:
    virtual ~Services() {};
    
    // List all available (non-claimed services)
    // Returns a dictionary set of (serviceIdentifier->Protocol)
    // by returning an allocated array of ATIKLinuxServiceIdentity objects
    // Number of objects is returned in returnCount;
    // returned array is an immutable copy of the current available services that will not be dynamically updated
    // caller is responsible for deallocating the returned std map.
    virtual ServiceIdentityList* availableServices()=0;
    
    // Claim a service instance of the given name for exclusive use.
    virtual ServiceInterface* claimService(std::string serviceIdentifier)=0;
    
    // Release the previously claimed service instance.
    virtual void releaseService(ServiceInterface* serviceInstance)=0;
};

#endif
