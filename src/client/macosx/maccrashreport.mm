/*
 * maccrashreport.mm by Andrew Dai
 *
 * Copyright (C) 2010 Atomic Blue (info@planeshift.it, http://www.atomicblue.org) 
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation (version 2 of the License)
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */
// Mac specific (Objective-C++) code for wrapping around Google Breakpad.

// Only enabled in release builds

#ifndef CS_DEBUG

#import <client/mac/Framework/Breakpad.h>
#include "maccrashreport.h"
#include "csconfig.h"

BreakpadRef breakpad;

static BreakpadRef InitBreakpad(const char* hwRenderer, const char* hwVersion) {
  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
  breakpad = 0;
  NSDictionary *plist = [[NSBundle mainBundle] infoDictionary];
  NSMutableDictionary *mutableDict = [[plist mutableCopy] autorelease];
  NSDictionary *extraDict = [NSDictionary dictionaryWithObjectsAndKeys:
                   [NSString stringWithCString:hwRenderer], 
                   @"Renderer",
                   [NSString stringWithCString:hwVersion], 
                   @"RendererVersion",
                   [NSString stringWithCString:CS_COMPILER_NAME],
                   @"Compiler",
                   [NSString stringWithCString:CS_PROCESSOR_NAME],
                   @"Processor",
                   [NSString stringWithCString:CS_PLATFORM_NAME],
                   @"Platform",
                   [NSString stringWithCString:__DATE__ " " __TIME__],
                   @"BuildDate",
                   nil];
  [mutableDict setObject: extraDict forKey: @"BreakpadServerParameters"];
  if (plist) {
    breakpad = BreakpadCreate(mutableDict);
  }
  [pool release];
  return breakpad;
}

static void EndBreakpad(void) {
  BreakpadRelease(breakpad);
}

MacCrashReport::MacCrashReport(const char* hwRenderer, const char* hwVersion)
{
  InitBreakpad(hwRenderer, hwVersion);
}

MacCrashReport::~MacCrashReport()
{
  EndBreakpad();
}
#endif // CS_DEBUG
