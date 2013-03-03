/*
Copyright 2012 Sergey Zavadski

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
 */

#ifndef _PROPELLER_COMMON_H_
#define _PROPELLER_COMMON_H_

///
// C RunTime Header Files
//
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

//
//	STL headers
//
#include <string>
#include <list>
#include <map>
#include <algorithm>

//
//	System independent wrappers
//
#include "system.h"

//
//  version header
//
#include "version.h"




#ifdef WIN32

#ifdef PROPELLER_DLL 
    #define PROPELLER_API   __declspec(dllexport)
#else 
    #define PROPELLER_API   __declspec(dllimport)
#endif  
#else
    #define PROPELLER_API
#endif


#endif //_COMMON_H_
