//
//  Logger.h
//  ATIKOSXDrivers
//
//  Created by Nick Kitchener on 12/10/2011.
//  Copyright 2011 ATIK. All rights reserved.
//

#ifndef ATIKArtemis_Logger_h
#define ATIKArtemis_Logger_h

#include <iostream>

class Logger {

public:
    static Logger *globalLogger(const char *logname);
    Logger(const char *logname);
    ~Logger();
    static void openFile();
    
    static void setDebugLogging(bool enableDebug);
    static bool debuggingEnabled();
    
    static void close();
    
    static void debugTimer(const char*fmt, ...);
    
    static void debugTransport(const char*fmt, ...);
    static void debugCypress(const char*fmt, ...);
    static void debugEZUSB(const char*fmt, ...);
    
    static void debug(const char*fmt, ...);
    static void important(const char*fmt, ...);
    
    static void dumpToFile(bool directionOut, uint8_t *data, int length);
    static void dumpToConsole(bool directionOut, uint8_t *data, int length);
};

#endif
