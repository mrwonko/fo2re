//
//-=-=-=-=-=-=-=-=-=-=-=--=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// File        : $Id: FO2data.h 447 2008-05-22 18:21:55Z GulBroz $
//   Revision  : $Revision: 447 $
//   Name      : $Name$
//
// Description : Definitions to import/export FlatOut2 track files from Blender
// Author      : 
// Date        : Sat Oct 07 09:54:12 2006
// Reader      : 
// Evolutions  : 
//
//  $Log$
//  Revision 1.3  2007/11/27 18:52:49  GulBroz
//  + Begin support for FO2 cars
//
//  Revision 1.2  2007/03/17 14:25:56  GulBroz
//   + Fix for Visual Studio
//
//  Revision 1.1  2007/03/13 21:27:58  GulBroz
//    + Port to Blender 2.43
//
//  ----- Switch to Blender 2.43
//
//  Revision 1.3  2007/02/17 17:03:27  GulBroz
//    + AI stuff (almost final)
//
//  Revision 1.2  2007/01/22 22:15:10  GulBroz
//    + Move matrices to FO2data
//
//  Revision 1.1  2006/10/21 18:43:50  GulBroz
//    + Add FO2 track support
//
//-=-=-=-=-=-=-=-=-=-=-=--=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//

#ifndef FO2DATA_H
#define FO2DATA_H

//#ifdef WITH_FO2
//#ifndef FO2_USE_ARMATURE
//#define FO2_USE_ARMATURE
//#endif
//#endif // WITH_FO2

//
//=============================================================================
//                              INCLUDE FILES
//=============================================================================
//
#if !defined(NO_BLENDER)
#include "DNA_object_types.h"
#include "DNA_curve_types.h"  // BPoint
#endif // NO_BLENDER

#include <stdio.h>

// #if !defined(NO_BLENDER)
// #ifdef BUILD_DLL
// /* DLL export */
// #define FO2_EXPORT __declspec(dllexport)
// #else
// /* EXE import */
// #define FO2_EXPORT __declspec(dllimport)
// #endif
// #endif // NO_BLENDER
// #define FO2_EXPORT

typedef unsigned int   uint;
typedef unsigned short ushort;
typedef unsigned char  uchar;

#ifdef _MSC_VER
#define snprintf _snprintf
#define strcasecmp _stricmp
#define strdup _strdup
#endif
//
//=============================================================================
//                                CONSTANTS
//=============================================================================
//
#define MAX_TABS 5

extern const char * const _tabs;

#define TABS(n) ( ((n) > MAX_TABS) ? _tabs : \
                  ((n) <= 0) ? &_tabs[MAX_TABS] : \
                  &_tabs[MAX_TABS-(n)] )

#define FO2_PATH_LEN 256     // Path length

#define FO2_READ_EXT  ".rd"  // Read log/dbg file extension
#define FO2_WRITE_EXT ".wr"  // Write log/dbg file extension

#define FO2_LOG_EXT  ".log"  // Log file extension
#define FO2_DBG_EXT  ".dbg"  // Debug file extension

#define LAYER_MIN  1	// Min index for layers
#define LAYER_MAX 20	// Max index for layers

#define LAYER_DYN_CAMERA  LAYER_MAX  // Layer for moving camera
#define LAYER_OBJECTS     20  // Layer for objects (lamps)
#define LAYER_DYN_OBJ     19  // Layer for dynamic objects
#define LAYER_DYN_MODELS  18  // Layer for dynamic object models
#define LAYER_AI_DATA     17  // Layer for AI data (not splines)
#define LAYER_SPLINES     16  // Layer for splines
#define LAST_USABLE_LAYER LAYER_SPLINES

// Directory separator
#ifdef WIN32
#define DIR_SEP '\\'
#else
#define DIR_SEP '/'
#endif // WIN32

//
//=============================================================================
//                      TYPE DEFINITIONS AND GLOBAL DATA
//=============================================================================
//
#ifdef WITH_FO2_DBG
extern FILE *dbg;
#endif // WITH_FO2_DBG
extern uint errLine;       // Faulty line in case of error

typedef struct {
    char geomDir[FO2_PATH_LEN];  // Track dir   : .../data/tracks/arena/arena1/a/geometry/
    char baseDir[FO2_PATH_LEN];  // Base dir    : .../data/tracks/arena/arena1/a/
    char lightDir[FO2_PATH_LEN]; // Light dir   : .../data/tracks/arena/arena1/a/lighting/
    char texDir[FO2_PATH_LEN];   // Textures dir: .../data/tracks/arena/textures/
    char dynDir[FO2_PATH_LEN];   // Dynamics dir: .../data/global/dynamics/
    char trackName[FO2_PATH_LEN];

    char logFile[FO2_PATH_LEN];  // Log file   : .../data/logs/xxxxx.rd.log
#ifdef WITH_FO2_DBG
    char dbgFile[FO2_PATH_LEN];  // Dbg file   : .../data/logs/xxxxx.rd.dbg
#endif // WITH_FO2_DBG
} FO2_Path_t;

typedef enum { // *DO NOT* change order...
    //
    FO2_CAR,
    FO2_MENUCAR,
    FO2_TIRE,
    FO2_CAMERA,
    FO2_RAGDOLL,
    //
    FO2_END_OF_LIST
} FO2_FileType_t;

#define BREAK_ON_ERROR(r,w)				\
			w += ( (r) > 0 ? 1 : 0 );	\
			if( (r) < 0 ) break; else (r) = 0;

#define SET_ERROR_AND_BREAK(err,rtn)			\
			if( !errLine ) errLine = __LINE__;	\
			(rtn) = (err);						\
			break;

#define SET_ERROR_AND_RETURN(err)				\
			if( !errLine ) errLine = __LINE__;	\
			return((err));

#define CLEAR_ERROR do { errLine = 0; } while(0)

//
//=============================================================================
//                                ROUTINES
//=============================================================================
//
#if !defined(NO_BLENDER)
#ifdef __cplusplus
extern "C" {
#endif
void FO2_read_track(char *name);
void FO2_write_track(char *name);

void FO2_read_physic( char *name );

void FO2_read_car(char *name);
void FO2_read_menucar(char *name);
void FO2_read_tires( char *name );
void FO2_read_ragdoll( char *name );

void FO2_read_cameras( char *name );
void FO2_write_cameras( char *name );

#ifdef __cplusplus
}
#endif

void sendToLayer( Object *obj, const int layer );
#endif // NO_BLENDER

#endif // FO2TRACKDATA_H


//-- End of file : FO2data.h  --
