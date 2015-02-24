//
//  FilterWheel100.h
//  ATIKOSXDrivers
//
//  Created by Nick Kitchener on 19/12/2014.
//  Copyright (c) 2014 ATIK. All rights reserved.
//

#ifndef ATIKOSXDrivers_FilterWheel100_h
#define ATIKOSXDrivers_FilterWheel100_h
#include <map>

#include "ServiceInterface.h"
#include "StateObserver.h"

const std::string kATIKProtocolNameFilterWheelAnyVersion = std::string("FilterWheel");

// filter states
const std::string kATIKFilterWheelStateInitialising = "kATIKFilterWheelStateInitialising";
const std::string kATIKFilterWheelStateIdle = "kATIKFilterWheelStateIdle";
const std::string kATIKFilterWheelStateTransitioning = "kATIKFilterWheelStateTransitioning";
const std::string kATIKFilterWheelStateTransitionComplete = "kATIKFilterWheelStateTransitionComplete";
const std::string kATIKFilterWheelStateError = "kATIKFilterWheelStateError";

typedef std::map<uint16_t, std::string> FilterList;
typedef FilterList::iterator FilterListIterator;

class FilterWheel100 : virtual public ServiceInterface {
public:
    virtual ~FilterWheel100() {};
    
    // Returns the available filters from this device.
    // In the case of a filter wheel, the count indicates the positions.
    // (1->""), (2->""), ..., (N->"")
    virtual FilterList* availableFilters()=0;
    
    virtual uint16_t totalNumberOfFiltersAvailable()=0;
    
    // select the filter by number
    // control returns once the filter has been selected.
    // position is 1 to N.
    virtual void setPosition(int position)=0;
    
    // read the current filter by number
    // returned position is 1 to N.
    virtual int position()=0;
    
    virtual bool isTransitioning()=0;
    
    virtual const std::string state() =0;
    virtual void setStateObserver(StateObserver* observer)=0;
    
};

#endif
