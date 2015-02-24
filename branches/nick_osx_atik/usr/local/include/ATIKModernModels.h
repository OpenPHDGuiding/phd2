//
//  ATIKModernModels.h
//  ATIKOSXDrivers
//
//  Created by Nick Kitchener on 02/01/2015.
//  Copyright (c) 2015 ATIK. All rights reserved.
//
//  This file holds string definitions for models.
//  Once initialised, the model is read from the camera (and will match these)

#ifndef ATIKOSXDrivers_ATIKModernModels_h
#define ATIKOSXDrivers_ATIKModernModels_h

const std::string kATIKModelModernAnyCamera = "ATIKModernAnyCamera";
const std::string kATIKModelModernAnyFilterWheel = "ATIKModernAnyFilterWheel";

// quicker cam driver
const std::string kATIKModelUnknownQuicker      = "ATIK Unknown Quicker";
const std::string kATIKModelATIKTitan           = "ATIK Titan";
const std::string kATIKModelSynopticsHighspeed  = "Synoptics Highspeed";

// IC24 cam driver
const std::string kATIKModelUnknownIC24     = "ATIK Unknown IC24";
const std::string kATIKModelSynopticsUSBHR  = "Synoptics USB HR";
const std::string kATIKModelSynopticsUSBXT  = "Synoptics USB XT";
const std::string kATIKModelSynopticsUSBEF  = "Synoptics USB EF";
const std::string kATIKModelATIK314L        = "ATIK 314L+";
const std::string kATIKModelATIK314E        = "ATIK 314E";
const std::string kATIKModelATIK320E        = "ATIK 320E";
const std::string kATIKModelATIK383L        = "ATIK 383L+";
const std::string kATIKModelATIK420         = "ATIK 420";
const std::string kATIKModelATIK450         = "ATIK 450";
const std::string kATIKModelATIK428EX       = "ATIK 428EX";
const std::string kATIKModelATIK460EX       = "ATIK 460EX";
const std::string kATIKModelATIK490EX       = "ATIK 490EX";
const std::string kATIKModelATIKOne         = "ATIK One";

const std::string kATIKModelATIK414EX       = "ATIK 414EX";
const std::string kATIKModelATIK4120EX         = "ATIK 4120EX";

// HSC cam driver
const std::string kATIKModelUnknownHSC      = "ATIK Unknown HSC";
const std::string kATIKModelATIK400011000   = "ATIK 4000/11000"; // "Fullframe"
const std::string kATIKModelATIK4000        = "ATIK 4000"; // note the camera returns 4002
const std::string kATIKModelATIK11000       = "ATIK 11000"; // note the camera returns 11002

// Filterwheels
const std::string kATIKModelATIKEFW         = "ATIK EFW";  // Old Filterwheel
const std::string kATIKModelATIKEFW2        = "ATIK EFW2"; // Filterwheel

// not supported
const std::string kATIKModelATIKGP              = "ATIK GP"; // NOTE THIS IS IIDC (not supported in drivers)
const std::string kATIKModelATIKSWIR            = "ATIK SWIR"; // NOTE THIS IS IIDC (not supported in drivers)


#endif
