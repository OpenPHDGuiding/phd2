//
//  ATIKLinuxDrivers_DriverListManagement.h
//  ATIKOSXDrivers
//
//  Created by Nick Kitchener on 24/12/2014.
//  Copyright (c) 2014 ATIK. All rights reserved.
//

// looks after a multi index list.

#ifndef __ATIKOSXDrivers__ATIKLinuxDrivers_DriverListManagement__
#define __ATIKOSXDrivers__ATIKLinuxDrivers_DriverListManagement__

#include <iostream>
#include <list>
#include <map>

#include "Services.h"
#include "ServiceInterface.h"

class ATIKLinuxDrivers_DriverListManagement {
    std::list<ServiceInterface*> *baseList;
    std::map<std::string,ServiceInterface*> *availableIndex; // available identities
    std::map<std::string,ServiceInterface*> *identityIndex;
    std::map<uint16_t,std::list<ServiceInterface*>> *busIndex; // each usb location = n services

public:
    ATIKLinuxDrivers_DriverListManagement();
    ~ATIKLinuxDrivers_DriverListManagement(); // deletes all elements and lists.

    // add a new service proxy to the list
    void addService(ServiceInterface* newService, uint16_t busIdentity);
    
    // remove the service
    void removeService(ServiceInterface* newService);
    
    // make a copy of all the service identifies which is a list of strings.
    ServiceIdentityList* copyServiceIdentityList();
    
    // claim from available list
    void claimService(ServiceInterface* newService);
    ServiceInterface* claimServiceUsingServiceIdentifer(std::string serviceIdentifier);
    
    // release back to available list
    void releaseService(ServiceInterface* newService);
    
    
    // get a service proxy using the bus address
    // this works regardless of available state.
    std::list<ServiceInterface*> findServiceUsingBusAddress(uint16_t busAddress);
    std::list<ServiceInterface*> findServiceUsingBusAddressAndProtocol(uint16_t busAddress, const std::string& protocolName);
    
};

#endif /* defined(__ATIKOSXDrivers__ATIKLinuxDrivers_DriverListManagement__) */
