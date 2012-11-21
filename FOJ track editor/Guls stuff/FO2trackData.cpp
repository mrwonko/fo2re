//
//-=-=-=-=-=-=-=-=-=-=-=--=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// File        : $Id: FO2trackData.cpp 605 2012-02-24 22:49:19Z GulBroz $
//   Revision  : $Revision: 605 $
//   Name      : $Name$
//
// Description : Class bodies to access FlatOut2 track files
// Author      :
// Date        : Sat Oct 07 11:35:52 2006
// Reader      :
// Evolutions  :
//
//  $Log$
//  Revision 1.4  2007/11/27 18:52:49  GulBroz
//  + Begin support for FO2 cars
//
//  Revision 1.3  2007/03/23 23:03:50  GulBroz
//    + Add support for multi-UV
//
//  Revision 1.2  2007/03/17 14:25:56  GulBroz
//   + Fix for Visual Studio
//
//  Revision 1.1  2007/03/13 21:27:58  GulBroz
//    + Port to Blender 2.43
//
//  ----- Switch to Blender 2.43
//
//  Revision 1.9  2007/02/20 19:48:52  GulBroz
//    + Add export and load to AI .bed files
//
//  Revision 1.8  2007/02/17 17:03:27  GulBroz
//    + AI stuff (almost final)
//
//  Revision 1.7  2007/01/30 19:41:04  GulBroz
//    + Fix model searching
//
//  Revision 1.6  2007/01/22 22:14:17  GulBroz
//    + Move matrices to FO2data
//
//  Revision 1.5  2007/01/01 11:37:11  GulBroz
//    + Add Scene name (import)
//    + Fix group management in Meshes
//    + Fix log & debug file path (export)
//
//  Revision 1.4  2006/12/26 15:24:43  GulBroz
//    + Fix matrix
//
//  Revision 1.3  2006/12/26 13:10:08  GulBroz
//    + Add write support
//
//  Revision 1.7  2006/11/29 22:57:27  GulBroz
//    + Add dynamic object support
//
//  Revision 1.6  2006/10/29 10:47:51  GulBroz
//    + Fix alpha channel in Blender
//
//  Revision 1.5  2006/10/27 22:57:58  GulBroz
//    + Fix track reading
//
//  Revision 1.4  2006/10/14 16:09:23  GulBroz
//    + Write debug data to file
//    + Add import/export function for Blender DLL
//
//  Revision 1.3  2006/10/13 17:51:29  GulBroz
//    + Add unknown structs, Objects, Models and Meshes
//
//  Revision 1.2  2006/10/08 08:46:24  GulBroz
//    + Add Streams and Surfaces
//
//  Revision 1.1  2006/10/08 06:32:55  GulBroz
//    + First release
//
//-=-=-=-=-=-=-=-=-=-=-=--=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//

// TODO: REMOVEME
//#define WITH_FO2_DBG

// Uncomment for surface writing
//#define FO2_WRITE_SURFACES

//
//=============================================================================
//                              INCLUDE FILES
//=============================================================================
//
#include <ctype.h>
#include <math.h>
#include <errno.h>
#include <time.h>

#include <stdio.h>
#include <sys/types.h>

#include "FO2regex.h"
#include "FO2data.h"
#include "FO2trackData.h"
#include "FO2trackAI.h"

#if !defined(NO_BLENDER)

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include "MEM_guardedalloc.h"

#include "MTC_matrixops.h"

#include "DNA_object_types.h"
#include "DNA_image_types.h"
#include "DNA_mesh_types.h"
#include "DNA_material_types.h"
#include "DNA_meshdata_types.h"
#include "DNA_scene_types.h"
#include "DNA_group_types.h"
#include "DNA_texture_types.h"
#include "DNA_material_types.h"
//#include "DNA_lamp_types.h"
#ifdef FO2_USE_ARMATURE
#include "DNA_armature_types.h"
#endif // FO2_USE_ARMATURE
#include "DNA_FO2physic_types.h"
#include "DNA_FO2object_types.h"
#include "DNA_view3d_types.h"

#include "BKE_main.h"
#include "BKE_global.h"
#include "BKE_object.h"
#include "BKE_library.h"
#include "BKE_image.h"
#include "BKE_mesh.h"
#include "BKE_bad_level_calls.h"
#include "BKE_material.h"
#include "BKE_group.h"
#include "BKE_texture.h"
#include "BKE_customdata.h"
#ifdef FO2_USE_ARMATURE
#include "BKE_armature.h"
#endif // FO2_USE_ARMATURE
#include "BKE_depsgraph.h"
#include "FO2physic.h"
#include "FO2object.h"

#include "BDR_editface.h"  // default_tface
#include "BDR_editobject.h" // apply_obmat()  test_parent_loop()

#include "blendef.h" // FIRSTBASE

#include "BLI_blenlib.h"  // BLI_addhead()
#include "BLI_arithb.h"   // Mat4Invert()

#include "BIF_toolbox.h"

#include "BIF_screen.h"   // screen_view3d_layers()
#ifdef FO2_USE_ARMATURE
#include "BIF_editarmature.h"
#endif // FO2_USE_ARMATURE

#include "mydevice.h"

#ifdef WITH_FO2_DBG
#include "DNA_curve_types.h"
#include "BKE_curve.h"
#endif // WITH_FO2_DBG

#include "BKE_utildefines.h" // VECCOPY INIT_MINMAX MAX2 SWAP

#ifdef __cplusplus
}
#endif // __cplusplus

#else
#ifndef FLT_EPSILON
#define FLT_EPSILON     1.192092896e-07F        /* smallest such that 1.0+FLT_EPSILON != 1.0 */
#endif // FLT_EPSILON

#endif // NO_BLENDER

#ifdef _MSC_VER
#define strncasecmp _strnicmp
#endif

//
//=============================================================================
//                                CONSTANTS
//=============================================================================
//

// Value used to detect dynamice objects models in streams
#define MAX_STREAM_DYN_MODEL_DETECT 10000

const char * const _tabs = "\t\t\t\t\t";
//
//=============================================================================
//                      TYPE DEFINITIONS AND GLOBAL DATA
//=============================================================================
//

#ifdef WITH_FO2_DBG
FILE *dbg = stdout;
#endif // WITH_FO2_DBG
uint errLine = 0;       // Faulty line in case of error

//
//=============================================================================
//                            INTERNAL ROUTINES
//=============================================================================
//

//-----------------------------------------------------------------------------
// Name   : printMsg
// Usage  : Log message to console and/or debug file
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         flag             X   -   Where to send message flag
//         fmt              X   -   Format string
//         ...              X   -   (Optional) arguments
//
// Return : Number of chars printed
//-----------------------------------------------------------------------------
int printMsg( int flag, const char *fmt, ... )
{
	int rtn = 0;
	va_list ap;
	va_start( ap, fmt );
#ifdef WITH_FO2_DBG
	if( (flag & MSG_DBG) && dbg ) {
		rtn = vfprintf( dbg, fmt, ap );
		fflush(dbg);
	}
#endif // WITH_FO2_DBG
	if( flag & MSG_CONSOLE) rtn = vprintf( fmt, ap );
	va_end(ap);
	return( rtn );
}

//-----------------------------------------------------------------------------
// Name   : readString
// Usage  : Read a zero-terminated string from file
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         h                X   X   File handler
//
// Note   : Caller is responsible for string deallocation!!
//
// Return : Pointer to string, or NULL in case of error
//-----------------------------------------------------------------------------
char *readString( FILE * h )
{
    char tmp[1024];
    char *pos = tmp;
    uint i = 0;

    for( i = 0; i < sizeof(tmp); ++i, ++pos ) {
        if( 1 != fread( pos, 1, 1, h )) return( NULL ); // Read error

        if( 0 == *pos ) break; // End of string...
    } // End for

    if( sizeof(tmp) == i ) return( NULL ); // Buffer is too small!!

    return( strdup( tmp ));
} // End proc readString

//-----------------------------------------------------------------------------
// Name   : readString
// Usage  : Read a zero-terminated string from file
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         h                X   X   File handler
//         str              -   X   String read
//
// Return : 0 = OK / -1 = read error / -2 internal buffer too small
//-----------------------------------------------------------------------------
int readString( FILE * h, string &str )
{
    char tmp[1024];
    char *pos = tmp;
    uint i = 0;

    for( i = 0; i < sizeof(tmp); ++i, ++pos ) {
        if( 1 != fread( pos, 1, 1, h )) return( -1 ); // Read error

        if( 0 == *pos ) break; // End of string...
    } // End for

    if( sizeof(tmp) == i ) return( -2 ); // Buffer is too small!!

    str = tmp;

    return( 0 );
} // End proc readString

//-----------------------------------------------------------------------------
// Name   : writeString
// Usage  : Write a zero-terminated string to file
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         h                X   X   File handler
//         pStr             X   -   Pointer to string
//
// Return : 0 in case of success, -1 otherwise
//-----------------------------------------------------------------------------
int writeString( FILE * h, const char *pStr )
{
    int rtn = -1;
    size_t l = 0;

    if( pStr ) {
        l = strlen( pStr ) + 1;
        if( 1 == fwrite( pStr, l, 1, h)) rtn = 0;
    } else {
        if( 1 == fwrite( &l, 1, 1, h)) rtn = 0;
    }

    return( rtn );
} // End proc readString

//-----------------------------------------------------------------------------
// Name   : writeString
// Usage  : Write a string to file
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         h                X   X   File handler
//         str              X   -   Pointer to string
//
// Return : 0 in case of success, -1 otherwise
//-----------------------------------------------------------------------------
int writeString( FILE * h, const string &str )
{
    int rtn = -1;
    size_t l = 0;

    l = str.length() + 1;
    if( 1 == fwrite( str.c_str(), l, 1, h)) rtn = 0;

    return( rtn );
} // End proc readString

// uint readuint( FILE *h );
// ushort  readushort( FILE *h );
// float readFloat( FILE *h );
// uchar  readuchar( FILE * h );
// uint readID( FILE * h );

//--#if !defined(NO_BLENDER)
//-----------------------------------------------------------------------------
// Name   : setDynObjModelLayer
// Usage  : Set dynamic object model layer
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         track            X   -   Track object
//         pLayers          X   -   Pointer to layer array
//
// Return : Nothing
//-----------------------------------------------------------------------------
static void setDynObjModelLayer( cTrack& track, uint *pLayers ) {
    cModel       *pModel      = NULL;
    cSurface     *pSurf       = NULL;
    cMesh        *pMesh       = NULL;
    cMeshToBBox  *pMeshToBB   = NULL;
    cBoundingBox *pBBox       = NULL;

    if( pLayers ) {
        for( uint m = 0; m < track.meshList.dwNMesh; ++m ) {
            pMesh  = &track.meshList.meshes[m];
            pMeshToBB = &track.meshToBBoxList.meshToBBoxes[ pMesh->dwM2BBIdx ];
            pBBox = &track.boundingBoxList.boundingBoxes[ pMeshToBB->dwBBIdx[0] ];
            pModel = &track.modelList.models[ pBBox->dwModelIdx[0] ];
            pSurf = &track.surfaceList.surfaces[ pModel->dwSurfIdx[0] ];

            pLayers[pSurf->dwVStreamIdx] = LAYER_DYN_MODELS;
        } // End for (m)
    } // End if
} // End setDynObjModelLayer
//--#endif // NO_BLENDER

#if !defined(NO_BLENDER)
//-----------------------------------------------------------------------------
// Name   : moveRotateObj
// Usage  : Move and rotate object
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         obj              X   -   Pointer to object to copy (source)
//         matrix           X   -   Pointer to Blender transformation matrix
//         name             X   -   (Optional) Object name
//
// Return : 0 in case of success, -1 otherwise
//-----------------------------------------------------------------------------
int moveRotateObj( Object           *obj,
                   const cMatrix4x4 *matrix,
                   char             *name ) {
    int rtn = -1;

    do {
        if( NULL == obj || NULL == matrix ) { errLine = __LINE__; break; }

        // Translate & rotate object to its position
		VECCOPY( obj->loc, matrix->m[3] );

        // Rotate object
		float e[3],m[4][4];
		memcpy( m, matrix->m, sizeof(m) );
		Mat4ToEul( m, e );
		VECCOPY( obj->rot, e );

        // Change object name
        if( name ) rename_id( &obj->id, name );

        rtn = 0;
    } while(0);

    return( rtn );
} // End moveRotateObj
#endif // NO_BLENDER

#if !defined(NO_BLENDER)
//-----------------------------------------------------------------------------
// Name   : linkCopyObj
// Usage  : Make a linked copy of object, move and rotate it, and send layer
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         obj              X   -   Pointer to object to copy (source)
//         layer            X   -   Layer of new object
//         matrix           X   -   (Optional) Pointer to transformation matrix
//         name             X   -   (Optional) New object name
//
// Return : Pointer to new object in case of success, NULL otherwise
//-----------------------------------------------------------------------------
static Object *linkCopyObj( Object           *obj,
                            const int         layer,
                            const cMatrix4x4 *matrix   = NULL,
                            char             *name     = NULL ) {

    Object *objNew = NULL;

    do {
        if( NULL == obj ) { errLine = __LINE__; break; }

#if 0  // Object copy
        objNew = add_object( OB_MESH );
        if( NULL == objNew ) { errLine = __LINE__; break; }
        objNew->data = copy_mesh( (Mesh*)obj->data ); // Copy
        objNew->gameflag |= 1;
#endif

        // Object linked copy
        Base *base, *basen;

        // From adduplicate() @ source/blender/src/editobject.c
        //  vv
        objNew = copy_object( obj );
        if( NULL == objNew ) { errLine = __LINE__; break; }

        base = (Base*)FIRSTBASE;  /* first base in current scene */
        while( base ){
            if( base->object == obj ) break;
            base = base->next;
        } // End while
        if( NULL == base ) { errLine = __LINE__; break; }

        basen = (Base*)MEM_mallocN(sizeof(Base), "duplibase");
        if( NULL == basen ) { errLine = __LINE__; break; }

        *basen = *base;
        BLI_addhead(&G.scene->base, basen); /* addhead: prevent eternal loop */
        basen->object = objNew;
        //  ^^

        // Rotate and translate object to its position
        if( matrix) {
            (void)moveRotateObj( objNew, matrix, name );
        } else if( name ) {
            // Change object name
            rename_id( &objNew->id, name );
        }

        objNew->recalc |= OB_RECALC_OB;
        DAG_object_flush_update(G.scene, objNew, OB_RECALC_DATA);

        // Move object to layer
        sendToLayer( objNew, layer );
    } while(0);

    return( objNew );
} // End linkCopyObj
#endif // NO_BLENDER

#if !defined(NO_BLENDER)
//-----------------------------------------------------------------------------
// Name   : findMaterial
// Usage  : Find material
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         name             X   -   Name of material
//
// Return : Pointer to Material, or NULL
//-----------------------------------------------------------------------------
static Material *findMaterial( const char *name )
{
    Material *ma = NULL;

    if( name && *name ) {
        // Try to find material in material list
        for( ma = (Material*)G.main->mat.first;
             NULL != ma;
             ma = (Material*)ma->id.next ) {
            // 2 first chars of id are object type...
            if( 0 == strcmp( ma->id.name+2, name )) break;
        } // End for
    }

    return( ma );
} // End findMaterial
#endif // NO_BLENDER

#if !defined(NO_BLENDER)
//-----------------------------------------------------------------------------
// Name   : findTexture
// Usage  : Find texture by name
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         name             X   -   Name of texture
//
// Return : Pointer to Tex, or NULL
//-----------------------------------------------------------------------------
static Tex *findTexture( const char *name )
{
    Tex *te = NULL;

    if( name && *name ) {
        // Try to find material in material list
        for( te = (Tex*)G.main->tex.first;
             NULL != te;
             te = (Tex*)te->id.next ) {
            // 2 first chars of id are object type...
            if( 0 == strcmp( te->id.name+2, name )) break;
        } // End for
    }

    return( te );
} // End findTexture
#endif // NO_BLENDER

#if !defined(NO_BLENDER)
//-----------------------------------------------------------------------------
// Name   : findPhysic
// Usage  : Find physic
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         name             X   -   Name of physic
//
// Return : Pointer to Physic, or NULL
//-----------------------------------------------------------------------------
static Physic *findPhysic( const char *name )
{
    Physic *phy = NULL;

    if( name && *name ) {
        // Try to find physic in physic list
        for( phy = (Physic*)G.main->physic.first;
             NULL != phy;
             phy = (Physic*)phy->id.next ) {
            // 2 first chars of id are object type...
            if( 0 == strcmp( phy->id.name+2, name )) break;
        } // End for
    }

    return( phy );
} // End findPhysic
#endif // NO_BLENDER

#if !defined(NO_BLENDER)
//-----------------------------------------------------------------------------
// Name   : setFakeUser
// Usage  : "Fake use" objects so they're saved
// Args   : None
//
// Return : Nothing
//-----------------------------------------------------------------------------
static void setFakeUser( short objType )
{
    ID *id = NULL;

	switch( objType ) {
		case ID_MA:
			id = (ID*)G.main->mat.first;
			break;
		case ID_TE:
			id = (ID*)G.main->tex.first;
			break;
		case ID_PH:
			id = (ID*)G.main->physic.first;
			break;
		default:
			id = NULL;
			break;
	}

	for( ; NULL != id; id = (ID*)id->next ) {
		id->flag |= LIB_FAKEUSER;
	} // End for

	return;
} // End setFakeUser
#endif // NO_BLENDER

//-----------------------------------------------------------------------------
// Name   : buildPath
// Usage  : Compute track related paths
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         name             X   -   Path to track_geom.w32 file
//         pPath            -   X   Pointer to path structure
//         forReading       X   -   Read flag
//
// Return : 0 == OK / -1 == error
//-----------------------------------------------------------------------------
int buildPath( const char* name, FO2_Path_t *pPath,
                      const int forReading ) {
    int rtn = 0;
    char *pSep;

    do {
        if( NULL == name || NULL == pPath ) {
            errLine = __LINE__;
            rtn = -1;
            break;
        }

        memset( pPath, 0, sizeof(*pPath) );

// printMsg( MSG_CONSOLE, "=> name = %s \n", name );
        // Compute track directory
        snprintf( pPath->geomDir, sizeof(pPath->geomDir),
                  "%s", name );
        pSep = strrchr( pPath->geomDir, DIR_SEP );
        if( !pSep ) { errLine = __LINE__; rtn = -1; break; }
        *pSep = 0;
// printMsg( MSG_CONSOLE, "=> geomDir = %s \n", pPath->geomDir );

        // Compute base directory
        snprintf( pPath->baseDir, sizeof(pPath->baseDir),
                  "%s", pPath->geomDir );
        pSep = strrchr( pPath->baseDir, DIR_SEP );
        if( !pSep ) { errLine = __LINE__; rtn = -1; break; }
        *pSep = 0;
// printMsg( MSG_CONSOLE, "=> baseDir = %s \n", pPath->baseDir );

        // Compute track name (use logFile as temp buffer!!)
        snprintf( pPath->logFile, sizeof(pPath->logFile),
                  "%s", pPath->baseDir );
// printMsg( MSG_CONSOLE, "=> logFile = %s \n", pPath->logFile );
        pSep = strrchr( pPath->logFile, DIR_SEP );
// printMsg( MSG_CONSOLE, "=> pSep = %s \n", pSep );
        for( int i = 0; i < 2; ++i ) {
            pSep = strrchr( pPath->logFile, DIR_SEP );
// printMsg( MSG_CONSOLE, "=> pSep = %s \n", pSep );
            if( !pSep ) { errLine = __LINE__; rtn = -1; break; }
            *pSep = '_';
        }
        if( -1 == rtn ) break;
        snprintf( pPath->trackName, sizeof(pPath->trackName),
                  "%s", (pSep+1) );
// printMsg( MSG_CONSOLE, "=> trackName = %s \n", pPath->trackName );

        // Compute log file name
        snprintf( pPath->logFile, sizeof(pPath->logFile),
                  "%s", pPath->baseDir );
        pSep = strrchr( pPath->logFile, DIR_SEP );
        for( int i = 0; i < 4; ++i ) {
            pSep = strrchr( pPath->logFile, DIR_SEP );
            if( !pSep ) { errLine = __LINE__; rtn = -1; break; }
            *pSep = 0;
        }
        if( -1 == rtn ) break;
        snprintf( pSep, sizeof(pPath->logFile)-strlen(pPath->logFile),
                  "%clogs%c%s%s%s",
                  DIR_SEP, DIR_SEP, pPath->trackName,
                  (forReading ? FO2_READ_EXT : FO2_WRITE_EXT), FO2_LOG_EXT );
// printMsg( MSG_CONSOLE, "=> logFile = %s \n", logFile );

#ifdef WITH_FO2_DBG
        snprintf( pPath->dbgFile, sizeof(pPath->dbgFile),
                  "%s", pPath->logFile );
        pSep = strstr( pPath->dbgFile, FO2_LOG_EXT );
        if( pSep ) sprintf( pSep, "%s", FO2_DBG_EXT );
#endif // WITH_FO2_DBG

        // Compute lighting directory
        snprintf( pPath->lightDir, sizeof(pPath->lightDir),
                  "%s%clighting", pPath->baseDir, DIR_SEP );

        // Compute texture directory
        snprintf( pPath->texDir, sizeof(pPath->texDir),
                  "%s", pPath->baseDir );
        for( int i = 0; i < 2; ++i ) {
            pSep = strrchr( pPath->texDir, DIR_SEP );
            if( !pSep ) { errLine = __LINE__; rtn = -1; break; }
            *pSep = 0;
        }
        if( -1 == rtn ) break;
        snprintf( pSep, sizeof(pPath->texDir)-strlen(pPath->texDir),
                  "%ctextures", DIR_SEP );

        // Compute dynamics directory
        snprintf( pPath->dynDir, sizeof(pPath->dynDir),
                  "%s", pPath->baseDir );
        pSep = strrchr( pPath->dynDir, DIR_SEP );
        for( int i = 0; i < 4; ++i ) {
            pSep = strrchr( pPath->dynDir, DIR_SEP );
            if( !pSep ) { errLine = __LINE__; rtn = -1; break; }
            *pSep = 0;
        }
        if( -1 == rtn ) break;
        snprintf( pSep, sizeof(pPath->dynDir)-strlen(pPath->dynDir),
                  "%cglobal%cdynamics", DIR_SEP, DIR_SEP );
    } while(0);

#ifdef WITH_FO2_DBG
    if( pPath ) {
        printMsg( MSG_CONSOLE, "Track file  : <%s>\n", name );
        printMsg( MSG_CONSOLE, "Track name  : <%s>\n", pPath->trackName );
        printMsg( MSG_CONSOLE, "Base dir    : <%s>\n", pPath->baseDir );
        printMsg( MSG_CONSOLE, "Track dir   : <%s>\n", pPath->geomDir );
        printMsg( MSG_CONSOLE, "Light dir   : <%s>\n", pPath->lightDir );
        printMsg( MSG_CONSOLE, "Textures dir: <%s>\n", pPath->texDir );
        printMsg( MSG_CONSOLE, "Dynamics dir: <%s>\n", pPath->dynDir );
        printMsg( MSG_CONSOLE, "Log file    : <%s>\n", pPath->logFile );
        printMsg( MSG_CONSOLE, "Dbg file    : <%s>\n", pPath->dbgFile );
    }
#endif // WITH_FO2_DBG

    return( rtn );
} // End proc buildPath

//
//=============================================================================
//                                ROUTINES
//=============================================================================
//

//-----------------------------------------------------------------------------
// Name   : cPoint3D::read
// Usage  : Read cPoint3D from file
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         h                X   X   File handler
//
// Return : 0 == OK / -1 == read error
//-----------------------------------------------------------------------------
int cPoint3D::read(FILE * h, bool fouc)
{
    int rtn = -1;

    do {
        if(!fouc) {
            if( 1 != fread( &x, sizeof(x), 1, h )) break;
            if( 1 != fread( &z, sizeof(z), 1, h )) break; // Z & Y are inverted!!
            if( 1 != fread( &y, sizeof(y), 1, h )) break;
        } else {
            short s;
            if( 1 != fread( &s, sizeof(s), 1, h )) break;
            x= (float)s / FOUC_FACTOR;
            if( 1 != fread( &s, sizeof(s), 1, h )) break; // Z & Y are inverted!!
            z= (float)s / FOUC_FACTOR;
            if( 1 != fread( &s, sizeof(s), 1, h )) break;
            y= (float)s / FOUC_FACTOR;
        }

        rtn = 0;
    } while(0);

    if( rtn ) errLine = __LINE__;

    return( rtn );
} // End proc cPoint3D::read

//-----------------------------------------------------------------------------
// Name   : cPoint3D::write
// Usage  : Write cPoint3D to file
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         h                X   X   File handler
//
// Return : 0 == OK / -1 == write error
//-----------------------------------------------------------------------------
int cPoint3D::write(FILE * h, bool fouc)
{
    int rtn = -1;

    do {
        if(!fouc) {
            if( 1 != fwrite( &x, sizeof(x), 1, h )) break;
            if( 1 != fwrite( &z, sizeof(z), 1, h )) break; // Z & Y are inverted!!
            if( 1 != fwrite( &y, sizeof(y), 1, h )) break;
        } else {
            short s;
            s= (short)(x * FOUC_FACTOR);
            if( 1 != fwrite( &s, sizeof(s), 1, h )) break;
            s= (short)(z * FOUC_FACTOR);
            if( 1 != fwrite( &s, sizeof(s), 1, h )) break; // Z & Y are inverted!!
            s= (short)(y * FOUC_FACTOR);
            if( 1 != fwrite( &s, sizeof(s), 1, h )) break;
        }

        rtn = 0;
    } while(0);

    if( rtn ) errLine = __LINE__;

    return( rtn );
} // End proc cPoint3D::write

//-----------------------------------------------------------------------------
// Name   : cPoint3D::show
// Usage  : Dump cPoint3D to screen
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         prefix           X   -   (Optional) prefix string
//         where            X   -   (Optional) where to send data
//
// Return : Nothing
//-----------------------------------------------------------------------------
void cPoint3D::show( const char * prefix, FILE *where )
{
    const char * myPre = ( prefix ? prefix : "" );
    FILE *out = ( where ? where : stdout );

    fprintf( out, "%sx=%f (%g) ", myPre, x, x );
    fprintf( out, "%sy=%f (%g) ", myPre, y, y );
    fprintf( out, "%sz=%f (%g) ", myPre, z, z );
	fflush( out );
} // End proc cPoint3D::show

//-----------------------------------------------------------------------------
// Name   : cPoint2D::read
// Usage  : Read cPoint2D from file
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         h                X   X   File handler
//
// Return : 0 == OK / -1 == read error
//-----------------------------------------------------------------------------
int cPoint2D::read(FILE * h, bool fouc)
{
    int rtn = -1;

    do {
        if(!fouc) {
            if( 1 != fread( &u, sizeof(u), 1, h )) break;
            if( 1 != fread( &v, sizeof(v), 1, h )) break;
        } else {
            short s;
            if( 1 != fread( &s, sizeof(s), 1, h )) break;
            u= (float)s / FOUC_FACTOR;
            if( 1 != fread( &s, sizeof(s), 1, h )) break;
            v= (float)s / FOUC_FACTOR;
        }

        rtn = 0;
    } while(0);

    if( rtn ) errLine = __LINE__;

    return( rtn );
} // End proc cPoint2D::read

//-----------------------------------------------------------------------------
// Name   : cPoint2D::write
// Usage  : Write cPoint2D to file
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         h                X   X   File handler
//
// Return : 0 == OK / -1 == write error
//-----------------------------------------------------------------------------
int cPoint2D::write(FILE * h, bool fouc)
{
    int rtn = -1;

    do {
        if(!fouc) {
            if( 1 != fwrite( &u, sizeof(u), 1, h )) break;
            if( 1 != fwrite( &v, sizeof(v), 1, h )) break;
        } else {
            short s;
            s= (short)(u * FOUC_FACTOR);
            if( 1 != fwrite( &s, sizeof(s), 1, h )) break;
            s= (short)(v * FOUC_FACTOR);
            if( 1 != fwrite( &s, sizeof(s), 1, h )) break;
        }

        rtn = 0;
    } while(0);

    if( rtn ) errLine = __LINE__;

    return( rtn );
} // End proc cPoint2D::write

//-----------------------------------------------------------------------------
// Name   : cPoint2D::show
// Usage  : Dump cPoint2D to screen
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         suffix           X   -   (Optionnal) suffix string
//         where            X   -   (Optional) where to send data
//
// Return : Nothing
//-----------------------------------------------------------------------------
void cPoint2D::show( const char * suffix, FILE *where )
{
    const char * mySuf = ( suffix ? suffix : "" );
    FILE *out = ( where ? where : stdout );

    fprintf( out, "u%s=%f (%g) ", mySuf, u, u );
    fprintf( out, "v%s=%f (%g) ", mySuf, v, v );
	fflush( out );
} // End proc cPoint2D::show

//-----------------------------------------------------------------------------
// Name   : cMatrix4x4::cMatrix4x4
// Usage  : Constructor from 3x3 matrix
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         m3               X   -   3x3 matrix
//
// Return : Nothing
//-----------------------------------------------------------------------------
// cMatrix4x4::cMatrix4x4( const cMatrix3x3& m3 )
// {
//     for( uint j = 0; j < 3; ++j ) {
//         for( uint i = 0; i < 3; ++i ) {
//             m[i][j] = m3[i][j];
//         }
//     }
//     m[0][3] = m[1][3] = m[2][3] = m[3][0] = m[3][1] = m[3][2] = 0.0;
//     m[3][3] = 1.0;
// } // End proc cMatrix4x4::cMatrix4x4

//-----------------------------------------------------------------------------
// Name   : cMatrix4x4::zero
// Usage  : Set matrix to zero matrix
// Args   : None
//
// Return : Nothing
//-----------------------------------------------------------------------------
void cMatrix4x4::zero( void )
{
	memset( m, 0, sizeof(m));
} // End proc cMatrix4x4::zero

#define READ_MAT(m,i,j)  if( 1 != fread(  &((m)[(i)][(j)]), sizeof((m)[(i)][(j)]), 1, h )) break;
#define WRITE_MAT(m,i,j) if( 1 != fwrite( &((m)[(i)][(j)]), sizeof((m)[(i)][(j)]), 1, h )) break;
//-----------------------------------------------------------------------------
// Name   : cMatrix4x4::read
// Usage  : Read cMatrix4x4 from file
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         h                X   X   File handler
//
// Return : 0 == OK / -1 == read error
//-----------------------------------------------------------------------------
int cMatrix4x4::read( FILE * h )
{
	int rtn = -1;

	do {
		// File (FlatOut) : ( a0 b0 c0 d0 )  Internal (Blender) : ( a0 c0 b0 d0 )
		//                  ( a1 b1 c1 d1 )                       ( a2 c2 b2 d2 )
		//                  ( a2 b2 c2 d2 )                       ( a1 c1 b1 d1 )
		//                  ( a3 b3 c3 d3 )                       ( a3 c3 b3 d3 )

		// 1st col of Blender matrix
		READ_MAT(m,0,0);
		READ_MAT(m,0,2);
		READ_MAT(m,0,1);
		READ_MAT(m,0,3);

		// 2nd col of Blender matrix
		READ_MAT(m,2,0);
		READ_MAT(m,2,2);
		READ_MAT(m,2,1);
		READ_MAT(m,2,3);

		// 3rd col of Blender matrix
		READ_MAT(m,1,0);
		READ_MAT(m,1,2);
		READ_MAT(m,1,1);
		READ_MAT(m,1,3);

		// 4th col of Blender matrix
		READ_MAT(m,3,0);
		READ_MAT(m,3,2);
		READ_MAT(m,3,1);
		READ_MAT(m,3,3);

		rtn = 0;
	} while(0);

	if( rtn ) errLine = __LINE__;

	return( rtn );
} // End proc cMatrix4x4::read

//-----------------------------------------------------------------------------
// Name   : cMatrix4x4::write
// Usage  : Write cMatrix4x4 to file
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         h                X   X   File handler
//
// Return : 0 == OK / -1 == write error
//-----------------------------------------------------------------------------
int cMatrix4x4::write( FILE * h )
{
	int rtn = -1;

	do {
		// Internal (Blender) : ( a0 b0 c0 d0 )  File (FlatOut) : ( a0 c0 b0 d0 )
		//                      ( a1 b1 c1 d1 )                   ( a2 c2 b2 d2 )
		//                      ( a2 b2 c2 d2 )                   ( a1 c1 b1 d1 )
		//                      ( a3 b3 c3 d3 )                   ( a3 c3 b3 d3 )

		// 1st col of FlatOut matrix
		WRITE_MAT(m,0,0);
		WRITE_MAT(m,0,2);
		WRITE_MAT(m,0,1);
		WRITE_MAT(m,0,3);

		// 2nd col of FlatOut matrix
		WRITE_MAT(m,2,0);
		WRITE_MAT(m,2,2);
		WRITE_MAT(m,2,1);
		WRITE_MAT(m,2,3);

		// 3rd col of FlatOut matrix
		WRITE_MAT(m,1,0);
		WRITE_MAT(m,1,2);
		WRITE_MAT(m,1,1);
		WRITE_MAT(m,1,3);

		// 4th col of FlatOut matrix
		WRITE_MAT(m,3,0);
		WRITE_MAT(m,3,2);
		WRITE_MAT(m,3,1);
		WRITE_MAT(m,3,3);

		rtn = 0;
	} while(0);

	if( rtn ) errLine = __LINE__;

	return( rtn );
} // End proc cMatrix4x4::write

//-----------------------------------------------------------------------------
// Name   : cMatrix4x4::show
// Usage  : Dump cMatrix4x4 to screen
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         tab              X   -   (Optional) number of tabs before printing
//         where            X   -   (Optional) where to send data
//
// Return : Nothing
//-----------------------------------------------------------------------------
void cMatrix4x4::show( const uint tab, FILE *where )
{
    const char * myTab = TABS(tab);
    FILE *out = ( where ? where : stdout );

    fprintf( out, "{\n" );
    for( uint j = 0; j < 4; ++j ) {
        fprintf( out, "%s\t", myTab );

        for( uint i = 0; i < 4; ++i ) {
//             fprintf( out, "(%d,%d) %f |\t", i, j, m[i][j] );
            fprintf( out, "(%d,%d) %g |\t", i, j, m[i][j] );
        }

        fprintf( out, "\n" );
    }
    fprintf( out, "%s}\n", myTab );
	fflush( out );
} // End proc cMatrix4x4::show

//-----------------------------------------------------------------------------
// Name   : cMatrix3x3::zero
// Usage  : Set matrix to zero matrix
// Args   : None
//
// Return : Nothing
//-----------------------------------------------------------------------------
void cMatrix3x3::zero( void )
{
	memset( m, 0, sizeof(m));
} // End proc cMatrix3x3::zero

//-----------------------------------------------------------------------------
// Name   : cMatrix3x3::read
// Usage  : Read cMatrix3x3 from file
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         h                X   X   File handler
//
// Return : 0 == OK / -1 == read error
//-----------------------------------------------------------------------------
int cMatrix3x3::read( FILE * h )
{
	int rtn = -1;

	do {
		// File (FlatOut) : ( a0 b0 c0 )  Internal (Blender) : ( a0 c0 b0 )
		//                  ( a1 b1 c1 )                       ( a2 c2 b2 )
		//                  ( a2 b2 c2 )                       ( a1 c1 b1 )

		// 1st col of Blender matrix
		READ_MAT(m,0,0);
		READ_MAT(m,0,2);
		READ_MAT(m,0,1);

		// 2nd col of Blender matrix
		READ_MAT(m,2,0);
		READ_MAT(m,2,2);
		READ_MAT(m,2,1);

		// 3rd col of Blender matrix
		READ_MAT(m,1,0);
		READ_MAT(m,1,2);
		READ_MAT(m,1,1);

		rtn = 0;
	} while(0);

	if( rtn ) errLine = __LINE__;

	return( rtn );
} // End proc cMatrix3x3::read

//-----------------------------------------------------------------------------
// Name   : cMatrix3x3::write
// Usage  : Write cMatrix3x3 to file
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         h                X   X   File handler
//
// Return : 0 == OK / -1 == write error
//-----------------------------------------------------------------------------
int cMatrix3x3::write( FILE * h )
{
	int rtn = -1;

	do {
		// Internal (Blender) : ( a0 b0 c0 )  File (FlatOut) : ( a0 c0 b0 )
		//                      ( a1 b1 c1 )                   ( a2 c2 b2 )
		//                      ( a2 b2 c2 )                   ( a1 c1 b1 )

		// 1st col of FlatOut matrix
		WRITE_MAT(m,0,0);
		WRITE_MAT(m,0,2);
		WRITE_MAT(m,0,1);

		// 2nd col of FlatOut matrix
		WRITE_MAT(m,2,0);
		WRITE_MAT(m,2,2);
		WRITE_MAT(m,2,1);

		// 3rd col of FlatOut matrix
		WRITE_MAT(m,1,0);
		WRITE_MAT(m,1,2);
		WRITE_MAT(m,1,1);

		rtn = 0;
	} while(0);

	if( rtn ) errLine = __LINE__;

	return( rtn );
} // End proc cMatrix3x3::write

//-----------------------------------------------------------------------------
// Name   : cMatrix3x3::show
// Usage  : Dump cMatrix3x3 to screen
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         tab              X   -   (Optional) number of tabs before printing
//         where            X   -   (Optional) where to send data
//
// Return : Nothing
//-----------------------------------------------------------------------------
void cMatrix3x3::show( const uint tab, FILE *where )
{
    const char * myTab = TABS(tab);
    FILE *out = ( where ? where : stdout );

    fprintf( out, "{\n" );
    for( uint j = 0; j < 3; ++j ) {
        fprintf( out, "%s\t", myTab );

        for( uint i = 0; i < 3; ++i ) {
//             fprintf( out, "(%d,%d) %f |\t", i, j, m[i][j] );
            fprintf( out, "(%d,%d) %g |\t", i, j, m[i][j] );
        }

        fprintf( out, "\n" );
    }
    fprintf( out, "%s}\n", myTab );
	fflush( out );
} // End proc cMatrix3x3::show

//-----------------------------------------------------------------------------
// Name   : cMaterial::cMaterial
// Usage  : cMaterial class constructor
// Args   : None
// Return : Nothing
//-----------------------------------------------------------------------------
cMaterial::cMaterial()
{
    dwID    = ID_MATERIAL;
    dwAlpha = 0;
    dwUnk1  = 0;
    dwNtex  = 0;
    memset( bUnknown, 0, sizeof(bUnknown) );
    sTexture.push_back("");
    sTexture.push_back("");
    sTexture.push_back("");

    _index   = 0;
    _filePos = 0;

#if !defined(NO_BLENDER)
    _img[0] = _img[1] = _img[2] = NULL;
#endif // !NO_BLENDER

} // End proc cMaterial::cMaterial

//-----------------------------------------------------------------------------
// Name   : cMaterial::~cMaterial
// Usage  : cMaterial class destructor
// Args   : None
// Return : Nothing
//-----------------------------------------------------------------------------
// cMaterial::~cMaterial()
// {
//     if( sName )     free( sName );
//     if( sTexture[0] ) free( sTexture[0] );
//     if( sTexture[1] ) free( sTexture[1] );
//     if( sUnknown )  free( sUnknown );
// } // End proc cMaterial::~cMaterial


//-----------------------------------------------------------------------------
// Name   : cMaterial::read
// Usage  : Read cMaterial from file
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         h                X   X   File handler
//         idx              X   -   (Optiona) material index
//
// Return : 0 == OK / -1 == read error / -2 == id error
//-----------------------------------------------------------------------------
int cMaterial::read( FILE *h, const uint idx )
{
    int rtn = -1;
    string s;

    do {
        _index   = idx;
        _filePos = ftell( h );

        sTexture.clear();
        sTexture.push_back("");
        sTexture.push_back("");
        sTexture.push_back("");

        if( 1 != fread( &dwID, sizeof(dwID), 1, h )) break;
        if( ID_MATERIAL != dwID ) { rtn = -2; break; }

        if( 0 != readString( h, sName )) break;
        if( 1 != fread( &dwAlpha, sizeof(dwAlpha), 1, h )) break;
        if( 1 != fread( &dwUnk1,  sizeof(dwUnk1), 1, h ))  break;
        if( 1 != fread( &dwNtex,  sizeof(dwNtex), 1, h ))  break;
        if( 1 != fread( &bUnknown[0], sizeof(bUnknown), 1, h )) break;
        if( 0 != readString( h, sTexture[0] )) break;
        if( 0 != readString( h, sTexture[1] )) break;
        if( 0 != readString( h, sTexture[2] )) break;

        rtn = 0;
    } while(0);

    if( rtn ) errLine = __LINE__;

    return( rtn );
} // End proc cMaterial::read

//-----------------------------------------------------------------------------
// Name   : cMaterial::write
// Usage  : Write cMaterial to file
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         h                X   X   File handler
//
// Return : 0 == OK / -1 == write error / -2 == id error
//-----------------------------------------------------------------------------
int cMaterial::write( FILE *h )
{
    int rtn = -1;

    do {
        if( ID_MATERIAL != dwID ) { rtn = -2; break; }
        if( 1 != fwrite( &dwID, sizeof(dwID), 1, h )) break;

        if( 0 != writeString( h, sName )) break;
        if( 1 != fwrite( &dwAlpha, sizeof(dwAlpha), 1, h )) break;
        if( 1 != fwrite( &dwUnk1,  sizeof(dwUnk1), 1, h ))  break;
        if( 1 != fwrite( &dwNtex,  sizeof(dwNtex), 1, h ))  break;
        if( 1 != fwrite( &bUnknown[0], sizeof(bUnknown), 1, h )) break;
        if( 0 != writeString( h, sTexture[0] )) break;
        if( 0 != writeString( h, sTexture[1] )) break;
        if( 0 != writeString( h, sTexture[2] )) break;

        rtn = 0;
    } while(0);

    if( rtn ) errLine = __LINE__;

    return( rtn );
} // End proc cMaterial::write

#if !defined(NO_BLENDER)
//-----------------------------------------------------------------------------
// Name   : cMaterial::toBlender
// Usage  : Draw in Blender
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         lightDir         X   -   Lighting directory
//         texDir           X   -   Texture directory
//         carMaterial      X   -   This is car material
//
// Return : 0 in case of success
//-----------------------------------------------------------------------------
int cMaterial::toBlender( const char* lightDir, const char* texDir,
                          const bool carMaterial )
{
    char fileName[FO2_PATH_LEN];
    Image     *pImg       = NULL;
    uint       lg         = 0;  // Texture name length
    uint       tIdx       = 0;  // Texture index
    const char *psTexture = NULL;
    int rtn = 0;

	Material *ma;		// Material
	Tex *te;			// Texture
	char name[256];		// Texture/Material name
	char *pDot;
	int nbTextures = 0;	// Number of textures in material

	// Create new Blender material
	snprintf( name, sizeof(name), "%s", sName.c_str() );
    ma = findMaterial( name );
	if( ma ) {
		// Eeeek! that should not happen!!!
	} else {
        ma = add_material( name );
	}

	// Set material properties
	if( ma ) {
		ma->mode |= MA_SHLESS;	// Insensitive to light/shadows
		ma->pr_type = MA_FLAT;

		ma->FO_terrain = bUnknown[1];
		ma->FO_flag0 = dwUnk1;
		ma->FO_flag1 = bUnknown[0];
		ma->FO_flag3 = bUnknown[2];
	}

    // Compute number of textures in material
	for( tIdx = 0; tIdx < MAX_NB_TEXTURES; ++tIdx ) if( sTexture[tIdx] != "" ) nbTextures++;

    for( tIdx = 0; tIdx < MAX_NB_TEXTURES; ++tIdx ) {
        psTexture = sTexture[tIdx].c_str();
        if( 0 == *psTexture ) continue;

        lg = *fileName = 0;
        if( carMaterial ) {
            if( 0 == strncasecmp( psTexture, TEX_SKIN, strlen(TEX_SKIN) )) {
                snprintf( fileName, sizeof(fileName),
                          "%s%c%s", texDir, DIR_SEP, psTexture );
            } else {
                snprintf( fileName, sizeof(fileName),
                          "%s%c%s", lightDir, DIR_SEP, psTexture );
            }
            lg = strlen( psTexture );
        } else {
            if( 0 == strcasecmp( psTexture, TEX_COLORMAP )) {
                snprintf( fileName, sizeof(fileName),
                          "%s%c%s", lightDir, DIR_SEP, TEX_LIGHTMAP );
                lg = strlen( TEX_LIGHTMAP );
            } else {
                snprintf( fileName, sizeof(fileName),
                          "%s%c%s", texDir, DIR_SEP, psTexture );
                lg = strlen( psTexture );
            }
        }

        if( *fileName ) {
            for( char *c = fileName + strlen(fileName) - 1;
                 lg; --lg, --c ) *c = tolower(*c);

#ifdef WITH_FO2_DBG
            printMsg( MSG_DBG, "About to load image '%s' ....\n", fileName );
#endif // WITH_FO2_DBG

            pImg = BKE_add_image_file( fileName );

#ifdef WITH_FO2_DBG
            printMsg( MSG_DBG, "\t%s -> %s\n", fileName, (pImg ? "OK" : "Failed"));
#endif // WITH_FO2_DBG

            if( NULL == pImg ) {
                errLine = __LINE__;
                rtn = -1;
                break;
            }

            // Update material class
            _img[tIdx] = pImg;

            // Change image name (remove extension,
            // and make it lowercase)
//                     char *pC  = strrchr( fileName, '.' );
//                     if( pC ) {
//                         *pC = 0;
//                         pC = strrchr( fileName, DIR_SEP );
//                         if( pC ) {
//                             for( char *c = pC+1; *c; ++c ) {
//                                 *c = tolower(*c );
//                             }
//                             rename_id( &pImg->id, pC+1 );
//                         }
//                     }
		} // End if (fileName)

		// Add Blender texture
		snprintf( name, sizeof(name), "%s", psTexture );
		pDot = strrchr( name, '.' );	// Remove file extension
		if( pDot ) *pDot = 0;
        te = findTexture( name );
        if( NULL == te ) {
            te = add_texture( name );
			if( te ) {
				te->type = TEX_IMAGE;
				te->ima  = pImg;
			}
        }
		// Set Alpha & Tiling flags
		switch( bUnknown[tIdx] ) {
			case TEXTURE_TILE:   { te->extend= TEX_REPEAT; break; }
			case TEXTURE_NORMAL: { te->extend= TEX_CLIP;   break; }
			default: { // For display only
				te->extend = TEX_CHECKER;
				te->flag |= ( TEX_CHECKER_ODD | TEX_CHECKER_EVEN );
			}
		}

		if( dwAlpha ) {
			te->imaflag = TEX_USEALPHA;
			te->flag |= TEX_PRV_ALPHA;
			ma->alpha = 0.0;
			ma->mode |= MA_RAYTRANSP;
		}

		// Add texture to material
		if( te && ma ) {
			ma->mtex[tIdx] = add_mtex();
			if( ma->mtex[tIdx] ) {
				ma->mtex[tIdx]->tex   = te;
				ma->mtex[tIdx]->texco = TEXCO_UV;
				snprintf( ma->mtex[tIdx]->uvname, sizeof(ma->mtex[tIdx]->uvname),
					"%s", (0==tIdx ? TEXTURE1_NAME : TEXTURE2_NAME) );
				ma->mtex[tIdx]->mapto = MAP_COL;
				if( dwAlpha ) ma->mtex[tIdx]->mapto |= MAP_ALPHA;
				ma->mtex[tIdx]->colfac = (1.0 / nbTextures);
			}
		}
    } // End for (tIdx)

    if( rtn ) errLine = __LINE__;

    return( rtn );
} // End proc cMaterial::toBlender
#endif // NO_BLENDER

#if !defined(NO_BLENDER)
//-----------------------------------------------------------------------------
// Name   : cMaterial::fromBlender
// Usage  : Get material from Blender
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         ma               X   -   Pointer to Blender material
//         carMaterial      X   -   This is car material
//
// Return : 0 in case of success
//-----------------------------------------------------------------------------
int cMaterial::fromBlender( Material *ma, const bool carMaterial )
{
	int rtn = 0;

	// Assuming Blender material is correct!

	do {
		sName = ma->id.name+2;
		dwAlpha = 0;
		dwNtex  = 0;
		_mat = ma;
		sTexture[0] = sTexture[1] = sTexture[2] = "";

		if( 0.0 == ma->alpha && (ma->mode & MA_RAYTRANSP) ) dwAlpha = 1;

		for( int i = 0; i < sTexture.size(); ++i ) {
			if( ma->mtex[i] ) {
				Tex *te = ma->mtex[i]->tex;
				if( te ) {
					sTexture[i] = te->id.name+2;
					sTexture[i] += ".tga";
					dwNtex++;
				}
			}
		} // End for

		// Texture flags
		memset( bUnknown, 0, sizeof(bUnknown) );
		bUnknown[1] = ma->FO_terrain;
		dwUnk1 = ma->FO_flag0;
		bUnknown[0] = ma->FO_flag1;
		bUnknown[2] = ma->FO_flag3;

		// Material without texture
		if( 0 == dwNtex ) dwNtex = 1;

	} while( 0 );

	return( rtn );
} // End proc cMaterial::fromBlender
#endif // NO_BLENDER

//-----------------------------------------------------------------------------
// Name   : cMaterial::show
// Usage  : Dump cMaterial to screen
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         tab              X   -   (Optional) number of tabs before printing
//         where            X   -   (Optional) where to send data
//
// Return : Nothing
//-----------------------------------------------------------------------------
void cMaterial::show( const uint tab, FILE *where )
{
    const char * myTab = TABS(tab);
    FILE *out = ( where ? where : stdout );

    fprintf( out, "%s{ -- material #%d (fileOffset=0x%08x)\n",
             myTab, _index, _filePos );
    fprintf( out, "%s\tdwID      = 0x%08x (%c%c%c%c)\n",
             myTab, dwID, SHOW_ID(dwID) );
    fprintf( out, "%s\tsName     = %s\n",          myTab, sName.c_str() );
    fprintf( out, "%s\tdwAlpha   = %d\n",          myTab, dwAlpha );
    fprintf( out, "%s\tdwUnk1    = 0x%08x (%d)\n", myTab, dwUnk1, dwUnk1 );
    fprintf( out, "%s\tdwNtex    = %d\n",          myTab, dwNtex );
    fprintf( out, "%s\tunknown   = { ", myTab );
    for( ushort i = 0; i < (sizeof(bUnknown)/sizeof(bUnknown[0])); ++i ) {
        fprintf( out, "0x%08x ", bUnknown[i] );
    } // End for
    fprintf( out, "}\n" );
    fprintf( out, "%s\tsTexture[0] = %s\n",        myTab, sTexture[0].c_str());
    fprintf( out, "%s\tsTexture[1] = %s\n",        myTab, sTexture[1].c_str());
    fprintf( out, "%s\tsTexture[2] = %s\n",        myTab, sTexture[2].c_str());
    fprintf( out, "%s}\n", myTab );
	fflush( out );
} // End proc cMaterial::show

//-----------------------------------------------------------------------------
// Name   : cMaterialList::read
// Usage  : Read cMaterialList from file
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         h                X   X   File handler
//
// Return : 0 == OK / -1 == read error / -2 == memory error
//-----------------------------------------------------------------------------
int cMaterialList::read( FILE *h )
{
    int rtn = 0;
    cMaterial m;

    _filePos = ftell( h );

    do {
        clear();

        if( 1 != fread( &dwNmaterials, sizeof(dwNmaterials), 1, h )) {
            rtn = -1;
            break;
        }

        if( dwNmaterials ) {
            materials.reserve( dwNmaterials );
            if( materials.capacity() < dwNmaterials ) {
                errLine = __LINE__;
                rtn = -2;
                break;
            }

            for( uint i = 0; 0 == rtn && i < dwNmaterials; ++i ) {
                rtn = m.read( h, i );
                if( 0 == rtn ) materials.push_back( m );
            } // End for
            if( 0 != rtn )  break;
        } // End if
    } while(0);

    return( rtn );
} // End proc cMaterialList::read

//-----------------------------------------------------------------------------
// Name   : cMaterialList::write
// Usage  : Write cMaterialList to file
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         h                X   X   File handler
//
// Return : 0 == OK / -1 == write error / -2 == memory error
//-----------------------------------------------------------------------------
int cMaterialList::write( FILE *h )
{
    int rtn = -1;

    do {
        if( dwNmaterials != materials.size() ) {
            errLine = __LINE__;
            rtn = -2;
            break;
        }

        if( 1 != fwrite( &dwNmaterials, sizeof(dwNmaterials), 1, h )) {
            errLine = __LINE__;
            break;
        }

        if( dwNmaterials ) {
            rtn = 0;
            for( uint i = 0; 0 == rtn && i < dwNmaterials; ++i ) {
                rtn = materials[i].write( h );
            } // End for
            if( 0 != rtn ) break;
        } // End if

        rtn = 0;
    } while(0);

    return( rtn );
} // End proc cMaterialList::write

#if !defined(NO_BLENDER)
//-----------------------------------------------------------------------------
// Name   : cMaterialList::toBlender
// Usage  : Draw in Blender
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         lightDir         X   -   Lighting directory
//         texDir           X   -   Texture directory
//         carMaterial      X   -   This is car material
//
// Return : Nothing
//-----------------------------------------------------------------------------
int cMaterialList::toBlender( const char* lightDir, const char* texDir,
                              const bool carMaterial )
{
    int rtn = 0;

    for( uint i = 0; 0 == rtn && i < dwNmaterials; ++i ) {
        rtn = materials[i].toBlender( lightDir, texDir, carMaterial );
    } // End for

    return( rtn );
} // End proc cMaterialList::toBlender
#endif // NO_BLENDER

#if !defined(NO_BLENDER)
//-----------------------------------------------------------------------------
// Name   : cMaterialList::fromBlender
// Usage  : Get material list from Blender
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         carMaterial      X   -   This is car material
//
// Return : 0 == OK / -1 == write error / -2 == id error / -3 == memory error
//-----------------------------------------------------------------------------
int cMaterialList::fromBlender( const bool carMaterial )
{
#if 0
	int rtn = 0;
    Material *ma;
    cMaterial *pMat;

	// Start with empty list
    clear();

	// --------------------------------------------------------------------
    //  Rebuild material list
    // --------------------------------------------------------------------
    for( ma = (Material*)G.main->mat.first; 0 == rtn && NULL != ma; ma = (Material*)ma->id.next ) {

		// Create new material
		pMat = new cMaterial;
		if( NULL == pMat ) {
			printMsg( MSG_ALL, "Memory error @ %d\n", __LINE__ );
			rtn = -1;
			break;
		}

		rtn = pMat->fromBlender( ma );
		if( 0 == rtn ) materials.push_back( *pMat );

		delete pMat;
	} // End for

	dwNmaterials = materials.size();

	return( rtn );
#endif
	int rtn = 0;
    Material *ma;
    cMaterial foMat;

	// Start with empty list
#if 0 // Because of unknown4 structure, only add new texture to end of list...
    clear();
#endif

	// --------------------------------------------------------------------
    //  Rebuild material list
    // --------------------------------------------------------------------
    for( ma = (Material*)G.main->mat.first; 0 == rtn && NULL != ma; ma = (Material*)ma->id.next ) {

#if 0 // Because of unknown4 structure, only add new texture to end of list...
		foMat._index = materials.size();
		rtn = foMat.fromBlender( ma );
		if( 0 == rtn ) materials.push_back( foMat );
#endif
		vector<cMaterial>::iterator pMat;
		for( pMat = materials.begin(); pMat != materials.end(); ++pMat ) {
			if( pMat->sName == ma->id.name+2 ) break;
		}
		if( pMat == materials.end() ) {
			// Not found, add a new one
			foMat._index = materials.size();
			rtn = foMat.fromBlender( ma );
			if( 0 == rtn ) materials.push_back( foMat );
		} else {
			// Already exists, update
			rtn = pMat->fromBlender( ma );
		}
	} // End for

	dwNmaterials = materials.size();

	return( rtn );
} // End proc cMaterialList::fromBlender
#endif // NO_BLENDER

#if !defined(NO_BLENDER)
//-----------------------------------------------------------------------------
// Name   : cMaterialList::indexOf
// Usage  : Find index of material in list
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         mat              X   -   Blender material
//
// Return : -1 == not found, list index otherwise
//-----------------------------------------------------------------------------
int cMaterialList::indexOf( Material *mat )
{
	int idx = -1;

	for( int i=0; mat && i < materials.size(); ++i ) {
		if( materials[i]._mat == mat ) {
			idx = i;
			break;
		}
	}

	return( idx );
} // End proc cMaterialList::indexOf
#endif // NO_BLENDER

//-----------------------------------------------------------------------------
// Name   : cMaterialList::show
// Usage  : Dump cMaterialList to screen
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         tab              X   -   (Optional) number of tabs before printing
//         where            X   -   (Optional) where to send data
//
// Return : Nothing
//-----------------------------------------------------------------------------
void cMaterialList::show( const uint tab, FILE *where )
{
    const char * myTab = TABS(tab);
    FILE *out = ( where ? where : stdout );

    fprintf( out, "%sMaterials = { -- NB materials: %d\n",
             myTab, dwNmaterials );
    for( uint i = 0; i < dwNmaterials; ++i ) {
        materials[i].show( tab + 1, where );
    } // End for
    fprintf( out, "%s}, -- End Materials\n", myTab );
	fflush( out );
} // End proc cMaterialList::show


//-----------------------------------------------------------------------------
// Name   : cStream::~cStream
// Usage  : cStream desctructor
// Args   : None
// Return : Nothing
//-----------------------------------------------------------------------------
cStream::~cStream()
{
    // Nothing...
} // End proc cStream::~cStream

//-----------------------------------------------------------------------------
// Name   : cStream::read
// Usage  : Read cStream from file
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         h                X   X   File handler
//
// Return : 0 == OK / -1 == read error
//-----------------------------------------------------------------------------
int cStream::read( FILE *h )
{
    int rtn = -1;

    do {
        _filePos = ftell( h );

        if( 1 != fread( &dwType,   sizeof(dwType),   1, h )) break;
        if( 1 != fread( &dwZero,   sizeof(dwZero),   1, h )) break;
        if( 1 != fread( &dwNum,    sizeof(dwNum),    1, h )) break;
        if( STREAM_TYPE_INDEX != dwType ) {
            if( 1 != fread( &dwStride, sizeof(dwStride), 1, h )) break;
            if( STREAM_TYPE_UNKNOWN != dwType ) {
                if( 1 != fread( &dwFormat, sizeof(dwFormat), 1, h )) break;
            }
        }

        rtn = 0;
    } while(0);

    if( rtn ) errLine = __LINE__;

    return( rtn );
} // End proc cStream::read

//-----------------------------------------------------------------------------
// Name   : cStream::write
// Usage  : Write cStream to file
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         h                X   X   File handler
//
// Return : 0 == OK / -1 == write error
//-----------------------------------------------------------------------------
int cStream::write( FILE *h )
{
    int rtn = -1;

    do {
        if( 1 != fwrite( &dwType,   sizeof(dwType),   1, h )) break;
        if( 1 != fwrite( &dwZero,   sizeof(dwZero),   1, h )) break;
        if( 1 != fwrite( &dwNum,    sizeof(dwNum),    1, h )) break;
        if( STREAM_TYPE_INDEX != dwType ) {
            if( 1 != fwrite( &dwStride, sizeof(dwStride), 1, h )) break;
            if( STREAM_TYPE_UNKNOWN != dwType ) {
                if( 1 != fwrite( &dwFormat, sizeof(dwFormat), 1, h )) break;
            }
        }

        rtn = 0;
    } while(0);

    if( rtn ) errLine = __LINE__;

    return( rtn );
} // End proc cStream::write

//-----------------------------------------------------------------------------
// Name   : cStream::show
// Usage  : Dump cStream to screen
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         tab              X   -   (Optional) number of tabs before printing
//         where            X   -   (Optional) where to send data
//
// Return : Nothing
//-----------------------------------------------------------------------------
#if 0
void cStream::show( const uint tab, FILE *where )
{
    const char * myTab = TABS(tab);
    FILE *out = ( where ? where : stdout );

    fprintf( out, "%sStream = {\n",     myTab );
    fprintf( out, "%s\tdwType   = 0x%08x\n", myTab, dwType );
    fprintf( out, "%s\tdwZero   = %d\n",     myTab, dwZero );
    fprintf( out, "%s\tdwNum    = 0x%08x (%d)\n", myTab, dwNum, dwNum );
    fprintf( out, "%s\tdwStride = 0x%08x (%d)\n", myTab, dwStride, dwStride);
    fprintf( out, "%s\tdwFormat = 0x%08x (%d)\n", myTab, dwFormat, dwFormat);
    fprintf( out, "%s}\n",                    myTab );
	fflush( out );
} // End proc cStream::show
#endif

//-----------------------------------------------------------------------------
// Name   : cVertexStream::cVertexStream
// Usage  : cVertexStream constructor
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         format           X   -   Vertex stream format
//
// Return : Nothing
//-----------------------------------------------------------------------------
cVertexStream::cVertexStream( const uint format ) : cStream()
{
	dwType = STREAM_TYPE_VERTEX;
	dwFormat = format;
	dwStride = 0;
	if( format & VERTEX_POSITION ) dwStride += 3*sizeof(float);
	if( format & VERTEX_UV )       dwStride += 2*sizeof(float);
	if( format & VERTEX_UV2 )      dwStride += 4*sizeof(float);
	if( format & VERTEX_NORMAL )   dwStride += 3*sizeof(float);
	if( format & VERTEX_BLEND )    dwStride += sizeof(float);
} // End proc cVertexStream::cVertexStream

//-----------------------------------------------------------------------------
// Name   : cVertexStream::read
// Usage  : Read cVertexStream from file
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         h                X   X   File handler
//
// Return : 0 == OK / -1 == read error / -2 memory error
//-----------------------------------------------------------------------------
int cVertexStream::read( FILE * h )
{
    int rtn = 0;

    cStream::read( h );

    _filePos = ftell( h );

	vertices.clear();

    if( dwNum ) {
        vertices.reserve( dwNum );
        if( vertices.capacity() < dwNum ) {
            rtn = -2;
		} else {
			cVertex v;
			for( uint i = 0; 0 == rtn && i < dwNum; ++i ) {
				rtn = v.read( h, dwFormat, _filePos, i );
				if( 0 == rtn ) vertices.push_back(v);
			} // End for
		}
    } // End if

    if( rtn ) errLine = __LINE__;

    return( rtn );
} // End proc cVertexStream::read

//-----------------------------------------------------------------------------
// Name   : cVertexStream::write
// Usage  : Write cVertexStream to file
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         h                X   X   File handler
//
// Return : 0 == OK / -1 == write error / -2 memory error
//-----------------------------------------------------------------------------
int cVertexStream::write( FILE * h )
{
    int rtn = 0;

    rtn = cStream::write( h );

    uint basePos = ftell( h );

    if( 0 == rtn && dwNum ) {
        if( dwNum != vertices.size() ) {
            rtn = -2;
		} else {
			for( uint i = 0; 0 == rtn && i < dwNum; ++i ) {
				rtn = vertices[i].write( h, dwFormat, basePos );
			} // End for
		}
    } // End if

    if( rtn ) errLine = __LINE__;

    return( rtn );
} // End proc cVertexStream::write

//-----------------------------------------------------------------------------
// Name   : cVertexStream::show
// Usage  : Dump cVertexStream to screen
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         n                X   -   Stream index
//         tab              X   -   (Optional) number of tabs before printing
//         where            X   -   (Optional) where to send data
//
// Return : Nothing
//-----------------------------------------------------------------------------
//
void cVertexStream::show( const uint n, const uint tab, FILE *where )
{
    const char * myTab = TABS(tab);
    FILE *out = ( where ? where : stdout );

    fprintf( out,
             "%s{ -- Vertex stream #%d @ 0x%08x\n", myTab, n, _filePos );
    fprintf( out, "%s\tdwType   = 0x%08x\n", myTab, dwType );
    fprintf( out, "%s\tdwZero   = %d\n",     myTab, dwZero );
    fprintf( out, "%s\tdwNum    = 0x%08x (%d)\n", myTab, dwNum, dwNum );
    fprintf( out, "%s\tdwStride = 0x%08x (%d)\n", myTab, dwStride, dwStride);
    fprintf( out, "%s\tdwFormat = 0x%08x (%d)\n", myTab, dwFormat, dwFormat);
    for( uint i = 0; i < dwNum; ++i ) {
        vertices[i].show( dwFormat, tab + 1, where );
    } // End for
    fprintf( out, "%s}, -- End Vertex stream #%d\n", myTab, n );
	fflush( out );
} // End proc cVertexStream::show

//-----------------------------------------------------------------------------
// Name   : cVertex::read
// Usage  : Read cVertex from file
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         h                X   X   File handler
//         format           X   -   Vertex format
//         basePos          X   -   Offset of 1st item in list
//         idx              X   -   (Optiona) material index
//
// Return : 0 == OK / -1 == read error / -2 == id error
//-----------------------------------------------------------------------------
int cVertex::read( FILE * h, const uint format,
                   const uint basePos, const uint idx )
{
    int rtn = -1;

    do {
        bool fouc= (VERTEX_FOUC == (format & VERTEX_FOUC));
        _index        = idx;
        _offsetStream = ftell( h ) - basePos;

        if(!fouc) {
            if( format & VERTEX_POSITION )
                if( 0 != ( rtn = pos.read( h ))) break;

            if( format & VERTEX_NORMAL )
                if( 0 != ( rtn = normal.read( h ))) break;

            if( format & VERTEX_BLEND )
                if( 1 != fread( &fBlend, sizeof(fBlend), 1, h )) {rtn = -1; break;}

            if( format & ( VERTEX_UV | VERTEX_UV2 ))
                if( 0 != ( rtn = uv[0].read( h ))) break;

            if( format & VERTEX_UV2 )
                if( 0 != ( rtn = uv[1].read( h ))) break;
        } else {
            unsigned int dummy;

            if( format & VERTEX_POSITION )
                if( 0 != ( rtn = pos.read( h, true ))) break;

            //if( format & VERTEX_NORMAL )
                if( 0 != ( rtn = normal.read( h, true ))) break;

            if( 1 != fread( &dummy, sizeof(dummy), 1, h ) ||
                1 != fread( &dummy, sizeof(dummy), 1, h )) {rtn = -1; break;}

            if( format & VERTEX_BLEND )
                if( 1 != fread( &fBlend, sizeof(fBlend), 1, h )) {rtn = -1; break;}

            if( format & ( VERTEX_UV | VERTEX_UV2 ))
                if( 0 != ( rtn = uv[0].read( h, true ))) break;

            if( format & VERTEX_UV2 )
                if( 0 != ( rtn = uv[1].read( h, true ))) break;
        }

        rtn = 0;
    } while(0);

    if( rtn ) errLine = __LINE__;

    return( rtn );
} // End proc cVertex::read

//-----------------------------------------------------------------------------
// Name   : cVertex::write
// Usage  : Write cVertex to file
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         h                X   X   File handler
//         format           X   -   Vertex format
//         basePos          X   -   Offset of 1st item in list
//
// Return : 0 == OK / -1 == write error / -2 == id error
//-----------------------------------------------------------------------------
// TODO: basePos useless ??
int cVertex::write( FILE * h, const uint format,
                    const uint basePos )
{
    int rtn = -1;

    do {
        if( format & VERTEX_POSITION )
            if( 0 != ( rtn = pos.write( h ))) break;

        if( format & VERTEX_NORMAL )
            if( 0 != ( rtn = normal.write( h ))) break;

        if( format & VERTEX_BLEND )
            if( 1 != fwrite( &fBlend, sizeof(fBlend), 1, h )) {
                rtn = -1;
                break;
            }

        if( format & ( VERTEX_UV | VERTEX_UV2 ))
            if( 0 != ( rtn = uv[0].write( h ))) break;

        if( format & VERTEX_UV2 )
            if( 0 != ( rtn = uv[1].write( h ))) break;

        rtn = 0;
    } while(0);

    if( rtn ) errLine = __LINE__;

    return( rtn );
} // End proc cVertex::write

//-----------------------------------------------------------------------------
// Name   : cVertex::show
// Usage  : Dump cVertex to screen
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         format           X   -   Vertex format
//         tab              X   -   (Optional) number of tabs before printing
//         where            X   -   (Optional) where to send data
//
// Return : Nothing
//-----------------------------------------------------------------------------
void cVertex::show( const uint format, const uint tab, FILE *where )
{
    const char * myTab = TABS(tab);
    FILE *out = ( where ? where : stdout );

    fprintf( out, "%s{\n", myTab );
    if( format & VERTEX_POSITION ) {
        fprintf( out, "%s\t", myTab );
        pos.show( NULL, where );
        fprintf( out, "\n" );
    }

    if( format & VERTEX_NORMAL ) {
        fprintf( out, "%s\t", myTab );
        normal.show( "N", where );
        fprintf( out, "\n" );
    }

    if( format & VERTEX_BLEND )
        fprintf( out, "%s\tfBlend    = %f (%g) (0x%08x)\n", myTab, fBlend, fBlend, *((int*)&fBlend) );

    if( format & ( VERTEX_UV | VERTEX_UV2 )) {
        fprintf( out, "%s\t", myTab );
        uv[0].show( "[0]", where );
        fprintf( out, "\n" );
    }

    if( format & VERTEX_UV2 ) {
        fprintf( out, "%s\t", myTab );
        uv[1].show( "[1]", where );
        fprintf( out, "\n" );
    }

    fprintf( out,
             "%s},  -- #%d @ offset 0x%08x\n", myTab, _index, _offsetStream );
	fflush( out );
} // End proc cVertex::show

//-----------------------------------------------------------------------------
// Name   : cIndexStream::read
// Usage  : Read cIndexStream from file
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         h                X   X   File handler
//
// Return : 0 == OK / -1 == read error / -2 memory error
//-----------------------------------------------------------------------------
int cIndexStream::read( FILE * h )
{
    int rtn = 0;

	indexes.clear();

    cStream::read( h );

//     fseek( h, -8, SEEK_CUR ); // 8 bytes backward...

    _filePos = ftell( h );

    if( dwNum ) {
		ushort index;

        for( uint i = 0; 0 == rtn && i < dwNum; ++i ) {
            if( 1 != fread( &index, sizeof(index), 1, h )) rtn = -1;
			if( 0 == rtn ) indexes.push_back( index );
        } // End for
    } // End if

    return( rtn );
} // End proc cIndexStream::read

//-----------------------------------------------------------------------------
// Name   : cIndexStream::write
// Usage  : Write cIndexStream to file
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         h                X   X   File handler
//
// Return : 0 == OK / -1 == write error / -2 memory error
//-----------------------------------------------------------------------------
int cIndexStream::write( FILE * h )
{
	int rtn = 0;

	rtn = cStream::write( h );

	if( 0 == rtn && dwNum ) {
		if( 0 == indexes.size() ) return( -2 );

		for( uint i = 0; 0 == rtn && i < dwNum; ++i ) {
			if( 1 != fwrite( &indexes[i], sizeof(indexes[i]), 1, h )) rtn = -1;
		} // End for
	} // End if

	return( rtn );
} // End proc cIndexStream::write

//-----------------------------------------------------------------------------
// Name   : cIndexStream::show
// Usage  : Dump cIndexStream to screen
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         n                X   -   Stream index
//         tab              X   -   (Optional) number of tabs before printing
//         where            X   -   (Optional) where to send data
//
// Return : Nothing
//-----------------------------------------------------------------------------
//
void cIndexStream::show( const uint n, const uint tab, FILE *where )
{
	const char * myTab = TABS(tab);
	FILE *out = ( where ? where : stdout );

	fprintf( out,
				"%s{ -- Index stream #%d @ 0x%08x\n", myTab, n, _filePos );
	fprintf( out, "%s\tdwType   = 0x%08x\n", myTab, dwType );
	fprintf( out, "%s\tdwZero   = %d\n",     myTab, dwZero );
	fprintf( out, "%s\tdwNum    = 0x%08x (%d)\n", myTab, dwNum, dwNum );
//  fprintf( out, "%s\tdwStride = 0x%08x (%d)\n", myTab, dwStride, dwStride);
//  fprintf( out, "%s\tdwFormat = 0x%08x (%d)\n", myTab, dwFormat, dwFormat);
	fprintf( out, "%s\t{\n",  myTab);
	fprintf( out, "%s\t\t", myTab);
	for( uint i = 0; i < dwNum; ++i ) {
		if( i && 0 == ( i % 10 )) {
			fprintf( out, " -- #%d\n", (((i-1) / 10)*10) );
			fprintf( out, "%s\t\t", myTab);
		}
		fprintf( out, "0x%04x ", indexes[i] );
	} // End for
	fprintf( out, "\n" );
	fprintf( out, "%s\t},\n",  myTab);
	fprintf( out, "%s}, -- End Index stream #%d\n", myTab, n );
	fflush( out );
} // End proc cIndexStream::show

//-----------------------------------------------------------------------------
// Name   : cUnknownStream::read
// Usage  : Read cUnknownStream from file
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         h                X   X   File handler
//
// Return : 0 == OK / -1 == read error / -2 memory error
//-----------------------------------------------------------------------------
int cUnknownStream::read( FILE * h )
{
	int rtn = 0;

	unknowns.clear();

	cStream::read( h );

//     fseek( h, -4, SEEK_CUR ); // 4 bytes backward...

	_filePos = ftell( h );

	if( dwNum ) {
		cUnknown unk;

		for( uint i = 0; 0 == rtn && i < dwNum; ++i ) {
			rtn = unk.read( h, dwFormat, _filePos, i );
			if( 0 == rtn ) unknowns.push_back( unk );
		} // End for
	} // End if

	return( rtn );
} // End proc cUnknownStream::read

//-----------------------------------------------------------------------------
// Name   : cUnknownStream::write
// Usage  : Write cUnknownStream to file
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         h                X   X   File handler
//
// Return : 0 == OK / -1 == write error / -2 memory error
//-----------------------------------------------------------------------------
int cUnknownStream::write( FILE * h )
{
	int rtn = 0;

	rtn = cStream::write( h );

	if( 0 == rtn && dwNum ) {
		if( 0 == unknowns.size() ) return( -2 );

		for( uint i = 0; 0 == rtn && i < dwNum; ++i ) {
			rtn = unknowns[i].write( h, dwFormat, _filePos );
		} // End for
	} // End if

	return( rtn );
} // End proc cUnknownStream::write

//-----------------------------------------------------------------------------
// Name   : cUnknownStream::show
// Usage  : Dump cUnknownStream to screen
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         n                X   -   Stream index
//         tab              X   -   (Optional) number of tabs before printing
//         where            X   -   (Optional) where to send data
//
// Return : Nothing
//-----------------------------------------------------------------------------
//
void cUnknownStream::show( const uint n, const uint tab, FILE *where )
{
	const char * myTab = TABS(tab);
	FILE *out = ( where ? where : stdout );

	fprintf( out,
				"%s{ -- Unknown stream #%d @ 0x%08x\n", myTab, n, _filePos );
	fprintf( out, "%s\tdwType   = 0x%08x\n", myTab, dwType );
	fprintf( out, "%s\tdwZero   = %d\n",     myTab, dwZero );
	fprintf( out, "%s\tdwNum    = 0x%08x (%d)\n", myTab, dwNum, dwNum );
	fprintf( out, "%s\tdwStride = 0x%08x (%d)\n", myTab, dwStride, dwStride);
//  fprintf( out, "%s\tdwFormat = 0x%08x (%d)\n", myTab, dwFormat, dwFormat);
	for( uint i = 0; i < dwNum; ++i ) {
		unknowns[i].show( dwFormat, tab + 1, where );
	} // End for
	fprintf( out, "%s}, -- End Unknown stream #%d\n", myTab, n );
	fflush( out );
} // End proc cUnknownStream::show

//-----------------------------------------------------------------------------
// Name   : cUnknown::read
// Usage  : Read cUnknown from file
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         h                X   X   File handler
//         format           X   -   Vertex format
//         basePos          X   -   Offset of 1st item in list
//         idx              X   -   (Optiona) material index
//
// Return : 0 == OK / -1 == read error / -2 == id error
//-----------------------------------------------------------------------------
int cUnknown::read( FILE * h, const uint format,
                   const uint basePos, const uint idx )
{
	int rtn = -1;

	do {
		_index        = idx;
		_offsetStream = ftell( h ) - basePos;

		if( 0 != ( rtn = pos.read( h ))) break;
		if( 0 != ( rtn = normal.read( h ))) break;
		if( 1 != fread( &dwGroup, sizeof(dwGroup), 1, h )) {rtn = -1; break;}

		rtn = 0;
	} while(0);

	return( rtn );
} // End proc cUnknown::read

//-----------------------------------------------------------------------------
// Name   : cUnknown::write
// Usage  : Write cUnknown to file
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         h                X   X   File handler
//         format           X   -   Vertex format
//         basePos          X   -   Offset of 1st item in list
//
// Return : 0 == OK / -1 == write error / -2 == id error
//-----------------------------------------------------------------------------
// TODO: basePos useless ??
int cUnknown::write( FILE * h, const uint format, const uint basePos )
{
	int rtn = -1;

	do {
		if( 0 != ( rtn = pos.write( h ))) break;
		if( 0 != ( rtn = normal.write( h ))) break;
		if( 1 != fwrite( &dwGroup, sizeof(dwGroup), 1, h )) {rtn = -1; break;}

		rtn = 0;
	} while(0);

	return( rtn );
} // End proc cUnknown::write

//-----------------------------------------------------------------------------
// Name   : cUnknown::show
// Usage  : Dump cUnknown to screen
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         format           X   -   Vertex format
//         tab              X   -   (Optional) number of tabs before printing
//         where            X   -   (Optional) where to send data
//
// Return : Nothing
//-----------------------------------------------------------------------------
void cUnknown::show( const uint format, const uint tab, FILE *where )
{
	const char * myTab = TABS(tab);
	FILE *out = ( where ? where : stdout );

	fprintf( out, "%s{\n", myTab );

	fprintf( out, "%s\t?x  = 0x%08x (%g)\n", myTab, *((int*)&pos.x), pos.x );
	fprintf( out, "%s\t?y  = 0x%08x (%g)\n", myTab, *((int*)&pos.y), pos.y );
	fprintf( out, "%s\t?z  = 0x%08x (%g)\n", myTab, *((int*)&pos.z), pos.z );

	fprintf( out, "%s\t?Nx = 0x%08x (%g)\n", myTab, *((int*)&normal.x), normal.x );
	fprintf( out, "%s\t?Ny = 0x%08x (%g)\n", myTab, *((int*)&normal.y), normal.y );
	fprintf( out, "%s\t?Nz = 0x%08x (%g)\n", myTab, *((int*)&normal.z), normal.z );

//     fprintf( out, "%s\t", myTab );
//     pos.show( "?", out );
//     fprintf( out, "\n" );

//     fprintf( out, "%s\t", myTab );
//     normal.show( "?N", out );
//     fprintf( out, "\n" );

	fprintf( out, "%s\t?dwGroup = 0x%08x\n", myTab, dwGroup );

	fprintf( out,
			"%s},  -- #%d @ offset 0x%08x\n", myTab, _index, _offsetStream );
	fflush( out );
} // End proc cUnknown::show

//-----------------------------------------------------------------------------
// Name   : cSurfaceList::clear
// Usage  : Clear surfaces in list
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         modelsOnly       X   -   Clear only model surfaces
//
// Return : Nothing
//-----------------------------------------------------------------------------
void cSurfaceList::clear( const bool modelsOnly )
{
	_curLayer = -1;
	if( !modelsOnly ) {
		surfaces.clear();
		dwNSurface = 0;
	} else {
		vector<cSurface>::iterator surf;
		for( surf = surfaces.begin(); surf != surfaces.end(); surf ++ ) {
			if( STREAM_VERTEX_MODEL == surf->dwFormat ) break;
		}
		if( surf != surfaces.end() ) surfaces.erase( surf, surfaces.end() );
		dwNSurface = surfaces.size();
	}
} // End proc cSurfaceList::clear

//-----------------------------------------------------------------------------
// Name   : cSurfaceList::read
// Usage  : Read cSurfaceList from file
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         h                X   X   File handler
//         sList            X   -   Stream list
//
// Return : 0 == OK / -1 == read error / -2 memory error
//-----------------------------------------------------------------------------
int cSurfaceList::read( FILE * h, cStreamList &sList )
{
	int rtn = 0;
	cSurface *pS;

	surfaces.clear(); dwNSurface = 0;

	if( 1 != fread( &dwNSurface, sizeof(dwNSurface), 1, h )) return( -1 );

	if( dwNSurface ) {
		surfaces.reserve( dwNSurface );
		if( surfaces.capacity() < dwNSurface ) return( -2 );

		for( uint i = 0; 0 == rtn && i < dwNSurface; ++i ) {
			pS = new cSurface;
			if( pS ) {
				rtn = pS->read( h, sList, i );
				if( 0 == rtn ) {
					pS->_index = surfaces.size();
					surfaces.push_back( *pS );
				}
				delete pS;
			} else {
				rtn = -2;
			}
		} // End for
	} // End if

	return( rtn );
} // End proc cSurfaceList::read

//-----------------------------------------------------------------------------
// Name   : cSurfaceList::write
// Usage  : Write cSurfaceList to file
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         h                X   X   File handler
//
// Return : 0 == OK / -1 == write error / -2 memory error
//-----------------------------------------------------------------------------
int cSurfaceList::write( FILE * h )
{
	int rtn = 0;

	if( 1 != fwrite( &dwNSurface, sizeof(dwNSurface), 1, h )) return( -1 );

	if( dwNSurface ) {
		if( dwNSurface != surfaces.size() ) return( -2 );

		for( uint i = 0; 0 == rtn && i < dwNSurface; ++i ) {
			rtn = surfaces[i].write( h );
		} // End for
	} // End if

	return( rtn );
} // End proc cSurfaceList::write

#if !defined(NO_BLENDER)
//-----------------------------------------------------------------------------
// Name   : cSurfaceList::linkMaterials
// Usage  : Make links to Blender materials
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         track            X   -   Track object surface list belongs to
//
// Return : 0 == OK / -1 == error / 1 == warning
//-----------------------------------------------------------------------------
int cSurfaceList::linkMaterials( cTrack& track )
{
	int rtn = 0;

	for( uint i=0; 0 == rtn && i < dwNSurface; ++i ) {
		rtn = surfaces[i].linkMaterials( track );
	}

	return( rtn );
} // End proc cSurfaceList::linkMaterials
#endif // NO_BLENDER

#if !defined(NO_BLENDER)
//-----------------------------------------------------------------------------
// Name   : cSurfaceList::toBlender
// Usage  : Draw in Blender
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         track            X   -   Track object surfaceList belongs to
//
// Return : 0 in case of success, -1 otherwise
//-----------------------------------------------------------------------------
int cSurfaceList::toBlender( cTrack& track )
{
    uint surfIdx = 0;  // Surface index
    int  rtn     = 0;

    for( surfIdx = 0; 0 == rtn && surfIdx < dwNSurface; ++surfIdx ) {
        rtn = surfaces[surfIdx].toBlender( track );
    } // End for

    return( rtn );
} // End proc cSurfaceList::toBlender
#endif //NO_BLENDER

#if !defined(NO_BLENDER)
//-----------------------------------------------------------------------------
// Name   : cSurfaceList::fromBlender
// Usage  : Get surface list data from Blender
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         track            X   -   Track object surface list belongs to
//         modelsOnly       X   -   Process only models flag
//
// Return : 0 == OK / -1 == error / 1 == warning
//-----------------------------------------------------------------------------
int cSurfaceList::fromBlender( cTrack& track, const bool modelsOnly )
{
	int rtn = 0;
	Object *ob = NULL;
	cStream *pUnkStream = NULL;
	int i = 0;

	// Mark all objects as unexported
	for( ob = (Object*)G.main->object.first; ob; ob = (Object*)ob->id.next ) {
		ob->par1 = 0;  // Export flag
		ob->par2 = -1; // Surface index
		ob->par3 = -1; // Model index
		ob->par4 = -1; // boundingbox index
		ob->par5 =  0; // 'Is damage' (1<<0), 'Is child' (1<<1) or 'Is LOD' (1<<2) flag
	}

	// Save unknown stream for later use
	if( !modelsOnly ) {
		for( i = 0; i < track.streamList.dwNstreams; ++i ) {
			if( STREAM_TYPE_UNKNOWN == track.streamList.streams[i]->dwType ) {
				pUnkStream = track.streamList.streams[i];
				track.streamList.streams[i] = NULL;
				break;
			}
		} // End for
	}

	// Delete all streams and related data
	track.streamList.clear( &track, modelsOnly );		// Delete streams
	clear( modelsOnly );						// Delete surfaces
	if( !modelsOnly ) track.surfcenterList.clear();	// Delete surface centers

	// Export surfaces (Mesh objects), by layer
	for( uint l = 0; rtn >= 0 && l < LAYER_MAX; ++l ) {
#ifdef WITH_FO2_DBG
		printMsg( MSG_DBG, "Exporting layer#%d (modelsOnly=%s):\n", l+1, (modelsOnly ? "Y" : "N" ));
#endif // WITH_FO2_DBG

		for( ob = (Object*)G.main->object.first;
				rtn >= 0 && NULL != ob;
				ob = (Object*)ob->id.next ) {
			if( !ob->data || NULL == ob->data || ID_ME != GS(((ID*)ob->data)->name) ) continue; // Meshes only
			if( ob->par1 || 0 == (ob->lay & (1<<l))) continue; // Skip if already processed or if not in layer
			if( OB_FO_NONE == ob->FOdynFlag || OB_FO_DYNAMIC == ob->FOdynFlag ) continue;
			if( modelsOnly && OB_FO_MODEL != ob->FOdynFlag ) continue;

			// Update damage/child/lod flag
			if( OB_FO_MODEL == ob->FOdynFlag ) {
				char *pSuffix = strrchr( ob->id.name, '_' );
				if( pSuffix && 0 == strncasecmp( pSuffix, MODEL_LOD1_SUFFIX, MODEL_LOD_SUFFIX_LG )) {
					ob->par5 |= MODEL_IS_LOD;
				}
				if( ob->damage ) ob->damage->par5 |= MODEL_IS_DAMAGE;
				if( ob->parent ) ob->par5 |= MODEL_IS_CHILD;
			}

			// Add surface
			cSurface surf;
			surf._index = surfaces.size();
			rtn = surf.fromBlender( ob, track );
			if( rtn < 0 ) break;
			surfaces.push_back( surf );
			dwNSurface++;

			// Add surface center, if not model
			if( OB_FO_MODEL != ob->FOdynFlag ) {
				float min[3],max[3];
				cSurfCenter sc;

				//sc.dwIdx1 = sc.dwIdx2 = sc._index = track.surfcenterList.surfcenters.size();
				//sc.dwSurfIdx = surf._index;
				sc.dwIdx1 = sc.dwIdx2 = surf._index;
				sc.dwSurfIdx = 0;

				INIT_MINMAX(min, max);
				minmax_object(ob, min, max);
				sc.posBBox.x = (max[0]-min[0])/2;
				sc.posBBox.y = (max[1]-min[1])/2;
				sc.posBBox.z = (max[2]-min[2])/2;
				sc.posWrld.x = min[0] + sc.posBBox.x;
				sc.posWrld.y = min[1] + sc.posBBox.y;
				sc.posWrld.z = min[2] + sc.posBBox.z;

				track.surfcenterList.surfcenters.push_back( sc );
				track.surfcenterList.dwNSurfCenter++;
			}

			// Mark object as processed
			ob->par1 = 1;
		}
	} // End for (l)

	// Put back unknown stream
	if( !modelsOnly ) {
		track.streamList.streams.push_back( pUnkStream );
		track.streamList.dwNstreams++;
	}

	// Fix material indexes
	if( 0 == rtn && modelsOnly ) {
		vector<cSurface>::iterator s;
		for( s = surfaces.begin(); 0 == rtn && s != surfaces.end(); ++s ) {
			// Not for models...
			if( STREAM_VERTEX_MODEL != s->dwFormat ) rtn = s->fixMaterial( track );
		}
	}

#if 0
	for( uint i=0; 0 == rtn && i < dwNSurface; ++i ) {
		rtn = surfaces[i].fromBlender( track );
	}

	// TODO TODO: REMOVEME
	uint nbObj = 0;
	uint nbStatic = 0;
	uint nbDynamic = 0;
	uint nbModel = 0;
	for( ob = (Object*)G.main->object.first;
			0 == rtn && NULL != ob;
			ob = (Object*)ob->id.next ) {
		if( !ob->data || NULL == ob->data || ID_ME != GS(((ID*)ob->data)->name) ) continue; // Meshes only

		++nbObj;
		switch( ob->FOdynFlag ) {
			case OB_FO_MODEL:
				++nbModel;
				break;
			case OB_FO_STATIC:
				++nbStatic;
				break;
			case OB_FO_DYNAMIC:
				++nbDynamic;
				break;
		}
	}
	printMsg( MSG_CONSOLE, "\n\nNb objects : %d\n"
		        "Nb Static  : %d\n"
		        "Nb Dynamic : %d\n"
		        "Nb Model   : %d\n\n",
				nbObj, nbStatic, nbDynamic, nbModel );

	// Export test of cone model
	for( ob = (Object*)G.main->object.first;
			0 == rtn && NULL != ob;
			ob = (Object*)ob->id.next ) {
		if( OB_FO_MODEL == ob->FOdynFlag && 0 == strcmp( ob->id.name+2, "_dyn_cone_small_00" )) {
			cSurface *surf = &surfaces[279];
			Mesh *me = get_mesh(ob);

			cVertexStream *vS = (cVertexStream*)track.streamList.streams[surf->dwVStreamIdx];
			//int base = 0;
			//for( base = 0; base < vS->dwNum; ++base ) {
			//    if( vS->vertices[base]._offsetStream == surf->dwVStreamOffset ) break;
			//} // End for
			int base = surf->_VStreamOffset;
			for( int i=0; i < me->totvert; ++i ) {
				cVertex *v = &vS->vertices[base+i];
				MVert *mv = &me->mvert[i];
				v->pos.x = mv->co[0];
				v->pos.y = mv->co[1];
				v->pos.z = mv->co[2];
				v->normal.x = mv->no[0]/32767.0;
				v->normal.y = mv->no[1]/32767.0;
				v->normal.z = mv->no[2]/32767.0;
			}
		}
	} // End for
	// TODO TODO: REMOVEME
#endif // 0

	return( rtn );
} // End proc cSurfaceList::fromBlender
#endif // NO_BLENDER

//-----------------------------------------------------------------------------
// Name   : cSurfaceList::show
// Usage  : Dump cSurfaceList to screen
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         tab              X   -   (Optional) number of tabs before printing
//         where            X   -   (Optional) where to send data
//
// Return : Nothing
//-----------------------------------------------------------------------------
//
void cSurfaceList::show( const uint tab, FILE *where )
{
    const char * myTab = TABS(tab);
    FILE *out = ( where ? where : stdout );

    fprintf( out,
             "%sSurfaces = { -- NB surfaces: %d\n", myTab, dwNSurface );
    for( uint i = 0; i < dwNSurface; ++i ) {
        surfaces[i].show( tab + 1, where );
    } // End for
    fprintf( out, "%s}, -- End Surfaces\n", myTab );
	fflush( out );
} // End proc cSurfaceList::show

//-----------------------------------------------------------------------------
// Name   : cSurface::cSurface
// Usage  : cSurface class constructor
// Args   : None
// Return : Nothing
//-----------------------------------------------------------------------------
cSurface::cSurface()
{
    dwIsNotMesh     = 0;
    dwMatIdx        = 0;
    dwVertNum       = 0;
    dwFormat        = 0;
    dwPolyNum       = 0;
    dwPolyMode      = 0;
    dwPolyNumIndex  = 0;
    dwStreamUse     = 0;
    dwVStreamIdx    = 0;
    dwVStreamOffset = 0;
    dwIStreamIdx    = 0;
    dwIStreamOffset = 0;

    _index   = 0;
    _filePos = 0;
	_VStreamOffset = 0;
	_IStreamOffset = 0;

#if !defined(NO_BLENDER)
    _obj	= NULL;
	_mat	= NULL;
#endif // !NO_BLENDER

} // End proc cSurface::cSurface

//-----------------------------------------------------------------------------
// Name   : cSurface::read
// Usage  : Read cSurface from file
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         h                X   X   File handler
//         sList            X   -   Stream list
//         idx              X   -   (Optiona) surface index
//
// Return : 0 == OK / -1 == read error / -2 == id error
//-----------------------------------------------------------------------------
int cSurface::read( FILE * h, cStreamList &sList, const uint idx )
{
    int rtn = -1;

    do {
        _index    = idx;
        _filePos  = ftell( h );

        if(1 != fread( &dwIsNotMesh,     sizeof(dwIsNotMesh), 1, h ))    break;
        if(1 != fread( &dwMatIdx,        sizeof(dwMatIdx), 1, h ))       break;
        if(1 != fread( &dwVertNum,       sizeof(dwVertNum), 1, h ))      break;
        if(1 != fread( &dwFormat,        sizeof(dwFormat), 1, h ))       break;
        if(1 != fread( &dwPolyNum,       sizeof(dwPolyNum), 1, h ))      break;
        if(1 != fread( &dwPolyMode,      sizeof(dwPolyMode), 1, h ))     break;
        if(1 != fread( &dwPolyNumIndex,  sizeof(dwPolyNumIndex), 1, h )) break;
        if(1 != fread( &dwStreamUse,     sizeof(dwStreamUse), 1, h ))    break;
        if(1 != fread( &dwVStreamIdx,    sizeof(dwVStreamIdx), 1, h ))   break;
        if(1 != fread( &dwVStreamOffset, sizeof(dwVStreamOffset), 1, h)) break;
        if( 0 == dwIsNotMesh ) {
			cVertexStream *vList = NULL;

            if( 1 != fread( &dwIStreamIdx,
                            sizeof(dwIStreamIdx), 1, h ))   break;
            if( 1 != fread( &dwIStreamOffset,
                            sizeof(dwIStreamOffset), 1, h)) break;

			// Index of 1st vertex in stream
			vList = (cVertexStream*)sList.streams[dwVStreamIdx];
			if( !vList ) break;
			for( _VStreamOffset = 0; _VStreamOffset < vList->dwNum; ++_VStreamOffset ) {
				if( dwVStreamOffset == vList->vertices[_VStreamOffset]._offsetStream ) break;
			}
			if( _VStreamOffset == vList->dwNum ) { _VStreamOffset = 0; break; }

			// Index of 1st index in stream
			_IStreamOffset = dwIStreamOffset / sizeof(ushort);
		} else {

			cUnknownStream *uList = NULL;

			// Index of 1st unknown in stream
			uList = (cUnknownStream*)sList.streams[dwVStreamIdx];
			if( !uList ) break;
			for( _VStreamOffset = 0; _VStreamOffset < uList->dwNum; ++_VStreamOffset ) {
				if( dwVStreamOffset == uList->unknowns[_VStreamOffset]._offsetStream ) break;
			}
			if( _VStreamOffset == uList->dwNum ) { _VStreamOffset = 0; break; }
		}

        rtn = 0;
    } while(0);

    return( rtn );
} // End proc cSurface::read

//-----------------------------------------------------------------------------
// Name   : cSurface::write
// Usage  : Write cSurface to file
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         h                X   X   File handler
//
// Return : 0 == OK / -1 == write error / -2 == id error
//-----------------------------------------------------------------------------
int cSurface::write( FILE * h )
{
    int rtn = -1;

    do {
        if(1 != fwrite( &dwIsNotMesh,     sizeof(dwIsNotMesh), 1, h ))   break;
        if(1 != fwrite( &dwMatIdx,        sizeof(dwMatIdx), 1, h ))      break;
        if(1 != fwrite( &dwVertNum,       sizeof(dwVertNum), 1, h ))     break;
        if(1 != fwrite( &dwFormat,        sizeof(dwFormat), 1, h ))      break;
        if(1 != fwrite( &dwPolyNum,       sizeof(dwPolyNum), 1, h ))     break;
        if(1 != fwrite( &dwPolyMode,      sizeof(dwPolyMode), 1, h ))    break;
        if(1 != fwrite( &dwPolyNumIndex,  sizeof(dwPolyNumIndex), 1, h ))break;
        if(1 != fwrite( &dwStreamUse,     sizeof(dwStreamUse), 1, h ))   break;
        if(1 != fwrite( &dwVStreamIdx,    sizeof(dwVStreamIdx), 1, h ))  break;
        if(1 != fwrite( &dwVStreamOffset, sizeof(dwVStreamOffset), 1, h))break;
        if( 0 == dwIsNotMesh ) {
            if( 1 != fwrite( &dwIStreamIdx,
                             sizeof(dwIStreamIdx), 1, h ))   break;
            if( 1 != fwrite( &dwIStreamOffset,
                             sizeof(dwIStreamOffset), 1, h)) break;
        }
        rtn = 0;
    } while(0);

    return( rtn );
} // End proc cSurface::write

#if !defined(NO_BLENDER)
//-----------------------------------------------------------------------------
// Name   : cSurface::linkMaterials
// Usage  : Make links to Blender materials
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         track            X   -   Track object surface belongs to
//
// Return : 0 == OK / -1 == error / 1 == warning
//-----------------------------------------------------------------------------
int cSurface::linkMaterials( cTrack& track )
{
	int rtn = 0;
	_mat = NULL;

	if( dwMatIdx < track.materialList.materials.size() ) {
		_mat = findMaterial( track.materialList.materials[dwMatIdx].sName.c_str() );
	}
	if( NULL == _mat ) {
		rtn = 1;
		printMsg( MSG_ALL, "[WNG] Cannot find Blender material for surface #%d\n", _index );
	}

	return( rtn );
} // End proc cSurface::linkMaterials
#endif //NO_BLENDER

#if !defined(NO_BLENDER)
//-----------------------------------------------------------------------------
// Name   : cSurface::toBlender
// Usage  : Draw in Blender
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         track            X   -   Track object surface belongs to
//
// Return : 0 in case of success, -1 otherwise
//-----------------------------------------------------------------------------
int cSurface::toBlender( cTrack& track )
{
    int  rtn     = 0;

    cIndexStream  *iS    = NULL;  // Index stream
    cVertexStream *vS    = NULL;  // Vertex stream
    MVert  *vertdata     = NULL;  // Vertex array
    MFace  *facedata     = NULL;  // Face array
    MTFace *tfacedata[2] = { NULL, NULL };  // Texture array

	do {
#ifdef WITH_FO2_DBG
		printMsg( MSG_DBG, "Processing surface #%d\n", _index );
#endif // WITH_FO2_DBG

		iS = (cIndexStream*)track.streamList.streams[ dwIStreamIdx ];
		vS = (cVertexStream*)track.streamList.streams[ dwVStreamIdx ];
		if( NULL == iS || NULL == vS ) { errLine = __LINE__; rtn = -1; break; }

//      printMsg( MSG_DBG, "iS->dwType=%d  vS->dwType=%d\n", iS->dwType, vS->dwType );

		if( 0 == dwIsNotMesh &&
			STREAM_TYPE_INDEX  == iS->dwType &&
			STREAM_TYPE_VERTEX == vS->dwType ) {

			uint vO = _VStreamOffset;  // Index of surface first vertex in vertex list

//                     pSurface->show( 0, dbg );
//                     vS->show(0, 0, dbg );
//                     printMsg( MSG_DBG, "pSurface->dwVStreamOffset=%d\n",
//                              pSurface->dwVStreamOffset);

#ifdef WITH_FO2_DBG
			printMsg( MSG_DBG,
					"\t VStreamOffset %d (0x%08x)\n"
					"\tindex  stream 0x%08x\n"
					"\tvertex stream 0x%08x\n"
					"\tvertex index  %d (_offsetStream=0x%08x)\n",
					dwVStreamOffset, dwVStreamOffset,
					iS, vS, vO, vS->vertices[vO]._offsetStream );
#endif // WITH_FO2_DBG

			Object *ob;
			Mesh   *me;

			ob = add_object( OB_MESH );
			me = get_mesh(ob);
			me->totvert = dwVertNum;
			me->mvert   = (MVert*)CustomData_add_layer(&me->vdata,
														CD_MVERT,
														CD_DEFAULT,
														NULL, me->totvert);
			if(NULL == me->mvert) {errLine = __LINE__; rtn = -1; break;}

			me->totface = dwPolyNum; // faceIdx; TODO: BLENDER243 CHECK
			me->mface   = (MFace*)CustomData_add_layer(&me->fdata,
														CD_MFACE,
														CD_DEFAULT,
														NULL, me->totface);
			if(NULL == me->mface) {errLine = __LINE__; rtn = -1; break;}

			// UV coords?
			if( vS->dwFormat & (VERTEX_UV | VERTEX_UV2) ) {
				tfacedata[0]=(MTFace*)CustomData_add_layer_named(&me->fdata,
																	CD_MTFACE,
																	CD_DEFAULT,
																	NULL,
																	me->totface,
																	TEXTURE1_NAME);
				if(NULL == tfacedata[0]) {errLine = __LINE__; rtn = -1; break;}
			} else {
				tfacedata[0] = NULL;
			}

			// 2nd set of UV coords?
			if( vS->dwFormat & VERTEX_UV2 ) {
				tfacedata[1]=(MTFace*)CustomData_add_layer_named(&me->fdata,
																	CD_MTFACE,
																	CD_DEFAULT,
																	NULL,
																	me->totface,
																	TEXTURE2_NAME);
				if(NULL == tfacedata[1]) {errLine = __LINE__; rtn = -1; break;}
			} else {
				tfacedata[1] = NULL;
			}

			vertdata   = me->mvert;
			facedata   = me->mface;

			CustomData_set_layer_active(&me->fdata, CD_MTFACE, 0);
			mesh_update_customdata_pointers(me);

#ifdef WITH_FO2_DBG
			printMsg( MSG_DBG,
					"\tobject       0x%08x\n"
					"\tmesh         0x%08x\n"
					"\tvertdata     0x%08x\n"
					"\tfacedata     0x%08x\n"
					"\ttfacedata[0] 0x%08x\n"
					"\ttfacedata[1] 0x%08x\n",
					ob, me, vertdata, facedata,
					tfacedata[0], tfacedata[1] );
#endif // WITH_FO2_DBG

#ifdef WITH_FO2_DBG
			printMsg( MSG_DBG, "\t-> Adding %d vertices\n", dwVertNum );
#endif // WITH_FO2_DBG

			// -------------------------------------------------------
			// -- Add vertices
			// -------------------------------------------------------
			for( uint i = 0; i < dwVertNum; ++i ) {
				cVertex *pVertex = &vS->vertices[ vO + i ];

				// Vertex coord (z & y invertion made while reading)
				vertdata[i].co[0] = pVertex->pos.x;
				vertdata[i].co[1] = pVertex->pos.y;
				vertdata[i].co[2] = pVertex->pos.z;

				// Vertex normal (z & y invertion made while reading)
				vertdata[i].no[0] = (short)(32767 * pVertex->normal.x);
				vertdata[i].no[1] = (short)(32767 * pVertex->normal.y);
				vertdata[i].no[2] = (short)(32767 * pVertex->normal.z);

//						printMsg( MSG_DBG, "DBG[%d]: \tx=%f y=%f z=%f\n",
//								i,pVertex->pos.x,pVertex->pos.y,pVertex->pos.z);
			} // End for (i)

			// -------------------------------------------------------
			// -- Add polygons
			// -------------------------------------------------------
			uint n = _IStreamOffset; // Index of 1st index to use in list
			uint s[3];
			uint faceIdx = 0;
			uint flipNormals = 1;

#ifdef WITH_FO2_DBG
			printMsg( MSG_DBG, "\t-> Adding %d polygons\n", dwPolyNum );
#endif // WITH_FO2_DBG

			for( uint i = 0; i < dwPolyNum; ++i ) {
				s[0] = iS->indexes[n]   - vO;
				s[1] = iS->indexes[n+1] - vO;
				s[2] = iS->indexes[n+2] - vO;

				if( SURF_MODE_INDEX == dwPolyMode ) {
					n += 3;
				} else {
					n += 1;
				}
				if( s[0] != s[1] && s[1] != s[2] && s[2] != s[0] ) {
					if( SURF_MODE_STRIP == dwPolyMode && 0 == (n % 2)) {
						SWAP( uint, s[0], s[2] );
						if( 0 == i ) flipNormals = 0;
					}
					// Last index *MUST NOT* be zero. If so, rotate
					// indexes or surface will not be displayed

// TODO: use test_index_face (blenkernel/intern/mesh.c ??
/*rotates the vertices of a face in case v[2] or v[3] (vertex index) is = 0. */
/*
#define UVSWAP(t, s) { SWAP(float, t[0], s[0]); SWAP(float, t[1], s[1]); }
void test_index_face(MFace *mface, MCol *mc, TFace *tface, int nr)
*/

					if( 0 == s[2] ) {
						uint tmp = s[0];
						s[0] = s[1];
						s[1] = s[2];
						s[2] = tmp;
					}
					facedata[faceIdx].v1 = s[0];
					facedata[faceIdx].v2 = s[1];
					facedata[faceIdx].v3 = s[2];
					facedata[faceIdx].v4 = 0;  // Not used

					++faceIdx;
				} // End if

			} // End for (i)

			// Flip surface?
			if( flipNormals ) {
				for( uint i = 0; i < faceIdx; ++i ) {
					SWAP( uint, facedata[i].v1, facedata[i].v2 );
				}
			} // End if (flipNormals)

			// -------------------------------------------------------
			// -- Build edges
			// -------------------------------------------------------
			make_edges(me, 0);

			// --------------------------------------------------------
			// ---   Add Blender material
			// --------------------------------------------------------
			if( dwMatIdx >= track.materialList.materials.size() ) { SET_ERROR_AND_BREAK(-1,rtn); }
			Material *ma = findMaterial( track.materialList.materials[ dwMatIdx ].sName.c_str() );
			if( ma ) assign_material( ob, ma, ob->actcol );

#ifdef WITH_FO2_DBG
			printMsg( MSG_DBG, "\t-> Adding Blender material: %s (0x%08x)\n", (ma ? "OK" : "Not found"), ma );
#endif // WITH_FO2_DBG

			// Change object & mesh name
			char objName[ID_NAME_LG];
			snprintf( objName, sizeof(objName), DEF_OBJECT_NAME, _index );
			rename_id( &ob->id, objName );
			rename_id( &me->id, objName );

			// Save pointer to Blender object
			_obj = ob;

			// --------------------------------------------------------
			// ---   Move object to layer
			// --------------------------------------------------------
			if( track.layers ) sendToLayer( ob,
										track.layers[dwVStreamIdx] );

			// Assume all objects are static here,
			// (models and dynamic instance will be processed later)
			//ob->FOdynFlag = OB_FO_STATIC;
			switch( vS->dwFormat ) {
				case STREAM_VERTEX_DECAL:		ob->FOdynFlag = OB_FO_DECAL; break;
				case STREAM_VERTEX_MODEL:		ob->FOdynFlag = OB_FO_MODEL; break;
				case STREAM_VERTEX_TERRAIN:
				case STREAM_VERTEX_TERRAIN2:	ob->FOdynFlag = OB_FO_STATIC; break;
				case STREAM_VERTEX_STATIC:		ob->FOdynFlag = OB_FO_STATIC; break;
				case STREAM_VERTEX_WINDOW:		ob->FOdynFlag = OB_FO_WINDOW; break;
				default:
					printMsg( MSG_ALL, "[WNG] Unknown vertex stream format 0x%08x\n", vS->dwFormat );
					ob->FOdynFlag = OB_FO_STATIC;
					break;
			}

			// --------------------------------------------------------
			// -- Add uv coords for textured faces
			// --------------------------------------------------------
			cMaterial *pMaterial = NULL;
			uint       matIdx    = 0;
			uint       tIdx      = 0;  // Texture index
			const char *pTexName    = NULL;

			for( tIdx = 0; tIdx < 2; ++tIdx ) {
				if( tfacedata[tIdx] ) {
					cVertex *pVertex = NULL;

#ifdef WITH_FO2_DBG
					printMsg( MSG_DBG, "\t-> UV mapping\n" );
#endif // WITH_FO2_DBG

					// -------------------------------------------------------
					// -- Find textures
					// -------------------------------------------------------
					pTexName = track.materialList.materials[ dwMatIdx ].sTexture[tIdx].c_str();
#ifdef WITH_FO2_DBG
					printMsg( MSG_DBG, "\t\t-> Adding texture (%s) materialIdx=%d texture#%d\n",
							pTexName, dwMatIdx, tIdx );
#endif // WITH_FO2_DBG

					if( pTexName && *pTexName ) {
						for( matIdx = 0;
								matIdx < track.materialList.dwNmaterials;
								++matIdx) {
							pMaterial = &track.materialList.materials[matIdx];

							if( 0 == strcasecmp(pTexName, TEX_COLORMAP)) {
								pTexName = TEX_LIGHTMAP;
							}

							if(pMaterial->_img[tIdx] &&
								0==strcasecmp(pTexName,
												pMaterial->_img[tIdx]->id.name+2)) {
								break;
							}
						} // End for
#ifdef WITH_FO2_DBG
						printMsg( MSG_DBG, "\t\t-< Adding texture (0x%08X)\n", pMaterial );
#endif // WITH_FO2_DBG
					} // End if

					for( uint f = 0; f < faceIdx; ++f ) {
						MTFace * pMTFace = &tfacedata[tIdx][f];

						for( uint i = 0; i < 3; ++i ) {
							pVertex=(0 == i ?
										&vS->vertices[vO+facedata[f].v1] :
										(1 == i ?
										&vS->vertices[vO+facedata[f].v2] :
										&vS->vertices[vO+facedata[f].v3] ));

							pMTFace->uv[i][0] = pVertex->uv[tIdx].u;
							// Adjust v coord (reverse it for blender)
							pMTFace->uv[i][1] = 1 - pVertex->uv[tIdx].v;
						} // End for (i)

						pMTFace->tpage  = ( pMaterial ? pMaterial->_img[tIdx]: NULL );
						pMTFace->mode   = ( TF_TEX | TF_DYNAMIC );
						if( pMaterial ) {
							pMTFace->transp = (pMaterial->dwAlpha ? TF_ALPHA : TF_SOLID);
							if( TEXTURE_TILE == pMaterial->bUnknown[tIdx] ) pMTFace->mode |= TF_TILES;
						}
					} // End for(j)

#ifdef WITH_FO2_DBG
					printMsg( MSG_DBG, "\t-< UV mapping\n" );
#endif // WITH_FO2_DBG

				} // End if
			} // End for (tIdx)
#ifdef WITH_FO2_DBG
			printMsg( MSG_DBG,
						"totFace = %d  faceIdx = %d layer = 0x%08X\n",
						me->totface, faceIdx, ob->lay );
#endif // WITH_FO2_DBG

		} else if( 1 == dwIsNotMesh &&
					STREAM_TYPE_UNKNOWN == vS->dwType ) {

			cUnknownStream *uS = (cUnknownStream*)vS;
			uint uO = _VStreamOffset;  // Index of surface first unknown in unknwon list

#ifdef WITH_FO2_DBG
			printMsg( MSG_DBG,
					"\t UStreamOffset %d (0x%08x)\n"
					"\tindex   stream 0x%08x\n"
					"\tunknown stream 0x%08x\n"
					"\tunknown index  %d (_offsetStream=0x%08x)\n",
					dwVStreamOffset, dwVStreamOffset,
					iS, uS, uO, uS->unknowns[uO]._offsetStream );
			printMsg( MSG_DBG, "\tdwPolyNum=%d   dwVertNum = %d\n", dwPolyNum, dwVertNum );
#endif // WITH_FO2_DBG

			Object *ob;
			Mesh   *me;

			ob = add_object( OB_MESH );
			me = get_mesh(ob); //(Mesh*)ob->data;
			me->totvert = dwPolyNum;
			me->mvert   = (MVert*)CustomData_add_layer(&me->vdata,
														CD_MVERT,
														CD_DEFAULT,
														NULL, me->totvert);
			if(NULL == me->mvert) {errLine = __LINE__; rtn = -1; break;}

			vertdata   = me->mvert;

			for( uint i = 0; i < dwPolyNum; ++i ) {
				cUnknown *pUnk = &uS->unknowns[ uO + i ];
				// Vertex coord
				vertdata[i].co[0] = pUnk->pos.x;
//                      vertdata[i].co[1] = pVertex->pos.z;// y && z inverted!!
//                      vertdata[i].co[2] = pVertex->pos.y;
				vertdata[i].co[1] = pUnk->pos.y;
				vertdata[i].co[2] = pUnk->pos.z;
			} // End for (i)

			// --------------------------------------------------------
			// ---   Add Blender material
			// --------------------------------------------------------
			Material *ma = findMaterial( track.materialList.materials[ dwMatIdx ].sName.c_str() );
			if( ma ) assign_material( ob, ma, ob->actcol );

			// Change object & mesh name
			char objName[ID_NAME_LG];
			snprintf( objName, sizeof(objName), DEF_OBJECT_NAME, _index );
			rename_id( &me->id, objName );
			snprintf( objName, sizeof(objName), "Unknown%d", _index );
			rename_id( &ob->id, objName );

			// Save pointer to Blender object
			_obj = ob;

			// --------------------------------------------------------
			// ---   Move object to layer
			// --------------------------------------------------------
			if( track.layers ) sendToLayer( ob,
										track.layers[dwVStreamIdx] );

		} else {
			errLine = __LINE__;
			rtn = -1;
		} // End if
	} while(0);

    return( rtn );
} // End proc cSurface::toBlender
#endif //NO_BLENDER

#if !defined(NO_BLENDER)
//-----------------------------------------------------------------------------
// Name   : cSurface::fromBlender
// Usage  : Get surface data from Blender
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         ob               X   -   Pointer to Blender object
//         track            X   X   Track object surface belongs to
//
// Return : 0 == OK / -1 == error / 1 == warning
//-----------------------------------------------------------------------------
int cSurface::fromBlender( Object *ob, cTrack& track )
{
	int rtn = 0;
    cIndexStream  *iS    = NULL;  // Index stream
    cVertexStream *vS    = NULL;  // Vertex stream

	if( !ob ) return( 1 );

#ifdef WITH_FO2_DBG
	printMsg( MSG_DBG, "\tAdding surface #%d for object '%s'\n", _index, ob->id.name+2 );
#endif // WITH_FO2_DBG

	do {
		Mesh *me = get_mesh(ob);
		MTFace *tfacedata[2] = { NULL, NULL };  // Texture array
		Material *ma = give_current_material(ob, ob->actcol);
		int matIdx = track.materialList.indexOf( ma );
		uint UVmask = 0;
		uint vSformat = 0;
		uint i;

		if( !me ) {
			printMsg( MSG_ALL, "\tObject '%s' is not a Mesh\n", ob->id.name+2 );
			SET_ERROR_AND_BREAK(-1,rtn);
		}

		if( matIdx < 0 || matIdx >= track.materialList.materials.size() ) {
			printMsg( MSG_ALL, "\tCannot find material for object '%s'\n", ob->id.name+2 );
			SET_ERROR_AND_BREAK(-1,rtn);
		}

		// UV data
		tfacedata[0]=(MTFace*)CustomData_get_layer_named(&me->fdata, CD_MTFACE, TEXTURE1_NAME);
		tfacedata[1]=(MTFace*)CustomData_get_layer_named(&me->fdata, CD_MTFACE, TEXTURE2_NAME);
		if( tfacedata[0] || tfacedata[1] ) UVmask |= VERTEX_UV;
		if( tfacedata[0] && tfacedata[1] ) UVmask |= VERTEX_UV2;

		switch( ob->FOdynFlag ) {
			case OB_FO_MODEL:
				vSformat = STREAM_VERTEX_MODEL;
				break;
			case OB_FO_STATIC:
				vSformat = ((UVmask & VERTEX_UV2) ? STREAM_VERTEX_TERRAIN : STREAM_VERTEX_STATIC);
				break;
			case OB_FO_DECAL:
				vSformat = STREAM_VERTEX_DECAL;
				break;
			case OB_FO_WINDOW:
				vSformat = STREAM_VERTEX_WINDOW;
				break;
			default:
				printMsg( MSG_ALL, "Object '%s' has FOdynFlag=%d\n", ob->id.name+2, ob->FOdynFlag );
				SET_ERROR_AND_BREAK(-1,rtn);
		}
		if( rtn < 0 ) break;

		printMsg( MSG_DBG, "\t\tvSformat : 0x%08x  nb UV maps : %d   material#%d : %s\n",
			vSformat, (tfacedata[0]?1:0)+(tfacedata[1]?1:0), matIdx, (ma ? ma->id.name+2 : "None"));

		// Find vertex stream to use
		if( track.streamList._vIdx >= 0 ) {
			vS = (cVertexStream*)track.streamList.streams[ track.streamList._vIdx ];
			if( vS ) {
				if( vS->dwFormat != vSformat ) vS = NULL; // Force new VStream
				else if( (vS->vertices.size() + me->totvert) > 32767 )  vS = NULL; // Not enough space in stream
			}
		}

		// Find index stream to use
		iS = NULL;
		for( uint x = 0; x < track.streamList.streams.size(); ++x ) {
			if( track.streamList.streams[x] && STREAM_TYPE_INDEX == track.streamList.streams[x]->dwType ) {
				iS = (cIndexStream*)track.streamList.streams[x];
				if( (iS->indexes.size() + (me->totface * 3)) > 65535 ) {
					iS = NULL; // Not enough space in stream
				} else {
					break;
				}
			}
		}

		// New streams ?
		if( NULL == vS ) {
			vS = new cVertexStream(vSformat);
			if( NULL == vS ) {
				printMsg( MSG_ALL, "Memory allocation error" );
				SET_ERROR_AND_BREAK(-2,rtn);
			}
			track.streamList._vIdx = vS->_index = track.streamList.streams.size();
			track.streamList.streams.push_back( (cStream*)vS );
			track.streamList.dwNstreams++;
		}
		if( NULL == iS ) {
			iS = new cIndexStream();
			if( NULL == iS ) {
				printMsg( MSG_ALL, "Memory allocation error" );
				SET_ERROR_AND_BREAK(-2,rtn);
			}
			track.streamList._iIdx = iS->_index = track.streamList.streams.size();
			track.streamList.streams.push_back( (cStream*)iS );
			track.streamList.dwNstreams++;
		}

		// Compute surface
		_VStreamOffset  = vS->vertices.size();	// Index of 1st vertex in vertex stream
		_IStreamOffset  = iS->indexes.size();	// Index of 1st index in index stream
		dwIsNotMesh     = 0;				// Surface is a mesh? 0=Yes 1=No (does not use Index stream then)
		dwMatIdx        = matIdx;			// Index of material to use
		dwVertNum       = me->totvert;		// Number of vertices in surface
		dwFormat        = vSformat;			// Surface format
		dwPolyNum       = me->totface;		// Number of polygons (from index tables)
		dwPolyMode      = SURF_MODE_INDEX;	// Polygon mode: 4-triindx or 5-tristrip	// TODO: use strip?
		dwPolyNumIndex  = 3*me->totface;	// Number of indexes for polygon
		dwStreamUse     = 2;				// Number of streams used (1 or 2: Vstream/Istream)
		dwVStreamIdx    = vS->_index;		// Index of vertex stream to use
		dwVStreamOffset = _VStreamOffset * vS->dwStride;  // Offset to 1st vertex in vertex stream
		dwIStreamIdx    = iS->_index;		// Index of index stream to use
		dwIStreamOffset = _IStreamOffset * sizeof(short);  // Offset to 1st index in index stream
		_obj            = ob;
		ob->par2 = _index;

		// Add vertices
		cVertex vertex;
		MVert *v = NULL;
		for( i=0, v=me->mvert; i < me->totvert; ++i, v++ ) {
			vertex._index        = vS->vertices.size();
			vertex._offsetStream = vertex._index * vS->dwStride;
			if( dwFormat & VERTEX_POSITION ) {
				float pos[3];

				// If not model, get world coords
				VecCopyf(pos, v->co);
				if( OB_FO_MODEL != ob->FOdynFlag ) Mat4MulVecfl(ob->obmat, pos);
				vertex.pos.x = pos[0];
				vertex.pos.y = pos[1];
				vertex.pos.z = pos[2];
			}

			if( dwFormat & VERTEX_NORMAL ) {
				vertex.normal.x = (float)v->no[0] / 32767.0;
				vertex.normal.y = (float)v->no[1] / 32767.0;
				vertex.normal.z = (float)v->no[2] / 32767.0;
			}
			if( dwFormat & VERTEX_BLEND ) vertex.fBlend = 0.0; // TODO ??

			uint nbUV = (UVmask & VERTEX_UV ? 1 : 0) + (UVmask & VERTEX_UV2 ? 1 : 0);
			for( uint f = 0; f < nbUV; ++f ) {
				for( uint j=0; j < me->totface; ++j ) {
					int k= -1;
					if( me->mface[j].v1 == i ) k = 0;
					else if( me->mface[j].v2 == i ) k = 1;
					else if( me->mface[j].v3 == i ) k = 2;
					if( k >= 0 ) {
						vertex.uv[f].u = tfacedata[f][j].uv[k][0];
						vertex.uv[f].v = 1 - tfacedata[f][j].uv[k][1]; // Adjust v coord
						break;
					}
				}
			}
			vS->vertices.push_back( vertex );
			vS->dwNum++;
		} // End for

		// Add faces
		MFace *f = NULL;
		ushort idx[3];
		for( i=0, f=me->mface; i < me->totface; ++i, f++ ) {
			idx[0] = f->v1 + _VStreamOffset;
			idx[1] = f->v2 + _VStreamOffset;
			idx[2] = f->v3 + _VStreamOffset;
			if( SURF_MODE_INDEX ==dwPolyMode ) {
				// Flip face
				ushort tmp = idx[0];
				idx[0] = idx[1];
				idx[1] = tmp;
			}
			iS->indexes.push_back(idx[0]);
			iS->indexes.push_back(idx[1]);
			iS->indexes.push_back(idx[2]);
			iS->dwNum += 3;
		}

	} while(0);

	return( rtn );
} // End proc cSurface::fromBlender
#endif //NO_BLENDER

#if !defined(NO_BLENDER)
//-----------------------------------------------------------------------------
// Name   : cSurface::fixMaterial
// Usage  : Try to fix material index for surface (in case some materials were
//          added and not exporting all surfaces)
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         track            X   -   Track object surface list belongs to
//
// Return : 0 == OK / -1 == error / 1 == warning
//-----------------------------------------------------------------------------
int cSurface::fixMaterial( cTrack& track )
{
	int rtn = 0;
	int matIdx = -1;

	char name[ID_NAME_LG];
	Object *ob= NULL;
	Material *ma= NULL;

	// Search for object to get its actual material
	sprintf( name, DEF_OBJECT_NAME, _index );
	ob = findObjectMesh( name, name );
	if( ob ) {
		ma= give_current_material(ob, ob->actcol);
		if( ma ) {
			matIdx = track.materialList.indexOf( ma );
			if( matIdx < 0 ) {
				// Failed, try with old material
				matIdx = track.materialList.indexOf( _mat );
			}
			if( matIdx < 0 )
				rtn = -1;
			else
				dwMatIdx = matIdx;
		}
	}

	return( rtn );
} // End proc cSurface::fixMaterial
#endif //NO_BLENDER

//-----------------------------------------------------------------------------
// Name   : cSurface::show
// Usage  : Dump cSurface to screen
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         tab              X   -   (Optional) number of tabs before printing
//         where            X   -   (Optional) where to send data
//
// Return : Nothing
//-----------------------------------------------------------------------------
void cSurface::show( const uint tab, FILE *where )
{
    const char * myTab = TABS(tab);
    FILE *out = ( where ? where : stdout );

    fprintf( out, "%s{ -- surface #%d @ 0x%08x\n", myTab, _index, _filePos );

    fprintf( out, "%s\tdwIsNotMesh     = 0x%08x (%d)\n",
             myTab, dwIsNotMesh, dwIsNotMesh );
    fprintf( out, "%s\tdwMatIdx        = 0x%08x (%d)\n",
             myTab, dwMatIdx, dwMatIdx );
    fprintf( out, "%s\tdwVertNum       = 0x%08x (%d)\n",
             myTab, dwVertNum, dwVertNum );
    fprintf( out, "%s\tdwFormat        = 0x%08x (%d)\n",
             myTab, dwFormat, dwFormat );
    fprintf( out, "%s\tdwPolyNum       = 0x%08x (%d)\n",
             myTab, dwPolyNum, dwPolyNum );
    fprintf( out, "%s\tdwPolyMode      = 0x%08x (%d)\n",
             myTab, dwPolyMode, dwPolyMode );
    fprintf( out, "%s\tdwPolyNumIndex  = 0x%08x (%d)\n",
             myTab, dwPolyNumIndex, dwPolyNumIndex );
    fprintf( out, "%s\tdwStreamUse     = 0x%08x (%d)\n",
             myTab, dwStreamUse, dwStreamUse );
    fprintf( out, "%s\tdwVStreamIdx    = 0x%08x (%d)\n",
             myTab, dwVStreamIdx, dwVStreamIdx );
    fprintf( out, "%s\tdwVStreamOffset = 0x%08x (%d)\n",
             myTab, dwVStreamOffset, dwVStreamOffset );
    fprintf( out, "%s\t_VStreamOffset  = 0x%08x (%d)\n",
             myTab, _VStreamOffset, _VStreamOffset );
    if( 0 == dwIsNotMesh ) {
        fprintf( out, "%s\tdwIStreamIdx    = 0x%08x (%d)\n",
                 myTab, dwIStreamIdx, dwIStreamIdx );
        fprintf( out, "%s\tdwIStreamOffset = 0x%08x (%d)\n",
                 myTab, dwIStreamOffset, dwIStreamOffset );
		fprintf( out, "%s\t_IStreamOffset  = 0x%08x (%d)\n",
				myTab, _IStreamOffset, _IStreamOffset );
    }

    fprintf( out,
             "%s},\n", myTab );
	fflush( out );
} // End proc cSurface::show

//-----------------------------------------------------------------------------
// Name   : cSurfCenterList::read
// Usage  : Read surface center list from file
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         h                X   X   File handler
//
// Return : 0 == OK / -1 == read error / -2 memory error
//-----------------------------------------------------------------------------
int cSurfCenterList::read( FILE * h )
{
    int rtn = 0;
	cSurfCenter c;

	do {
		surfcenters.clear();

		if( 1 != fread( &dwNSurfCenter, sizeof(dwNSurfCenter), 1, h )) {
			rtn = -1;
			break;
		}

		if( dwNSurfCenter ) {
			surfcenters.reserve( dwNSurfCenter );
			if( surfcenters.capacity() < dwNSurfCenter ) {
                errLine = __LINE__;
                rtn = -2;
                break;
            }

			for( uint i = 0; 0 == rtn && i < dwNSurfCenter; ++i ) {
				rtn = c.read( h, i );
				if( 0 == rtn ) surfcenters.push_back( c );
			} // End for
            if( 0 != rtn )  break;
		} // End if
	} while(0);

    return( rtn );
} // End proc cSurfCenterList::read

//-----------------------------------------------------------------------------
// Name   : cSurfCenterList::write
// Usage  : Write surface center list to file
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         h                X   X   File handler
//
// Return : 0 == OK / -1 == write error / -2 memory error
//-----------------------------------------------------------------------------
int cSurfCenterList::write( FILE * h )
{
    int rtn = 0;

    if( 1 != fwrite( &dwNSurfCenter, sizeof(dwNSurfCenter), 1, h )) return( -1 );

    if( dwNSurfCenter ) {
        if( dwNSurfCenter != surfcenters.size() ) return( -2 );

        for( uint i = 0; 0 == rtn && i < dwNSurfCenter; ++i ) {
            rtn = surfcenters[i].write( h );
        } // End for
    } // End if

    return( rtn );
} // End proc cSurfCenterList::write

//-----------------------------------------------------------------------------
// Name   : cSurfCenterList::show
// Usage  : Dump surface center list to screen
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         tab              X   -   (Optional) number of tabs before printing
//         where            X   -   (Optional) where to send data
//
// Return : Nothing
//-----------------------------------------------------------------------------
//
void cSurfCenterList::show( const uint tab, FILE *where )
{
    const char * myTab = TABS(tab);
    FILE *out = ( where ? where : stdout );

    fprintf( out,
             "%sSurfCenters = { -- NB surfCenters: %d\n", myTab, dwNSurfCenter );
    for( uint i = 0; i < dwNSurfCenter; ++i ) {
        surfcenters[i].show( tab + 1, where );
    } // End for
    fprintf( out, "%s}, -- End SurfCenters\n", myTab );
	fflush( out );
} // End proc cSurfCenterList::show

//-----------------------------------------------------------------------------
// Name   : cSurfCenter::cSurfCenter
// Usage  : cSurfCenter class constructor
// Args   : None
// Return : Nothing
//-----------------------------------------------------------------------------
cSurfCenter::cSurfCenter()
{
    dwIdx1 = dwIdx2 = -1;
    dwSurfIdx = -1;
    posWrld.x = posWrld.y = posWrld.z = 0.0;
    posBBox.x = posBBox.y = posBBox.z = 0.0;

    _index   = 0;
    _filePos = 0;
} // End proc cSurfCenter::cSurfCenter

//-----------------------------------------------------------------------------
// Name   : cSurfCenter::read
// Usage  : Read cSurfCenter from file
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         h                X   X   File handler
//         idx              X   -   (Optional) surface center index
//
// Return : 0 == OK / -1 == read error / -2 == id error
//-----------------------------------------------------------------------------
int cSurfCenter::read( FILE * h, const uint idx )
{
    int rtn = -1;

    do {
        _index    = idx;
        _filePos  = ftell( h );

        if(1 != fread( &dwIdx1,    sizeof(dwIdx1), 1, h ))    break;
        if(1 != fread( &dwIdx2,    sizeof(dwIdx2), 1, h ))    break;
        if(1 != fread( &dwSurfIdx, sizeof(dwSurfIdx), 1, h )) break;
        if( 0 != ( rtn = posWrld.read( h ))) break;
        if( 0 != ( rtn = posBBox.read( h ))) break;

        rtn = 0;
    } while(0);

    return( rtn );
} // End proc cSurfCenter::read

//-----------------------------------------------------------------------------
// Name   : cSurfCenter::write
// Usage  : Read cSurfCenter from file
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         h                X   X   File handler
//
// Return : 0 == OK / -1 == write error / -2 == id error
//-----------------------------------------------------------------------------
int cSurfCenter::write( FILE * h )
{
    int rtn = -1;

    do {
        if(1 != fwrite( &dwIdx1,    sizeof(dwIdx1), 1, h ))    break;
        if(1 != fwrite( &dwIdx2,    sizeof(dwIdx2), 1, h ))    break;
        if(1 != fwrite( &dwSurfIdx, sizeof(dwSurfIdx), 1, h )) break;
        if( 0 != ( rtn = posWrld.write( h ))) break;
        if( 0 != ( rtn = posBBox.write( h ))) break;

        rtn = 0;
    } while(0);

    return( rtn );
} // End proc cSurfCenter::write

//-----------------------------------------------------------------------------
// Name   : cSurfCenter::show
// Usage  : Dump cSurfCenter to screen
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         tab              X   -   (Optional) number of tabs before printing
//         where            X   -   (Optional) where to send data
//
// Return : Nothing
//-----------------------------------------------------------------------------
void cSurfCenter::show( const uint tab, FILE *where )
{
	const char * myTab = TABS(tab);
	FILE *out = ( where ? where : stdout );

	fprintf( out, "%s{ -- surfaceCenter #%d @ 0x%08x\n",
				myTab, _index, _filePos );

	fprintf( out, "%s\tdwIdx1    = 0x%08x (%d)\n",
				myTab, dwIdx1, dwIdx1 );
	fprintf( out, "%s\tdwIdx1    = 0x%08x (%d)\n",
				myTab, dwIdx1, dwIdx1 );
	fprintf( out, "%s\tdwSurfIdx = 0x%08x (%d)\n",
				myTab, dwSurfIdx, dwSurfIdx );
	fprintf( out, "%s\tposWrld = ", myTab );
	posWrld.show( NULL, where );
	fprintf( out, "\n" );
	fprintf( out, "%s\tposBBox = ", myTab );
	posBBox.show( NULL, where );
	fprintf( out, "\n" );

	fprintf( out,
				"%s},\n", myTab );
	fflush( out );
} // End proc cSurfCenter::show

//-----------------------------------------------------------------------------
// Name   : cUnknown2List::read
// Usage  : Read cUnknown2List from file
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         h                X   X   File handler
//
// Return : 0 == OK / -1 == read error / -2 memory error
//-----------------------------------------------------------------------------
int cUnknown2List::read( FILE * h )
{
    int rtn = 0;
	uint u;

	do {
		unknown2s.clear();

		if( 1 != fread( &dwNUnknown2, sizeof(dwNUnknown2), 1, h )) {
            rtn = -1;
            break;
		}

		if( dwNUnknown2 ) {
			unknown2s.reserve( dwNUnknown2 );
			if( unknown2s.capacity() < dwNUnknown2 ) {
                errLine = __LINE__;
                rtn = -2;
                break;
            }

			for( uint i = 0; 0 == rtn && i < dwNUnknown2; ++i ) {
				if( 1 != fread( &u, sizeof(u) , 1, h)) rtn = -1;
				if( 0 == rtn ) unknown2s.push_back( u );
			} // End for
            if( 0 != rtn )  break;
		} // End if
	} while(0);

    return( rtn );
} // End proc cUnknown2List::read

//-----------------------------------------------------------------------------
// Name   : cUnknown2List::write
// Usage  : Write cUnknown2List to file
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         h                X   X   File handler
//
// Return : 0 == OK / -1 == write error / -2 memory error
//-----------------------------------------------------------------------------
int cUnknown2List::write( FILE * h )
{
    int rtn = 0;

    if( 1 != fwrite( &dwNUnknown2, sizeof(dwNUnknown2), 1, h )) return( -1 );

    if( dwNUnknown2 ) {
        if( 0 == unknown2s.size() ) return( -2 );

        for( uint i = 0; 0 == rtn && i < dwNUnknown2; ++i ) {
            if( 1 != fwrite( &unknown2s[i],
                             sizeof(unknown2s[i]) , 1, h)) rtn = -1;
        } // End for
    } // End if

    return( rtn );
} // End proc cUnknown2List::write

//-----------------------------------------------------------------------------
// Name   : cUnknown2List::show
// Usage  : Dump cUnknown2List to screen
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         tab              X   -   (Optional) number of tabs before printing
//         where            X   -   (Optional) where to send data
//
// Return : Nothing
//-----------------------------------------------------------------------------
//
void cUnknown2List::show( const uint tab, FILE *where )
{
    const char * myTab = TABS(tab);
    FILE *out = ( where ? where : stdout );

    fprintf( out,
             "%sUnknown2s = { -- NB unknown2s: %d\n", myTab, dwNUnknown2 );

    fprintf( out, "%s\t", myTab);
    for( uint i = 0; i < dwNUnknown2; ++i ) {
        if( i && 0 == ( i % 10 )) {
            fprintf( out, " -- #%d\n", (((i-1) / 10)*10) );
            fprintf( out, "%s\t", myTab);
        }
        fprintf( out, "0x%08x ", unknown2s[i] );
    } // End for
    fprintf( out, "\n" );
    fprintf( out, "%s}, -- End Unknown2s\n", myTab );
	fflush( out );
} // End proc cUnknown2List::show

//-----------------------------------------------------------------------------
// Name   : cUnknown3List::read
// Usage  : Read cUnknown3List from file
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         h                X   X   File handler
//
// Return : 0 == OK / -1 == read error / -2 memory error
//-----------------------------------------------------------------------------
int cUnknown3List::read( FILE * h )
{
    int rtn = 0;
	cUnknown3 u;

	do {
		unknown3s.clear();

		if( 1 != fread( &dwNUnknown3, sizeof(dwNUnknown3), 1, h )) {
            rtn = -1;
            break;
        }

		if( dwNUnknown3 ) {
			unknown3s.reserve( dwNUnknown3 );
			if( unknown3s.capacity() < dwNUnknown3 ) {
                errLine = __LINE__;
                rtn = -2;
                break;
            }

			for( uint i = 0; 0 == rtn && i < dwNUnknown3; ++i ) {
				rtn = u.read( h, i );
				if( 0 == rtn ) unknown3s.push_back( u );
			} // End for
            if( 0 != rtn )  break;
		} // End if
	} while(0);

    return( rtn );
} // End proc cUnknown3List::read

//-----------------------------------------------------------------------------
// Name   : cUnknown3List::write
// Usage  : Write cUnknown3List to file
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         h                X   X   File handler
//
// Return : 0 == OK / -1 == write error / -2 memory error
//-----------------------------------------------------------------------------
int cUnknown3List::write( FILE * h )
{
    int rtn = 0;

    if( 1 != fwrite( &dwNUnknown3, sizeof(dwNUnknown3), 1, h )) return( -1 );

    if( dwNUnknown3 ) {
        if( 0 == unknown3s.size() ) return( -2 );

        for( uint i = 0; 0 == rtn && i < dwNUnknown3; ++i ) {
            rtn = unknown3s[i].write( h );
        } // End for
    } // End if

    return( rtn );
} // End proc cUnknown3List::write

//-----------------------------------------------------------------------------
// Name   : cUnknown3List::show
// Usage  : Dump cUnknown3List to screen
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         tab              X   -   (Optional) number of tabs before printing
//         where            X   -   (Optional) where to send data
//
// Return : Nothing
//-----------------------------------------------------------------------------
//
void cUnknown3List::show( const uint tab, FILE *where )
{
    const char * myTab = TABS(tab);
    FILE *out = ( where ? where : stdout );

    fprintf( out,
             "%sUnknown3s = { -- NB unknown3s: %d\n", myTab, dwNUnknown3 );
    for( uint i = 0; i < dwNUnknown3; ++i ) {
        unknown3s[i].show( tab + 1, where );
    } // End for
    fprintf( out, "%s}, -- End Unknown3s\n", myTab );
	fflush( out );
} // End proc cUnknown3List::show

//-----------------------------------------------------------------------------
// Name   : cUnknown3::cUnknown3
// Usage  : cUnknown3 class constructor
// Args   : None
// Return : Nothing
//-----------------------------------------------------------------------------
cUnknown3::cUnknown3()
{
    fUnk1  = 0.0;
    fUnk2  = 0.0;
    fUnk3  = 0.0;
    fUnk4  = 0.0;
    fUnk5  = 0.0;
    dwUnk1 = 0;
    dwUnk2 = 0;

    _index   = 0;
    _filePos = 0;
} // End proc cUnknown3::cUnknown3

//-----------------------------------------------------------------------------
// Name   : cUnknown3::read
// Usage  : Read cUnknown3 from file
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         h                X   X   File handler
//         idx              X   -   (Optiona) surface index
//
// Return : 0 == OK / -1 == read error / -2 == id error
//-----------------------------------------------------------------------------
int cUnknown3::read( FILE * h, const uint idx )
{
    int rtn = -1;

    do {
        _index    = idx;
        _filePos  = ftell( h );

        if(1 != fread( &fUnk1,  sizeof(fUnk1),  1, h )) break;
        if(1 != fread( &fUnk2,  sizeof(fUnk2),  1, h )) break;
        if(1 != fread( &fUnk3,  sizeof(fUnk3),  1, h )) break;
        if(1 != fread( &fUnk4,  sizeof(fUnk4),  1, h )) break;
        if(1 != fread( &fUnk5,  sizeof(fUnk5),  1, h )) break;
        if(1 != fread( &dwUnk1, sizeof(dwUnk1), 1, h )) break;
        if(1 != fread( &dwUnk2, sizeof(dwUnk2), 1, h )) break;

        rtn = 0;
    } while(0);

    return( rtn );
} // End proc cUnknown3::read

//-----------------------------------------------------------------------------
// Name   : cUnknown3::write
// Usage  : Write cUnknown3 to file
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         h                X   X   File handler
//
// Return : 0 == OK / -1 == write error / -2 == id error
//-----------------------------------------------------------------------------
int cUnknown3::write( FILE * h )
{
    int rtn = -1;

    do {
        if(1 != fwrite( &fUnk1,  sizeof(fUnk1),  1, h )) break;
        if(1 != fwrite( &fUnk2,  sizeof(fUnk2),  1, h )) break;
        if(1 != fwrite( &fUnk3,  sizeof(fUnk3),  1, h )) break;
        if(1 != fwrite( &fUnk4,  sizeof(fUnk4),  1, h )) break;
        if(1 != fwrite( &fUnk5,  sizeof(fUnk5),  1, h )) break;
        if(1 != fwrite( &dwUnk1, sizeof(dwUnk1), 1, h )) break;
        if(1 != fwrite( &dwUnk2, sizeof(dwUnk2), 1, h )) break;

        rtn = 0;
    } while(0);

    return( rtn );
} // End proc cUnknown3::write

//-----------------------------------------------------------------------------
// Name   : cUnknown3::show
// Usage  : Dump cUnknown3 to screen
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         tab              X   -   (Optional) number of tabs before printing
//         where            X   -   (Optional) where to send data
//
// Return : Nothing
//-----------------------------------------------------------------------------
void cUnknown3::show( const uint tab, FILE *where )
{
    const char * myTab = TABS(tab);
    FILE *out = ( where ? where : stdout );

    fprintf( out, "%s{ -- unknown3 #%d @ 0x%08x\n",
             myTab, _index, _filePos );

    fprintf( out, "%s\tfUnk1   = 0x%08x (%g)\n", myTab, *((int*)&fUnk1), fUnk1 );
    fprintf( out, "%s\tfUnk2   = 0x%08x (%g)\n", myTab, *((int*)&fUnk2), fUnk2 );
    fprintf( out, "%s\tfUnk3   = 0x%08x (%g)\n", myTab, *((int*)&fUnk3), fUnk3 );
    fprintf( out, "%s\tfUnk4   = 0x%08x (%g)\n", myTab, *((int*)&fUnk4), fUnk4 );
    fprintf( out, "%s\tfUnk5   = 0x%08x (%g)\n", myTab, *((int*)&fUnk5), fUnk5 );
    fprintf( out, "%s\tdwUnk1  = 0x%08x (%d)\n",
             myTab, dwUnk1, dwUnk1 );
    fprintf( out, "%s\tdwUnk2  = 0x%08x (%d)\n",
             myTab, dwUnk2, dwUnk2 );

    fprintf( out,
             "%s},\n", myTab );
	fflush( out );
} // End proc cUnknown3::show

//-----------------------------------------------------------------------------
// Name   : cUnknown4List::read
// Usage  : Read cUnknown4List from file
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         h                X   X   File handler
//
// Return : 0 == OK / -1 == read error / -2 memory error
//-----------------------------------------------------------------------------
int cUnknown4List::read( FILE * h )
{
    int rtn = 0;
	cUnknown4 u;

	do {
		unknown4s.clear();

		if( 1 != fread( &dwNUnknown4, sizeof(dwNUnknown4), 1, h )) {
            rtn = -1;
            break;
        }

		if( dwNUnknown4 ) {
			unknown4s.reserve( dwNUnknown4 );
			if( unknown4s.capacity() < dwNUnknown4 ) {
                errLine = __LINE__;
                rtn = -2;
                break;
            }

			for( uint i = 0; 0 == rtn && i < dwNUnknown4; ++i ) {
				rtn = u.read( h, i );
				if( 0 == rtn ) unknown4s.push_back( u );
			} // End for
            if( 0 != rtn )  break;
		} // End if
	} while(0);

    return( rtn );
} // End proc cUnknown4List::read

//-----------------------------------------------------------------------------
// Name   : cUnknown4List::write
// Usage  : Write cUnknown4List to file
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         h                X   X   File handler
//
// Return : 0 == OK / -1 == write error / -2 memory error
//-----------------------------------------------------------------------------
int cUnknown4List::write( FILE * h )
{
    int rtn = 0;

    if( 1 != fwrite( &dwNUnknown4, sizeof(dwNUnknown4), 1, h )) return( -1 );

    if( dwNUnknown4 ) {
        if( 0 == unknown4s.size() ) return( -2 );

        for( uint i = 0; 0 == rtn && i < dwNUnknown4; ++i ) {
            rtn = unknown4s[i].write( h );
        } // End for
    } // End if

    return( rtn );
} // End proc cUnknown4List::write

//-----------------------------------------------------------------------------
// Name   : cUnknown4List::show
// Usage  : Dump cUnknown4List to screen
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         tab              X   -   (Optional) number of tabs before printing
//         where            X   -   (Optional) where to send data
//
// Return : Nothing
//-----------------------------------------------------------------------------
//
void cUnknown4List::show( const uint tab, FILE *where )
{
    const char * myTab = TABS(tab);
    FILE *out = ( where ? where : stdout );

    fprintf( out,
             "%sUnknown4s = { -- NB unknown4s: %d\n", myTab, dwNUnknown4 );
    for( uint i = 0; i < dwNUnknown4; ++i ) {
        unknown4s[i].show( tab + 1, where );
    } // End for
    fprintf( out, "%s}, -- End Unknown4s\n", myTab );
	fflush( out );
} // End proc cUnknown4List::show

//-----------------------------------------------------------------------------
// Name   : cUnknown4::cUnknown4
// Usage  : cUnknown4 class constructor
// Args   : None
// Return : Nothing
//-----------------------------------------------------------------------------
cUnknown4::cUnknown4()
{
    dwUnk1 = dwUnk2 = dwSurfIdx1 = dwUnk3 = -1;
    memset( fUnk1,  0, sizeof(fUnk1));
	dwSurfIdx2 = dwSurfIdx3 = dwSurfIdx4 = -1;
    dwUnk4 = dwUnk5 = dwMatIdx = -1;

    _index   = 0;
    _filePos = 0;
} // End proc cUnknown4::cUnknown4

//-----------------------------------------------------------------------------
// Name   : cUnknown4::read
// Usage  : Read cUnknown4 from file
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         h                X   X   File handler
//         idx              X   -   (Optiona) surface index
//
// Return : 0 == OK / -1 == read error / -2 == id error
//-----------------------------------------------------------------------------
int cUnknown4::read( FILE * h, const uint idx )
{
    int rtn = -1;

    do {
        _index    = idx;
        _filePos  = ftell( h );

		if( 1 != fread( &dwUnk1,     sizeof(dwUnk1), 1, h ))     break;
		if( 1 != fread( &dwUnk2,     sizeof(dwUnk2), 1, h ))     break;
		if( 1 != fread( &dwSurfIdx1, sizeof(dwSurfIdx1), 1, h )) break;
		if( 1 != fread( &dwUnk3,     sizeof(dwUnk3), 1, h ))     break;
        if( 1 != fread( fUnk1,       sizeof(fUnk1),  1, h ))     break;
		if( 1 != fread( &dwSurfIdx2, sizeof(dwSurfIdx2), 1, h )) break;
		if( 1 != fread( &dwSurfIdx3, sizeof(dwSurfIdx3), 1, h )) break;
		if( 1 != fread( &dwSurfIdx4, sizeof(dwSurfIdx4), 1, h )) break;
		if( 1 != fread( &dwUnk4,     sizeof(dwUnk4), 1, h ))     break;
		if( 1 != fread( &dwUnk4,     sizeof(dwUnk4), 1, h ))     break;
		if( 1 != fread( &dwMatIdx,   sizeof(dwMatIdx), 1, h ))   break;

        rtn = 0;
    } while(0);

    return( rtn );
} // End proc cUnknown4::read

//-----------------------------------------------------------------------------
// Name   : cUnknown4::write
// Usage  : Write cUnknown4 to file
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         h                X   X   File handler
//         idx              X   -   (Optiona) surface index
//
// Return : 0 == OK / -1 == write error / -2 == id error
//-----------------------------------------------------------------------------
int cUnknown4::write( FILE * h )
{
    int rtn = -1;

    do {
		if( 1 != fwrite( &dwUnk1,     sizeof(dwUnk1), 1, h ))     break;
		if( 1 != fwrite( &dwUnk2,     sizeof(dwUnk2), 1, h ))     break;
		if( 1 != fwrite( &dwSurfIdx1, sizeof(dwSurfIdx1), 1, h )) break;
		if( 1 != fwrite( &dwUnk3,     sizeof(dwUnk3), 1, h ))     break;
        if( 1 != fwrite( fUnk1,       sizeof(fUnk1),  1, h ))     break;
		if( 1 != fwrite( &dwSurfIdx2, sizeof(dwSurfIdx2), 1, h )) break;
		if( 1 != fwrite( &dwSurfIdx3, sizeof(dwSurfIdx3), 1, h )) break;
		if( 1 != fwrite( &dwSurfIdx4, sizeof(dwSurfIdx4), 1, h )) break;
		if( 1 != fwrite( &dwUnk4,     sizeof(dwUnk4), 1, h ))     break;
		if( 1 != fwrite( &dwUnk4,     sizeof(dwUnk4), 1, h ))     break;
		if( 1 != fwrite( &dwMatIdx,   sizeof(dwMatIdx), 1, h ))   break;

        rtn = 0;
    } while(0);

    return( rtn );
} // End proc cUnknown4::write

//-----------------------------------------------------------------------------
// Name   : cUnknown4::show
// Usage  : Dump cUnknown4 to screen
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         tab              X   -   (Optional) number of tabs before printing
//         where            X   -   (Optional) where to send data
//
// Return : Nothing
//-----------------------------------------------------------------------------
void cUnknown4::show( const uint tab, FILE *where )
{
	const char * myTab = TABS(tab);
	FILE *out = ( where ? where : stdout );
	uint i;

	fprintf( out, "%s{ -- unknown4 #%d @ 0x%08x\n",
				myTab, _index, _filePos );

	fprintf( out, "%s\tdwUnk1     = 0x%08x (%d)\n", myTab, dwUnk1, dwUnk1 );
	fprintf( out, "%s\tdwUnk2     = 0x%08x (%d)\n", myTab, dwUnk2, dwUnk2 );
	fprintf( out, "%s\tdwSurfIdx1 = 0x%08x (%d)\n", myTab, dwSurfIdx1, dwSurfIdx1 );
	fprintf( out, "%s\tdwUnk3     = 0x%08x (%d)\n", myTab, dwUnk3, dwUnk3 );

	for( i = 0; i < (sizeof(fUnk1)/sizeof(fUnk1[0])); ++i ) {
		fprintf( out, "%s\tfUnk1[%2d] = 0x%08x (%g)\n",
					myTab, i, *((int*)&fUnk1[i]), fUnk1[i] );
	}

	fprintf( out, "%s\tdwSurfIdx2 = 0x%08x (%d)\n", myTab, dwSurfIdx2, dwSurfIdx2 );
	fprintf( out, "%s\tdwSurfIdx3 = 0x%08x (%d)\n", myTab, dwSurfIdx3, dwSurfIdx3 );
	fprintf( out, "%s\tdwSurfIdx4 = 0x%08x (%d)\n", myTab, dwSurfIdx4, dwSurfIdx4 );
	fprintf( out, "%s\tdwUnk4     = 0x%08x (%d)\n", myTab, dwUnk4, dwUnk4 );
	fprintf( out, "%s\tdwUnk5     = 0x%08x (%d)\n", myTab, dwUnk5, dwUnk5 );
	fprintf( out, "%s\tdwUnk1     = 0x%08x (%d)\n", myTab, dwUnk1, dwUnk1 );
	fprintf( out, "%s\tdwMatIdx   = 0x%08x (%d)\n", myTab, dwMatIdx, dwMatIdx );

	fprintf( out, "%s},\n", myTab );
	fflush( out );
} // End proc cUnknown4::show

//-----------------------------------------------------------------------------
// Name   : cUnknown5List::read
// Usage  : Read cUnknown5List from file
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         h                X   X   File handler
//
// Return : 0 == OK / -1 == read error / -2 memory error
//-----------------------------------------------------------------------------
int cUnknown5List::read( FILE * h )
{
    int rtn = 0;
	float u;

	do {
		unknown5s.clear();

//     if( 1 != fread( &dwNUnknown5, sizeof(dwNUnknown5), 1, h )) return( -1 );
		dwNUnknown5 = 16; // TODO: CHECKME

		if( dwNUnknown5 ) {
			unknown5s.reserve( dwNUnknown5 );
			if( unknown5s.capacity() < dwNUnknown5 ) {
                errLine = __LINE__;
                rtn = -2;
                break;
            }

			for( uint i = 0; 0 == rtn && i < dwNUnknown5; ++i ) {
				if( 1 != fread( &u, sizeof(u), 1, h)) rtn = -1;
				if( 0 == rtn ) unknown5s.push_back( u );
			} // End for
            if( 0 != rtn )  break;
		} // End if
	} while(0);

    return( rtn );
} // End proc cUnknown5List::read

//-----------------------------------------------------------------------------
// Name   : cUnknown5List::write
// Usage  : Write cUnknown5List to file
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         h                X   X   File handler
//
// Return : 0 == OK / -1 == write error / -2 memory error
//-----------------------------------------------------------------------------
int cUnknown5List::write( FILE * h )
{
    int rtn = 0;

    if( dwNUnknown5 ) {
        if( 0 == unknown5s.size() ) return( -2 );

        for( uint i = 0; 0 == rtn && i < dwNUnknown5; ++i ) {
            if( 1 != fwrite( &unknown5s[i],
                             sizeof(unknown5s[i]) , 1, h)) rtn = -1;
        } // End for
    } // End if

    return( rtn );
} // End proc cUnknown5List::write

//-----------------------------------------------------------------------------
// Name   : cUnknown5List::show
// Usage  : Dump cUnknown5List to screen
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         tab              X   -   (Optional) number of tabs before printing
//         where            X   -   (Optional) where to send data
//
// Return : Nothing
//-----------------------------------------------------------------------------
//
void cUnknown5List::show( const uint tab, FILE *where )
{
    const char * myTab = TABS(tab);
    FILE *out = ( where ? where : stdout );

    fprintf( out,
             "%sUnknown5s = { -- NB unknown5s: %d\n", myTab, dwNUnknown5 );

    for( uint i = 0; i < dwNUnknown5; ++i ) {
        fprintf( out, "%s\tunknown5s[%2d] = (0x%08x) %f (%g)\n",
                 myTab, i, *((uint*)&unknown5s[i]), unknown5s[i], unknown5s[i] );
    } // End for
    fprintf( out, "%s}, -- End Unknown5s\n", myTab );
	fflush( out );
} // End proc cUnknown5List::show

//-----------------------------------------------------------------------------
// Name   : cModelList::read
// Usage  : Read cModelList from file
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         h                X   X   File handler
//
// Return : 0 == OK / -1 == read error / -2 memory error
//-----------------------------------------------------------------------------
int cModelList::read( FILE * h )
{
    int rtn = 0;
	cModel m;

	do {
		clear();

		if( 1 != fread( &dwNModel, sizeof(dwNModel), 1, h )) {
            rtn = -1;
            break;
        }

		if( dwNModel ) {
			models.reserve( dwNModel );
			if( models.capacity() < dwNModel ) {
                errLine = __LINE__;
                rtn = -2;
                break;
            }

			for( uint i = 0; 0 == rtn && i < dwNModel; ++i ) {
				rtn = m.read( h, i );
				if( 0 == rtn ) models.push_back( m );
			} // End for
			if( 0 != rtn ) break;
		} // End if
	} while(0);

    return( rtn );
} // End proc cModelList::read

//-----------------------------------------------------------------------------
// Name   : cModelList::write
// Usage  : Write cModelList to file
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         h                X   X   File handler
//
// Return : 0 == OK / -1 == write error / -2 memory error
//-----------------------------------------------------------------------------
int cModelList::write( FILE * h )
{
    int rtn = 0;

    if( 1 != fwrite( &dwNModel, sizeof(dwNModel), 1, h )) return( -1 );

    if( dwNModel ) {
        if( 0 == models.size() ) return( -2 );

        for( uint i = 0; 0 == rtn && i < dwNModel; ++i ) {
            rtn = models[i].write( h );
        } // End for
    } // End if

    return( rtn );
} // End proc cModelList::write

#if !defined(NO_BLENDER)
//-----------------------------------------------------------------------------
// Name   : cModelList::toBlender
// Usage  : Draw in Blender
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         track            X   -   Track object surfaceList belongs to
//
// Return : 0 in case of success, -1 otherwise
//-----------------------------------------------------------------------------
int cModelList::toBlender( cTrack& track )
{
    uint modIdx = 0;  // Model index
    int  rtn     = 0;

    for( modIdx = 0; 0 == rtn && modIdx < dwNModel; ++modIdx ) {
        rtn = models[modIdx].toBlender( track );
    } // End for

    // ---------------------------------------------------------------
    // ---   Organize models
    // ---------------------------------------------------------------
    if( rtn >= 0 ) organize( track );

    return( rtn );
} // End proc cModelList::toBlender
#endif //NO_BLENDER

#if !defined(NO_BLENDER)
//-----------------------------------------------------------------------------
// Name   : cModelList::fromBlender
// Usage  : Get model list data from Blender
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         track            X   -   Track object model list belongs to
//
// Return : 0 in case of success, -1 otherwise
//-----------------------------------------------------------------------------
int cModelList::fromBlender( cTrack& track )
{
	int rtn = 0;
	Object *ob = NULL;
	cBoundingBox bb;
	cModel       mo;
	cMeshToBBox  mbb;
	vector<cModel> children;

	// Delete all models and related data
	track.modelList.clear();
	track.boundingBoxList.clear();
	track.meshToBBoxList.clear();

	for( ob = (Object*)G.main->object.first;
			rtn >= 0 && NULL != ob;
			ob = (Object*)ob->id.next ) {
		if( !ob->data || NULL == ob->data || ID_ME != GS(((ID*)ob->data)->name) ) continue; // Meshes only
		if( !ob->par1 || OB_FO_MODEL != ob->FOdynFlag ) continue; // Skip if not processed or if not model

		// Add model
		mo._index = track.modelList.models.size();
		rtn = mo.fromBlender( ob );
		if( rtn >= 0 ) {
#if 0 // TEST
			models.push_back( mo );
			dwNModel++;
			ob->par3 = mo._index;
#else
			if( ( MODEL_IS_CHILD & ob->par5 ) ) {
				children.push_back( mo );
			} else {
				models.push_back( mo );
				dwNModel++;
				ob->par3 = mo._index;
			}
#endif
		}
		if( mo.sName ) { free( mo.sName ); mo.sName = NULL; }
		if( rtn < 0 ) break;

		// "Bounding box" are not used for child/lod objects
		if( 0 == ob->par5 || ( MODEL_IS_DAMAGE & ob->par5 )) {
			// Add bounding box
			bb._index = track.boundingBoxList.boundingBoxes.size();
			rtn = bb.fromBlender( ob, &mo );
			if( rtn >= 0 ) {
				track.boundingBoxList.boundingBoxes.push_back( bb );
				track.boundingBoxList.dwNBoundingBox++;
				ob->par4 = bb._index;
			}
			if( rtn < 0 ) break;
		} // End if (not lod)
	} // End for

	// Fix bounding boxes
	// (lod objects may have been not initialized on bbox creation)
	vector<cBoundingBox>::iterator bbox;
	for( bbox = track.boundingBoxList.boundingBoxes.begin(); bbox != track.boundingBoxList.boundingBoxes.end(); ++bbox ) {
		uint i = 1 /* the 1st one is right */;
		if( bbox->_obj->lod1 ) bbox->dwModelIdx[i++] = bbox->_obj->lod1->par3;
		if( bbox->_obj->lod2 ) bbox->dwModelIdx[i++] = bbox->_obj->lod2->par3;
	}

#if 1 //TEST
	vector<cModel>::iterator child;
	for( child = children.begin(); child != children.end(); ++child ) {
		vector<uint>::iterator first;
		if( child->_obj->parent->par3 < 0 || child->_obj->parent->par3 > dwNModel ) { SET_ERROR_AND_RETURN(-1); }
		first= models[child->_obj->parent->par3].dwSurfIdx.begin();
		//models[child->_obj->parent->par3].dwSurfIdx.push_back( child->_obj->par2 );
		models[child->_obj->parent->par3].dwSurfIdx.insert( first, child->_obj->par2 );
		models[child->_obj->parent->par3].dwNSurfIdx++;
		if( models[child->_obj->parent->par3].bSphereR < child->bSphereR ) {
			models[child->_obj->parent->par3].bSphereR = child->bSphereR;
			models[child->_obj->parent->par3].bBox = child->bBox;
			models[child->_obj->parent->par3].bBoxCenterOffset = child->bBoxCenterOffset;

			track.boundingBoxList.boundingBoxes[child->_obj->parent->par4].bBox = child->bBox;
			track.boundingBoxList.boundingBoxes[child->_obj->parent->par4].bBoxCenterOffset = child->bBoxCenterOffset;
		}
	}
#endif

	// Update parent/child and mesh to bounding box
	for( uint i = 0; rtn >= 0 && i < dwNModel; ++i ) {
		cModel *pMod = &models[i];

		if( pMod->_obj->parent ) {
			if( pMod->_obj->parent->par3 < 0 || pMod->_obj->parent->par3 > dwNModel ) { SET_ERROR_AND_RETURN(-1); }
			models[pMod->_obj->parent->par3].dwSurfIdx.push_back( pMod->_obj->par2 );
			models[pMod->_obj->parent->par3].dwNSurfIdx++;
		}

		// Add mesh to bounding box (not for damage/child/lod objects!!)
		if( 0 == pMod->_obj->par5 ) {
			mbb._index = track.meshToBBoxList.meshToBBoxes.size();
			rtn = mbb.fromBlender( pMod->_obj );
			if( rtn >= 0 ) {
				track.meshToBBoxList.meshToBBoxes.push_back( mbb );
				track.meshToBBoxList.dwNMeshToBBox++;
			}
			if( mbb.sMeshPrefix ) { free( mbb.sMeshPrefix ); mbb.sMeshPrefix = NULL; }
			if( rtn < 0 ) break;
		} // End if (not damage object)
	} // End for

	return( rtn );
} // End proc cModelList::fromBlender
#endif //NO_BLENDER

#if !defined(NO_BLENDER)
//-----------------------------------------------------------------------------
// Name   : cModelList::organize
// Usage  : Move models in Blender scene so that they do not overlap
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         track            X   -   Track object surfaceList belongs to
//
// Return : 0 in case of success, -1 otherwise
//-----------------------------------------------------------------------------
int cModelList::organize( cTrack& track )
{
    uint modIdx = 0;  // Model index
    int  rtn     = 0;

	int r = 0; // Row
	int c = 0; // Column
	int aSz = (int)sqrt((float)dwNModel) + 1;	// Number of rows & columns
	float *bndSzX = (float*)MEM_callocN( sizeof(float)*dwNModel, "Bound size X");
	float *bndSzY = (float*)MEM_callocN( sizeof(float)*dwNModel, "Bound size Y");
	float *offX = (float*)MEM_callocN( sizeof(float)*aSz, "Offset X");
	float *offY = (float*)MEM_callocN( sizeof(float)*aSz, "Offset Y");
	int i;
	Object *obj;
	Mesh *me;
	float min[3], max[3];

	if( bndSzX && bndSzY && offX && offY ) {
		// Compute bounding box sizes
		INIT_MINMAX(min, max);
		for( i = 0; i < aSz; ++i ) offX[i] = offY[i] = max[0];

		for( modIdx = 0; modIdx < dwNModel; ++modIdx ) {
			float lX, lY;

			int sIdx = models[modIdx].dwSurfIdx[(models[modIdx].dwNSurfIdx - 1)];
			obj = track.surfaceList.surfaces[sIdx]._obj;
			me = get_mesh( obj );
			if( NULL == obj || NULL == me ) continue;

			// Compute objects bounding box sizes
			// (bounding box seems not to be already initialized here, so have to compute it)
			boundbox_mesh( me, NULL, NULL );
			INIT_MINMAX(min, max);
			minmax_object(obj, min, max);
			lX = ( max[0] - min[0] );
			lY = ( max[1] - min[1] );
			bndSzX[modIdx] = lX;
			bndSzY[modIdx] = lY;

			if( !obj->parent ) {
				offX[r] = MAX2( offX[r], lX );
				offY[r] = MAX2( offY[r], lY );
				// Check for new row
				if( 0 == ( ++c % aSz )) ++r;
			}
		} // End for

		// Arrange objects
		float startX = 0.0;
		float startY = 0.0;
		float dX, dY;
		float maxY;

		INIT_MINMAX(min, max);
		for( i = 0; i < aSz; ++i ) {
			if( max[0] != offX[i] ) startX -= offX[i];
			if( max[0] != offY[i] ) startY += offY[i];
		}
		r = c = 0;
		dX = startX / 2;
		dY = startY / 2;

		for( modIdx = 0; modIdx < dwNModel; ++modIdx ) {
			int sIdx = models[modIdx].dwSurfIdx[(models[modIdx].dwNSurfIdx - 1)];
			obj = track.surfaceList.surfaces[sIdx]._obj;
			if( !obj ) continue;

			if( obj->parent ) {
				// If object is child, then coords are relative to parent
				obj->loc[0] = 0.0;
				obj->loc[1] = 0.0;
				obj->loc[2] = 0.0;
			} else {
				// else, move object to its new position
				obj->loc[0] = dX + bndSzX[modIdx] / 2;
				obj->loc[1] = dY - bndSzY[modIdx] / 2;
				obj->loc[2] = 0.0;
				dX += ( bndSzX[modIdx] + 1.0 );
				// Check for new row
				if( 0 == ( ++c % aSz )) {
					dX = startX / 2;
					dY -= ( offY[r] + 1.0 );
					r++;
				}
			}
		}
	} // End if

	if( bndSzX ) MEM_freeN( bndSzX );
	if( bndSzY ) MEM_freeN( bndSzY );
	if( offX ) MEM_freeN( offX );
	if( offY ) MEM_freeN( offY );

    return( rtn );
} // End proc cModelList::organize
#endif //NO_BLENDER

//-----------------------------------------------------------------------------
// Name   : cModelList::show
// Usage  : Dump cModelList to screen
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         tab              X   -   (Optional) number of tabs before printing
//         where            X   -   (Optional) where to send data
//
// Return : Nothing
//-----------------------------------------------------------------------------
//
void cModelList::show( const uint tab, FILE *where )
{
    const char * myTab = TABS(tab);
    FILE *out = ( where ? where : stdout );

    fprintf( out,
             "%sModels = { -- NB models: %d\n", myTab, dwNModel );
    for( uint i = 0; i < dwNModel; ++i ) {
        models[i].show( tab + 1, where );
    } // End for
    fprintf( out, "%s}, -- End Models\n", myTab );
	fflush( out );
} // End proc cModelList::show

//-----------------------------------------------------------------------------
// Name   : cModel::cModel
// Usage  : cModel class constructor
// Args   : None
// Return : Nothing
//-----------------------------------------------------------------------------
cModel::cModel()
{
    dwID   = ID_MODEL;
    dwUnk1 = 4;			// ??? Set to 4 on export...
    sName  = NULL;
    bBox.x = bBox.y = bBox.z = bSphereR = 0.0;
	bBoxCenterOffset = bBox;
    dwNSurfIdx = 0;
    dwSurfIdx.clear();

    _index   = 0;
    _filePos = 0;
#if !defined(NO_BLENDER)
	_obj = NULL;
#endif //NO_BLENDER
} // End proc cModel::cModel

//-----------------------------------------------------------------------------
// Name   : cModel::cModel
// Usage  : Copy constructor
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         m                X   -   Source object
//
// Return : Nothing
//-----------------------------------------------------------------------------
cModel::cModel( const cModel& m )
{
    dwID             = m.dwID;
    dwUnk1           = m.dwUnk1;
    sName            = ( m.sName ? strdup( m.sName ) : NULL );
    bBoxCenterOffset = m.bBoxCenterOffset;
    bBox             = m.bBox;
	bSphereR         = m.bSphereR;
    dwNSurfIdx       = m.dwNSurfIdx;
    dwSurfIdx        = m.dwSurfIdx;

    _index   = m._index;
    _filePos = m._filePos;
#if !defined(NO_BLENDER)
	_obj     = m._obj;
#endif //NO_BLENDER
} // End proc cModel::cModel

//-----------------------------------------------------------------------------
// Name   : cModel::~cModel
// Usage  : cModel class destructor
// Args   : None
// Return : Nothing
//-----------------------------------------------------------------------------
cModel::~cModel()
{
    if( sName ) free( sName );
} // End proc cModel::cModel

//-----------------------------------------------------------------------------
// Name   : cModel::read
// Usage  : Read cModel from file
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         h                X   X   File handler
//         idx              X   -   (Optiona) surface index
//
// Return : 0 == OK / -1 == read error / -2 == id error / -3 == memory error
//-----------------------------------------------------------------------------
int cModel::read( FILE * h, const uint idx )
{
    int rtn = -1;
	uint si;

    do {
        _index    = idx;
        _filePos  = ftell( h );

        if( 1 != fread( &dwID,   sizeof(dwID), 1, h ))     break;
        if( ID_MODEL != dwID ) { rtn = -2; break; }

        if( 1 != fread( &dwUnk1, sizeof(dwUnk1), 1, h ))   break;
        if( NULL == ( sName = readString( h )))            break;
		if( 0 != bBoxCenterOffset.read( h )) break;
        if( 0 != bBox.read( h )) break;
        if( 1 != fread( &bSphereR, sizeof(bSphereR), 1, h )) break;

        if( 1 != fread( &dwNSurfIdx, sizeof(dwNSurfIdx), 1, h )) break;
        if( dwNSurfIdx ) {
            dwSurfIdx.reserve( dwNSurfIdx );
            if( dwSurfIdx.capacity() < dwNSurfIdx ) { rtn = -3; break; }
			rtn = 0;
			for( uint i = 0; 0 == rtn && i < dwNSurfIdx; ++i ) {
				if( 1 != fread( &si, sizeof(si), 1, h )) rtn = -1;
				if( 0 == rtn ) {
					dwSurfIdx.push_back( 0 );
					dwSurfIdx[i] = si;
				}
			} // End for
			if( 0 != rtn ) break;
        }

        rtn = 0;
    } while(0);

    return( rtn );
} // End proc cModel::read

//-----------------------------------------------------------------------------
// Name   : cModel::write
// Usage  : Write cModel to file
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         h                X   X   File handler
//
// Return : 0 == OK / -1 == write error / -2 == id error / -3 == memory error
//-----------------------------------------------------------------------------
int cModel::write( FILE * h )
{
    int rtn = -1;

    do {
        if( ID_MODEL != dwID ) { rtn = -2; break; }
        if( 1 != fwrite( &dwID,   sizeof(dwID), 1, h ))     break;

        if( 1 != fwrite( &dwUnk1, sizeof(dwUnk1), 1, h ))   break;
        if( 0 != writeString( h, sName ))                   break;
        if( 0 != bBoxCenterOffset.write( h )) break;
        if( 0 != bBox.write( h )) break;
        if( 1 != fwrite( &bSphereR, sizeof(bSphereR), 1, h )) break;

        if( 1 != fwrite( &dwNSurfIdx, sizeof(dwNSurfIdx), 1, h )) break;
        if( dwNSurfIdx ) {
            if( 0 == dwSurfIdx.size() ) { rtn = -3; break; }
			rtn = 0;
			for( uint i = 0; 0 == rtn && i < dwNSurfIdx; ++i ) {
				if( 1 != fwrite( &dwSurfIdx[i], sizeof(dwSurfIdx[i]), 1, h )) rtn = -1;
			} // End for
			if( 0 != rtn ) break;
        }

        rtn = 0;
    } while(0);

    return( rtn );
} // End proc cModel::write

#if !defined(NO_BLENDER)
//-----------------------------------------------------------------------------
// Name   : cModel::toBlender
// Usage  : Draw in Blender
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         track            X   -   Track object surfaceList belongs to
//
// Return : 0 in case of success, 1 for warning, -1 otherwise
//-----------------------------------------------------------------------------
int cModel::toBlender( cTrack& track )
{
    int  rtn = 0;
    cSurface *pSurf = NULL;
    uint      sIdx  = 0;  // Surface index

    if( dwNSurfIdx &&
        dwSurfIdx.size() ) {

        sIdx = (dwNSurfIdx - 1);
        pSurf = &track.surfaceList.surfaces[dwSurfIdx[sIdx]];

        if( pSurf->_obj ) {
            char tmpName[ID_NAME_LG];

            snprintf( tmpName, sizeof(tmpName), "_%s", sName );
            rename_id( &pSurf->_obj->id, tmpName );
#ifdef WITH_FO2_DBG
            printMsg( MSG_DBG,
                        "Renaming model #%d (surface #%d) to '%s'\n",
                        _index, pSurf->_index, tmpName );
#endif // WITH_FO2_DBG

			// Model is made of several surfaces;
			// let's link them (parent/child link)
			for( sIdx = 0; rtn >= 0 && sIdx < (dwNSurfIdx-1); ++sIdx ) {
				Object *child;
				child = track.surfaceList.surfaces[dwSurfIdx[sIdx]]._obj;

				if( !child ) continue;
				if( 0 != makeParent( pSurf->_obj, child )) rtn = -1;
			} // End for

			// --------------------------------------------------------
			// ---   Set flags for dynamic models
			// --------------------------------------------------------
			pSurf->_obj->FOdynFlag = OB_FO_MODEL;
		} else {
			rtn = 1;
			printMsg( MSG_ALL, "[WNG] Model #%d: Surface #%d has no object !!\n",
					_index, pSurf->_index );
        } // End else
    } // End if

    return( rtn );
} // End proc cModel::toBlender
#endif //NO_BLENDER

#if !defined(NO_BLENDER)
//-----------------------------------------------------------------------------
// Name   : cModel::fromBlender
// Usage  : Get model data from Blender
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         ob               X   -   Blender model object
//         track            X   -   Track object model belongs to
//
// Return : 0 in case of success, 1 for warning, -1 otherwise
//-----------------------------------------------------------------------------
int cModel::fromBlender( Object *ob )
{
    int  rtn = 0;
	char name[ID_NAME_LG];
	char *c;
	float min[3],max[3];

	if( !ob ) { SET_ERROR_AND_RETURN(-1); }

	// Name of 1st mesh /wo "@nnn" extension
	snprintf( name, sizeof(name), "%s", ob->id.name+2 );
	c = strchr( name, '@' );
	if( c ) *c = '\0';
	if( sName ) free( sName );
	sName = strdup( name + ('_' == name[0] ? 1 : 0));

	// Bounding box ( = Blender bounding box / 2 )
	INIT_MINMAX(min, max);
	minmax_object(ob, min, max);
	bBox.x = (max[0]-min[0])/2;
	bBox.y = (max[1]-min[1])/2;
	bBox.z = (max[2]-min[2])/2;

	// Bounding box center offset
	if( !ob->parent ) {
		bBoxCenterOffset.x = max[0] - (ob->loc[0]+bBox.x);
		bBoxCenterOffset.y = max[1] - (ob->loc[1]+bBox.y);
		bBoxCenterOffset.z = max[2] - (ob->loc[2]+bBox.z);
	} else {
		bBoxCenterOffset.x = max[0] - (ob->parent->loc[0]+bBox.x);
		bBoxCenterOffset.y = max[1] - (ob->parent->loc[1]+bBox.y);
		bBoxCenterOffset.z = max[2] - (ob->parent->loc[2]+bBox.z);
	}

	// Bounding sphere radius ( = | bBox | )
	bSphereR = sqrt(bBox.x*bBox.x + bBox.y*bBox.y + bBox.z*bBox.z);

	// Surface indexes
	if( ob->par2 < 0 ) { SET_ERROR_AND_RETURN(-1); }
	dwSurfIdx.clear();
	dwSurfIdx.push_back( ob->par2 );
	dwNSurfIdx = 1;

	_obj = ob;
	ob->par3 = _index;

	return( rtn );
} // End proc cModel::fromBlender
#endif //NO_BLENDER

//-----------------------------------------------------------------------------
// Name   : cModel::show
// Usage  : Dump cModel to screen
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         tab              X   -   (Optional) number of tabs before printing
//         where            X   -   (Optional) where to send data
//
// Return : Nothing
//-----------------------------------------------------------------------------
void cModel::show( const uint tab, FILE *where )
{
    const char * myTab = TABS(tab);
    FILE *out = ( where ? where : stdout );
    uint i;

    fprintf( out, "%s{ -- model #%d @ 0x%08x\n", myTab, _index, _filePos );
    fprintf( out, "%s\tdwID    = 0x%08x (%c%c%c%c)\n",
             myTab, dwID, SHOW_ID(dwID) );
    fprintf( out, "%s\tdwUnk1  = 0x%08x (%d)\n", myTab, dwUnk1, dwUnk1 );
    fprintf( out, "%s\tsName   = %s\n",          myTab, sName );

    fprintf( out, "%s\tbBoxCenterOffset = { ", myTab );
    bBoxCenterOffset.show( "", out );
    fprintf( out, "}\n" );
    fprintf( out, "%s\tbBox = { ", myTab );
    bBox.show( "", out );
    fprintf( out, "}\n" );
    fprintf( out, "%s\tbSphereR =%f (%g)\n", myTab, bSphereR, bSphereR );

    fprintf( out, "%s\tdwNSurfIdx = 0x%08x (%d)\n",
             myTab, dwNSurfIdx, dwNSurfIdx );
    for( i = 0; i < dwNSurfIdx; ++i ) {
        fprintf( out, "%s\tdwSurfIdx[%d] = 0x%08x (%d)\n",
                 myTab, i, dwSurfIdx[i], dwSurfIdx[i] );
    }

    fprintf( out,
             "%s},\n", myTab );
	fflush( out );
} // End proc cModel::show

//-----------------------------------------------------------------------------
// Name   : cObjectList::read
// Usage  : Read cObjectList from file
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         h                X   X   File handler
//
// Return : 0 == OK / -1 == read error / -2 memory error
//-----------------------------------------------------------------------------
int cObjectList::read( FILE * h )
{
    int rtn = 0;
    cObject o;

    objects.clear(); dwNObject = 0;

    if( 1 != fread( &dwNObject, sizeof(dwNObject), 1, h )) return( -1 );

    if( dwNObject ) {
        objects.reserve( dwNObject );
        if( objects.capacity() < dwNObject ) return( -2 );

        for( uint i = 0; 0 == rtn && i < dwNObject; ++i ) {
            rtn = o.read( h, i );
            if( 0 == rtn ) objects.push_back( o );
        } // End for
    } // End if
    o.raz(); // To avoid memory leak

    return( rtn );
} // End proc cObjectList::read

//-----------------------------------------------------------------------------
// Name   : cObjectList::write
// Usage  : Write cObjectList to file
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         h                X   X   File handler
//
// Return : 0 == OK / -1 == write error / -2 memory error
//-----------------------------------------------------------------------------
int cObjectList::write( FILE * h )
{
    int rtn = 0;

    if( 1 != fwrite( &dwNObject, sizeof(dwNObject), 1, h )) return( -1 );

    if( dwNObject ) {
        if( dwNObject != objects.size() ) return( -2 );

        for( uint i = 0; 0 == rtn && i < dwNObject; ++i ) {
            rtn = objects[i].write( h );
        } // End for
    } // End if

    return( rtn );
} // End proc cObjectList::write

#if !defined(NO_BLENDER)
#ifdef FO2_USE_ARMATURE
//-----------------------------------------------------------------------------
// Name   : add_editbone
// Usage  : Create new edit bone
// Args   : None
//
// Return : Pointer to new edit bone or NULL
//-----------------------------------------------------------------------------
EditBone *add_editbone( void ) {
    EditBone *eb = NULL;
    float head[3], tail[3];

    eb = (EditBone*)MEM_callocN(sizeof(EditBone), "eBone");
    if( eb ) {
	eb->parent = NULL;
	eb->flag |= BONE_TIPSEL;
	eb->weight= 1.0F;
	eb->dist= 0.25F;
	eb->xwidth= 0.1F;
	eb->zwidth= 0.1F;
	eb->ease1= 1.0;
	eb->ease2= 1.0;
	eb->rad_head= 0.10F;
	eb->rad_tail= 0.05F;
	eb->segments= 1;

	eb->roll = 0.0f;

 	head[0] = head[1] = head[2] = 0.0f;
 	tail[1] = tail[2] = 0.0f;
 	tail[0] = 1.0f;
	VECCOPY(eb->head, head);
 	VECCOPY(eb->tail, tail);
    }

    return( eb );
}
#endif // FO2_USE_ARMATURE
#endif //NO_BLENDER

#if !defined(NO_BLENDER)
#ifdef FO2_USE_ARMATURE
extern "C" {
void editbones_to_armature (ListBase *list, Object *ob);
}
#endif // FO2_USE_ARMATURE
#endif //NO_BLENDER

#if !defined(NO_BLENDER)
//-----------------------------------------------------------------------------
// Name   : cObjectList::toBlender
// Usage  : Draw in Blender
// Args   : --------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         track            X   -   Track objectList belongs to
//
// Return : 0 = success / -1 = error / -2 = memory error
//-----------------------------------------------------------------------------
int cObjectList::toBlender( cTrack& track )
{
	int rtn = 0;
	Object    *ob = NULL;

	for( uint i = 0; 0 == rtn && i < dwNObject; ++i ) {
		rtn = objects[i].toBlender(i,track);
	}

	return( rtn );
} // End proc cObjectList::toBlender

#if 0
// For cars?
//-----------------------------------------------------------------------------
// Name   : cObjectList::toBlender
// Usage  : Draw in Blender
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         gr               X   -   (Optional) Object group
//
// Return : 0 = success / -1 = error / -2 = memory error
//-----------------------------------------------------------------------------
int cObjectList::toBlender( Group* gr )
{
#ifdef FO2_USE_ARMATURE
#if 0
	// Only one armature in all scene...
	int rtn = 0;
	Object    *ob = NULL;
	bArmature *ar = NULL;
	ListBase li;

	if( 0 != dwNObject ) {
		li.first = li.last = NULL;

		ob = add_object( OB_ARMATURE );
		if( ob ) {
			ar = get_armature(ob);
			if( gr ) add_to_group( gr, ob );
		}

		if( NULL == ob || NULL == ar ) {

			errLine = __LINE__;
			rtn = -1;

		} else {

			ar->drawtype = ARM_LINE;  // Draw lines

			for( uint i = 0; 0 == rtn && i < dwNObject; ++i ) {
				rtn = objects[i].toBlender( &li, ar->layer );
			} // End for

			EditBone  *ebo;
			uint i = 0;

			for (ebo=(EditBone*)li.first; ebo; ebo=(EditBone*)ebo->next){
				printMsg( MSG_CONSOLE, "Bone #%d : %s\n", i++, ebo->name );
			}

			// Transform edit bones to bones, and link them to armature
			editbones_to_armature( &li, ob );

			// Free edit bone list
			BLI_freelistN( &li );
		}
	}
#else
	int rtn = 0;
	Object    *ob = NULL;
	bArmature *ar = NULL;
	ListBase li;
	char name[ID_NAME_LG];
	EditBone  *ebo = NULL;

	if( 0 != dwNObject ) {
		for( uint i = 0; 0 == rtn && i < dwNObject; ++i ) {
			ob = add_object( OB_ARMATURE );
			if( ob ) {
				sendToLayer( ob, LAYER_DYN_OBJ );
				if( gr ) add_to_group( gr, ob );
				ar = get_armature(ob);

				// Name armature
				snprintf( name, sizeof(name), "%s", objects[i].sName );
				rename_id( &ob->id, name );
				if( ar ) {
					snprintf( name, sizeof(name), "object_%d", i );
					rename_id( &ar->id, name );
				}

				ob->loc[0] = objects[i].matrix.m[3][0];
				ob->loc[1] = objects[i].matrix.m[3][2];  // Y and Z are inverted
				ob->loc[2] = objects[i].matrix.m[3][1];
			}

			if( NULL == ob || NULL == ar ) {
				SET_ERROR_AND_RETURN(-1);
			}

			// Armature parameters
			ar->drawtype = ARM_OCTA;	// Draw octahedrons
			ar->flag     = ARM_RESTPOS;	// No pose mode

			// Move to dynamic object models layer
			ar->layer = LAYER_DYN_OBJ;

			li.first = li.last = NULL;
			ebo = objects[i].toBlender( ar->layer );
			if( !ebo ) { SET_ERROR_AND_BREAK(-2,rtn); }

			// Transform edit bones to bones, and link them to armature
			BLI_addtail( &li, ebo );
			editbones_to_armature( &li, ob );

			// Free edit bone list
			BLI_freelistN( &li );
		} // End for
	}
#endif //0
#else
	int rtn = 0;

	for( uint i = 0; 0 == rtn && i < dwNObject; ++i ) {
		rtn = objects[i].toBlender( gr );
	} // End for
#endif // FO2_USE_ARMATURE

    return( rtn );
} // End proc cObjectList::toBlender
#endif // 0
#endif // NO_BLENDER

#if !defined(NO_BLENDER)
//-----------------------------------------------------------------------------
// Name   : cObjectList::fromBlender
// Usage  : Get object list data from Blender
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         track            X   -   Track object list belongs to
//
// Return : 0 in case of success, -1 otherwise
//-----------------------------------------------------------------------------
int cObjectList::fromBlender( cTrack& track )
{
	int rtn = 0;
	Object *ob = NULL;
	cObject obj;

	// Delete all objects
	objects.clear();
	dwNObject = 0;

	for( ob = (Object*)G.main->object.first;
			rtn >= 0 && NULL != ob;
			ob = (Object*)ob->id.next ) {
//--		if( !ob->data || NULL == ob->data || ID_FO != GS(((ID*)ob->data)->name) ) continue; // FObjects only
		if( !ob->data || ID_FO != GS(((ID*)ob->data)->name) || !(OB_FO_FOBJECT & ob->FOdynFlag) ) continue; // FObjects only

		obj._index = objects.size();
		rtn = obj.fromBlender( ob );
		if( rtn >= 0 ) {
			objects.push_back( obj );
			dwNObject++;
		}
	} // End for

	return( rtn );
} // End proc cObjectList::fromBlender
#endif //NO_BLENDER

//-----------------------------------------------------------------------------
// Name   : cObjectList::show
// Usage  : Dump cObjectList to screen
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         tab              X   -   (Optional) number of tabs before printing
//         where            X   -   (Optional) where to send data
//
// Return : Nothing
//-----------------------------------------------------------------------------
//
void cObjectList::show( const uint tab, FILE *where )
{
    const char * myTab = TABS(tab);
    FILE *out = ( where ? where : stdout );

    fprintf( out,
             "%sObjects = { -- NB objects: %d\n", myTab, dwNObject );
    for( uint i = 0; i < dwNObject; ++i ) {
        objects[i].show( tab + 1, where );
    } // End for
    fprintf( out, "%s}, -- End Objects\n", myTab );
	fflush( out );
} // End proc cObjectList::show

//-----------------------------------------------------------------------------
// Name   : cObject::cObject
// Usage  : cObject class constructor
// Args   : None
// Return : Nothing
//-----------------------------------------------------------------------------
cObject::cObject()
{
    dwID   = ID_OBJECT;
    sName  = NULL;
    sUnk1  = NULL;
    dwFlag = 0;

    _index   = 0;
    _filePos = 0;
#if !defined(NO_BLENDER)
    _obj = NULL;
#endif // !NO_BLENDER
} // End proc cObject::cObject

//-----------------------------------------------------------------------------
// Name   : cObject::cObject
// Usage  : Copy constructor
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         o                X   -   Source object
//
// Return : Nothing
//-----------------------------------------------------------------------------
cObject::cObject( const cObject& o )
{
    dwID   = o.dwID;
    sName  = ( o.sName ? strdup( o.sName ) : NULL );
    sUnk1  = ( o.sUnk1 ? strdup( o.sUnk1 ) : NULL );
    dwFlag = o.dwFlag;
	matrix = o.matrix;

    _index   = o._index;
    _filePos = o._filePos;

#if !defined(NO_BLENDER)
    _obj = o._obj;
#endif // !NO_BLENDER
} // End proc cObject::cObject

//-----------------------------------------------------------------------------
// Name   : cObject::read
// Usage  : Read cObject from file
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         h                X   X   File handler
//         idx              X   -   (Optiona) surface index
//
// Return : 0 == OK / -1 == read error / -2 == id error
//-----------------------------------------------------------------------------
int cObject::read( FILE * h, const uint idx )
{
    int rtn = -1;

    do {
        _index    = idx;
        _filePos  = ftell( h );

        if( 1 != fread( &dwID,   sizeof(dwID), 1, h ))   break;
        if( ID_OBJECT != dwID ) { rtn = -2; break; }

        if( NULL == ( sName = readString( h )))          break;
        if( NULL == ( sUnk1 = readString( h )))          break;
        if( 1 != fread( &dwFlag, sizeof(dwFlag), 1, h )) break;
        if(0 != matrix.read( h ))  break;

        rtn = 0;
    } while(0);

    return( rtn );
} // End proc cObject::read

//-----------------------------------------------------------------------------
// Name   : cObject::write
// Usage  : Write cObject to file
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         h                X   X   File handler
//
// Return : 0 == OK / -1 == write error / -2 == id error
//-----------------------------------------------------------------------------
int cObject::write( FILE * h )
{
    int rtn = -1;

    do {
        if( ID_OBJECT != dwID ) { rtn = -2; break; }
        if( 1 != fwrite( &dwID,   sizeof(dwID), 1, h ))   break;

        if( 0 != writeString( h, sName ))                 break;
        if( 0 != writeString( h, sUnk1 ))                 break;
        if( 1 != fwrite( &dwFlag, sizeof(dwFlag), 1, h )) break;
        if( 0 != matrix.write( h ))                       break;

        rtn = 0;
    } while(0);

    return( rtn );
} // End proc cObject::write

#if !defined(NO_BLENDER)
//-----------------------------------------------------------------------------
// Name   : cObject::toBlender
// Usage  : Draw in Blender
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         index            X   -   Index of object in list
//         track            X   -   Track object belongs to
//
// Return : 0 = success / -1 = error / -2 = memory error
//-----------------------------------------------------------------------------
int cObject::toBlender( const uint index, cTrack& track )
{
	Object  *ob = NULL;
	FObject *fob = NULL;
	char name[ID_NAME_LG];

	ob = add_object( OB_FOBJECT );
	if( ob ) fob = (FObject*)ob->data;
	if( !ob || !fob ) { SET_ERROR_AND_RETURN(-2); }

	_obj   = ob;
	_index = index;

	// Move, rotate and rename object
	snprintf( name, sizeof(name), "%s", sName );
	moveRotateObj( ob, &matrix, name );

	// Send to object layer
	sendToLayer( ob, LAYER_OBJECTS );

	// See if we can link object to a dynamic object...
	char *c = strchr( name, '#' );
	if( c ) *c = 0;
	c = strchr( name, '_' );
	if( 0 == strncasecmp( name, "dyn_", 4 )) c = strchr( c+1, '_' );
	if( c ) {
		Object *o = track.meshList.findMeshObject( c+1 );
		if( o ) {
			// Found! make it our parent...
			ob->parent = o;

			// Compute invert of parent matrix
			// to fix rotation & location
			// (o->invmat is not computed yet)
			float e[3];
			float m[4][4], i[4][4], c[4][4];

			// Parent matrix
			VECCOPY( e ,o->rot );
			EulToMat4( e, m );
			VECCOPY( m[3], o->loc );

			if( MTC_Mat4Invert( i, m ) ) {
				// Fix location
				MTC_Mat4MulVecfl( i, ob->loc );

				// Fix rotation
				VECCOPY( e ,ob->rot );
				EulToMat4( e, m );		// Object matrix

				MTC_Mat4MulMat4( c, m, i );
				Mat4ToEul( c, e );
				VECCOPY( ob->rot, e );
			} else { SET_ERROR_AND_RETURN(-1); }

			// Send to dynamic object layer
			sendToLayer( ob, LAYER_DYN_OBJ );
		}
	}

	// Name FObject
	snprintf( name, sizeof(name), "object_%d", index );
	rename_id( &fob->id, name );

	// FObject flags
	default_fobject( fob, sName );

	// Object flags
	ob->FOflag    = dwFlag;
	ob->FOdynFlag = OB_FO_FOBJECT;

	return( 0 );
} // End proc cObject::toBlender
#if 0
// For cars?
//-----------------------------------------------------------------------------
// Name   : cObject::toBlender
// Usage  : Draw in Blender
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         layer            X   -   Bone layer
//
// Return : Pointer to EditBone in case of success, NULL otherwise
//-----------------------------------------------------------------------------
#ifdef FO2_USE_ARMATURE
EditBone *cObject::toBlender( const int layer )
{
    EditBone *ebo = NULL;

    ebo = add_editbone();
    if( ebo ) {
        ebo->layer = layer;
        ebo->head[0] = 0.0;
        ebo->head[1] = 0.0;
        ebo->head[2] = 0.0;
        ebo->tail[0] = 1.0; //matrix.m[3][0];
        ebo->tail[1] = 1.0; //matrix.m[3][2];  // Y and Z are inverted
        ebo->tail[2] = 1.0; //matrix.m[3][1];

        // Rotate object
        HMatrix M;
        EulerAngles R;

        memcpy( M, matrix.m, sizeof(M) );

        // Y and Z are inverted!!
        R = Eul_FromHMatrix( M, EulOrdXZYr );

		printf("Bone '%s' : Rx=%f  Ry=%f  Rz=%f\n", ebo->name, R.x, R.y, R.z );

		float v[3], mat[4][4];
		v[0] = R.x; v[1] = R.y; v[2] = R.z;
		EulToMat4( v, mat );
		v[0] = 0.0; v[1] = -1.0; v[2] = 0.0;
		Mat4MulVecfl( mat, v );

		ebo->tail[0] = v[0];
		ebo->tail[1] = v[1];
        ebo->tail[2] = v[2];
	}

    return( ebo );
} // End proc cObject::toBlender
#else
int cObject::toBlender( Group* gr )
{
    int rtn = 0;
    Object    *ob = NULL;

    ob = add_object( OB_EMPTY );
    if( ob && gr ) add_to_group( gr, ob );

    if( NULL == ob ) {

        errLine = __LINE__;
        rtn = -1;

    } else {
        _obj = ob;

		VECCOPY( ob->loc, matrix.m[3] );

        ob->empty_drawtype = OB_PLAINAXES;
        ob->empty_drawsize = 0.25;

        rename_id( &ob->id, sName );
    }

    return( rtn );
} // End proc cObject::toBlender
#endif // FO2_USE_ARMATURE
#endif //0
#endif // NO_BLENDER

#if !defined(NO_BLENDER)
//-----------------------------------------------------------------------------
// Name   : cObject::fromBlender
// Usage  : Get model data from Blender
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         ob               X   -   Blender model object
//         track            X   -   Track object model belongs to
//
// Return : 0 in case of success, 1 for warning, -1 otherwise
//-----------------------------------------------------------------------------
int cObject::fromBlender( Object *ob )
{
	char name[ID_NAME_LG*2];
	char *c;
	if( !ob || NULL == ob->data || ID_FO != GS(((ID*)ob->data)->name) ) { SET_ERROR_AND_RETURN(-1); }

	_obj = ob;

	snprintf( name, sizeof(name), ob->id.name+2 );
	// If parent, use parent name as suffix
	if( ob->parent ) {
		char *n = strchr( name, '#' );
		char *c = strchr( name, '_' );
		if( 0 == strncasecmp( name, "dyn_", 4 )) c = strchr( c+1, '_' );
		uint idx = 0;
		if( n ) idx = atoi( n+1 );
		if( c ) {
			snprintf( c+1, sizeof(name)-(int)(c-name), "%s", ob->parent->id.name+2 );
			if( n ) {
				snprintf( name+strlen(name), sizeof(name)-strlen(name), "#%d", idx );
			}
		}
	}
	if( sName ) free( sName );
	sName = strdup( name );
	if( sUnk1 ) free( sUnk1 );
	sUnk1 = strdup( "" );

	// Object flags
    dwFlag = ob->FOflag;

	// -----------------------------------------------------------
	// FObject orientation & location
	// -----------------------------------------------------------
	memcpy( matrix.m, ob->obmat, sizeof(matrix.m) );

	return( 0 );
} // End proc cObject::fromBlender
#endif // NO_BLENDER

//-----------------------------------------------------------------------------
// Name   : cObject::show
// Usage  : Dump cObject to screen
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         tab              X   -   (Optional) number of tabs before printing
//         where            X   -   (Optional) where to send data
//
// Return : Nothing
//-----------------------------------------------------------------------------
void cObject::show( const uint tab, FILE *where )
{
    const char * myTab = TABS(tab);
    FILE *out = ( where ? where : stdout );

    fprintf( out, "%s{ -- object #%d @ 0x%08x\n", myTab, _index, _filePos );
    fprintf( out, "%s\tdwID    = 0x%08x (%c%c%c%c)\n",
             myTab, dwID, SHOW_ID(dwID) );
    fprintf( out, "%s\tsName   = %s\n",          myTab, sName );
    fprintf( out, "%s\tsUnk1   = %s\n",          myTab, sUnk1 );
    fprintf( out, "%s\tdwFlag  = 0x%08x (%d)\n", myTab, dwFlag, dwFlag );
    fprintf( out, "%s\tmatrix  = ", myTab );
    matrix.show( tab + 1, where );

    fprintf( out,
             "%s},\n", myTab );
	fflush( out );
} // End proc cObject::show

//-----------------------------------------------------------------------------
// Name   : cBoundingBoxList::read
// Usage  : Read cBoundingBoxList from file
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         h                X   X   File handler
//
// Return : 0 == OK / -1 == read error / -2 memory error
//-----------------------------------------------------------------------------
int cBoundingBoxList::read( FILE * h )
{
    int rtn = 0;
    cBoundingBox bb;

    boundingBoxes.clear(); dwNBoundingBox = 0;

    if( 1 != fread( &dwNBoundingBox, sizeof(dwNBoundingBox), 1, h )) return( -1 );

    if( dwNBoundingBox ) {
        boundingBoxes.reserve( dwNBoundingBox );
        if( boundingBoxes.capacity() < dwNBoundingBox ) return( -2 );

        for( uint i = 0; 0 == rtn && i < dwNBoundingBox; ++i ) {
            rtn = bb.read( h, i );
            if( 0 == rtn ) boundingBoxes.push_back( bb );
        } // End for
    } // End if

    return( rtn );
} // End proc cBoundingBoxList::read

//-----------------------------------------------------------------------------
// Name   : cBoundingBoxList::write
// Usage  : Write cBoundingBoxList to file
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         h                X   X   File handler
//
// Return : 0 == OK / -1 == write error / -2 memory error
//-----------------------------------------------------------------------------
int cBoundingBoxList::write( FILE * h )
{
    int rtn = 0;

    if( 1 != fwrite( &dwNBoundingBox,
                     sizeof(dwNBoundingBox), 1, h )) return( -1 );

    if( dwNBoundingBox ) {
        if( dwNBoundingBox != boundingBoxes.size() ) return( -2 );

        for( uint i = 0; 0 == rtn && i < dwNBoundingBox; ++i ) {
            rtn = boundingBoxes[i].write( h );
        } // End for
    } // End if

    return( rtn );
} // End proc cBoundingBoxList::write

//-----------------------------------------------------------------------------
// Name   : cBoundingBoxList::show
// Usage  : Dump cBoundingBoxList to screen
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         tab              X   -   (Optional) number of tabs before printing
//         where            X   -   (Optional) where to send data
//
// Return : Nothing
//-----------------------------------------------------------------------------
//
void cBoundingBoxList::show( const uint tab, FILE *where )
{
    const char * myTab = TABS(tab);
    FILE *out = ( where ? where : stdout );

    fprintf( out,
             "%sBoundingBoxes = { -- NB boundingBoxes: %d\n",
             myTab, dwNBoundingBox );
    for( uint i = 0; i < dwNBoundingBox; ++i ) {
        boundingBoxes[i].show( tab + 1, where );
    } // End for
    fprintf( out, "%s}, -- End BoundingBoxes\n", myTab );
	fflush( out );
} // End proc cBoundingBoxList::show

//-----------------------------------------------------------------------------
// Name   : cBoundingBox::cBoundingBox
// Usage  : cBoundingBox class constructor
// Args   : None
// Return : Nothing
//-----------------------------------------------------------------------------
cBoundingBox::cBoundingBox()
{
    dwNbIdx = 0;
    memset( dwModelIdx, 0, sizeof(dwModelIdx));

	bBoxCenterOffset.x = bBoxCenterOffset.y = bBoxCenterOffset.z = 0.0;

#if !defined(NO_BLENDER)
	_obj = NULL;
#endif //NO_BLENDER

	_index   = 0;
    _filePos = 0;
} // End proc cBoundingBox::cBoundingBox

//-----------------------------------------------------------------------------
// Name   : cBoundingBox::read
// Usage  : Read cBoundingBox from file
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         h                X   X   File handler
//         idx              X   -   (Optiona) surface index
//
// Return : 0 == OK / -1 == read error / -2 == id error
//-----------------------------------------------------------------------------
int cBoundingBox::read( FILE * h, const uint idx )
{
    int rtn = 0;

    do {
        _index    = idx;
        _filePos  = ftell( h );

        if(1 != fread( &dwNbIdx,    sizeof(dwNbIdx), 1, h )) { rtn=-1; break; }
        for( uint i = 0; i < dwNbIdx; ++i ) {
            if(1 != fread( &dwModelIdx[i], sizeof(dwModelIdx[0]), 1, h )) {
                rtn = -1;
                break;
            }
        } // End for
        if( 0 != rtn ) break;

        if( 0 != bBoxCenterOffset.read( h )) break;
        if( 0 != bBox.read( h )) break;

    } while(0);

    return( rtn );
} // End proc cBoundingBox::read

//-----------------------------------------------------------------------------
// Name   : cBoundingBox::write
// Usage  : Write cBoundingBox to file
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         h                X   X   File handler
//
// Return : 0 == OK / -1 == write error / -2 == id error
//-----------------------------------------------------------------------------
int cBoundingBox::write( FILE * h )
{
    int rtn = 0;

    do {
        if(1 != fwrite( &dwNbIdx,  sizeof(dwNbIdx), 1, h )) { rtn=-1; break; }
        for( uint i = 0; i < dwNbIdx; ++i ) {
            if(1 != fwrite( &dwModelIdx[i], sizeof(dwModelIdx[0]), 1, h )) {
                rtn = -1;
                break;
            }
        } // End for
        if( 0 != rtn ) break;

		if( 0 != bBoxCenterOffset.write( h )) break;
		if( 0 != bBox.write( h )) break;

    } while(0);

    return( rtn );
} // End proc cBoundingBox::read

#if !defined(NO_BLENDER)
//-----------------------------------------------------------------------------
// Name   : cBoundingBox::fromBlender
// Usage  : Get bounding box data from Blender
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         ob               X   -   Blender model object
//         mo               X   -   Flatout model object
//
// Return : 0 in case of success, 1 for warning, -1 otherwise
//-----------------------------------------------------------------------------
int cBoundingBox::fromBlender( Object *ob, cModel *mo )
{
    int  rtn = 0;

 	if( !ob || !mo ) { SET_ERROR_AND_RETURN(-1); }

	dwNbIdx = 0;
    memset( dwModelIdx, 0, sizeof(dwModelIdx));

	dwModelIdx[dwNbIdx++] = ob->par3;

	if( ob->lod1 ) dwModelIdx[dwNbIdx++] = ob->lod1->par3;
	if( ob->lod2 ) dwModelIdx[dwNbIdx++] = ob->lod2->par3;

	// Same as cModel
	bBoxCenterOffset = mo->bBoxCenterOffset;
	bBox             = mo->bBox;

	_obj = ob;

	return( rtn );
} // End proc cBoundingBox::fromBlender
#endif // NO_BLENDER

//-----------------------------------------------------------------------------
// Name   : cBoundingBox::show
// Usage  : Dump cBoundingBox to screen
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         tab              X   -   (Optional) number of tabs before printing
//         where            X   -   (Optional) where to send data
//
// Return : Nothing
//-----------------------------------------------------------------------------
void cBoundingBox::show( const uint tab, FILE *where )
{
    const char * myTab = TABS(tab);
    FILE *out = ( where ? where : stdout );
    uint i;

    fprintf( out, "%s{ -- boundingBox #%d @ 0x%08x\n",
             myTab, _index, _filePos );

    fprintf( out, "%s\tdwNbIdx         = 0x%08x (%d)\n",
             myTab, dwNbIdx, dwNbIdx );
    for( i = 0; i < dwNbIdx; ++i ) {
        fprintf( out, "%s\tdwModelIndex[%d] = 0x%08x (%d)\n",
                 myTab, i, dwModelIdx[i], dwModelIdx[i] );
    }

    fprintf( out, "%s\tbBoxCenterOffset = { ", myTab );
    bBoxCenterOffset.show( "", out );
    fprintf( out, "}\n" );
    fprintf( out, "%s\tbBox = { ", myTab );
    bBox.show( "", out );
    fprintf( out, "}\n" );

    fprintf( out,
             "%s},\n", myTab );
	fflush( out );
} // End proc cBoundingBox::show

//-----------------------------------------------------------------------------
// Name   : cMeshToBBoxList::read
// Usage  : Read cMeshToBBoxList from file
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         h                X   X   File handler
//
// Return : 0 == OK / -1 == read error / -2 memory error
//-----------------------------------------------------------------------------
int cMeshToBBoxList::read( FILE * h )
{
    int rtn = 0;
    cMeshToBBox m2bb;

    meshToBBoxes.clear(); dwNMeshToBBox = 0;

    if( 1 != fread( &dwNMeshToBBox, sizeof(dwNMeshToBBox), 1, h )) return(-1);

    if( dwNMeshToBBox ) {
        meshToBBoxes.reserve( dwNMeshToBBox );
        if( meshToBBoxes.capacity() < dwNMeshToBBox ) return( -2 );

        for( uint i = 0; 0 == rtn && i < dwNMeshToBBox; ++i ) {
            rtn = m2bb.read( h, i );
            if( 0 == rtn ) meshToBBoxes.push_back( m2bb );
			if( m2bb.sMeshPrefix ) { free( m2bb.sMeshPrefix ); m2bb.sMeshPrefix = NULL; }
        } // End for
    } // End if

    return( rtn );
} // End proc cMeshToBBoxList::read

//-----------------------------------------------------------------------------
// Name   : cMeshToBBoxList::write
// Usage  : Write cMeshToBBoxList to file
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         h                X   X   File handler
//
// Return : 0 == OK / -1 == write error / -2 memory error
//-----------------------------------------------------------------------------
int cMeshToBBoxList::write( FILE * h )
{
    int rtn = 0;

    if( 1 != fwrite( &dwNMeshToBBox, sizeof(dwNMeshToBBox), 1, h )) return(-1);

    if( dwNMeshToBBox ) {
        if( dwNMeshToBBox != meshToBBoxes.size() ) return( -2 );

        for( uint i = 0; 0 == rtn && i < dwNMeshToBBox; ++i ) {
            rtn = meshToBBoxes[i].write( h );
        } // End for
    } // End if

    return( rtn );
} // End proc cMeshToBBoxList::write

//-----------------------------------------------------------------------------
// Name   : cMeshToBBoxList::show
// Usage  : Dump cMeshToBBoxList to screen
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         tab              X   -   (Optional) number of tabs before printing
//         where            X   -   (Optional) where to send data
//
// Return : Nothing
//-----------------------------------------------------------------------------
//
void cMeshToBBoxList::show( const uint tab, FILE *where )
{
    const char * myTab = TABS(tab);
    FILE *out = ( where ? where : stdout );

    fprintf( out,
             "%sMeshToBBoxes = { -- NB meshToBBoxes: %d\n",
             myTab, dwNMeshToBBox );
    for( uint i = 0; i < dwNMeshToBBox; ++i ) {
        meshToBBoxes[i].show( tab + 1, where );
    } // End for
    fprintf( out, "%s}, -- End MeshToBBoxes\n", myTab );
	fflush( out );
} // End proc cMeshToBBoxList::show

//-----------------------------------------------------------------------------
// Name   : cMeshToBBox::cMeshToBBox
// Usage  : Copy constructor
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         m                X   -   Source object
//
// Return : Nothing
//-----------------------------------------------------------------------------
cMeshToBBox::cMeshToBBox( const cMeshToBBox& m )
{
	sMeshPrefix = ( m.sMeshPrefix ? strdup( m.sMeshPrefix ) : NULL );
	dwBBIdx[0]  = m.dwBBIdx[0];
	dwBBIdx[1]  = m.dwBBIdx[1];
	_index    = m._index;
	_filePos  = m._filePos;
} // End proc cMeshToBBox::cMeshToBBox

//-----------------------------------------------------------------------------
// Name   : cMeshToBBox::read
// Usage  : Read cMeshToBBox from file
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         h                X   X   File handler
//         idx              X   -   (Optiona) surface index
//
// Return : 0 == OK / -1 == read error / -2 == id error
//-----------------------------------------------------------------------------
int cMeshToBBox::read( FILE * h, const uint idx )
{
    int rtn = -1;

    do {
        _index    = idx;
        _filePos  = ftell( h );

        if( NULL == ( sMeshPrefix = readString( h )))     break;
        if(1 != fread( &dwBBIdx, sizeof(dwBBIdx), 1, h )) break;

        rtn = 0;
    } while(0);

    return( rtn );
} // End proc cMeshToBBox::read

//-----------------------------------------------------------------------------
// Name   : cMeshToBBox::write
// Usage  : Write cMeshToBBox to file
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         h                X   X   File handler
//
// Return : 0 == OK / -1 == write error / -2 == id error
//-----------------------------------------------------------------------------
int cMeshToBBox::write( FILE * h )
{
    int rtn = -1;

    do {
        if( 0 != writeString( h,sMeshPrefix ))              break;
        if( 1 != fwrite( &dwBBIdx, sizeof(dwBBIdx), 1, h )) break;

        rtn = 0;
    } while(0);

    return( rtn );
} // End proc cMeshToBBox::write

#if !defined(NO_BLENDER)
//-----------------------------------------------------------------------------
// Name   : cMeshToBBox::fromBlender
// Usage  : Get mesh to boudningbox data from Blender
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         ob               X   -   Blender model object
//
// Return : 0 in case of success, -1 otherwise
//-----------------------------------------------------------------------------
int cMeshToBBox::fromBlender( Object *ob )
{
    int  rtn = 0;
	char name[ID_NAME_LG];
	char *c;

	if( !ob ) { SET_ERROR_AND_RETURN(-1); }

	// Name of 1st mesh /wo any trailing number
	snprintf( name, sizeof(name), "%s", ob->id.name+2 );
	c = &name[strlen(name)];
	if( c != name ) {
		--c;
		while( c != name &&
				((*c >= '0' && *c <= '9') || '.' == *c || '@' == *c )) --c;
		*(c+1) = '\0';
	}
	if( sMeshPrefix ) free( sMeshPrefix );
	sMeshPrefix = strdup( name + ('_' == name[0] ? 1 : 0));

	dwBBIdx[0] = ob->par4;
	dwBBIdx[1] = ( ob->damage ? ob->damage->par4 : -1 );

	return( rtn );
} // End proc cMeshToBBox::fromBlender
#endif // NO_BLENDER

//-----------------------------------------------------------------------------
// Name   : cMeshToBBox::show
// Usage  : Dump cMeshToBBox to screen
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         tab              X   -   (Optional) number of tabs before printing
//         where            X   -   (Optional) where to send data
//
// Return : Nothing
//-----------------------------------------------------------------------------
void cMeshToBBox::show( const uint tab, FILE *where )
{
    const char * myTab = TABS(tab);
    FILE *out = ( where ? where : stdout );

    fprintf( out, "%s{ -- meshToBBox #%d @ 0x%08x\n",
             myTab, _index, _filePos );

    fprintf( out, "%s\tsMeshPrefix = %s\n",          myTab, sMeshPrefix );
    fprintf( out, "%s\tdwBBIdx[0]  = 0x%08x (%d)\n",
             myTab, dwBBIdx[0], dwBBIdx[0] );
    fprintf( out, "%s\tdwBBIdx[1]  = 0x%08x (%d)\n",
             myTab, dwBBIdx[1], dwBBIdx[1] );

    fprintf( out,
             "%s},\n", myTab );
	fflush( out );
} // End proc cMeshToBBox::show


//-----------------------------------------------------------------------------
// Name   : cMeshList::read
// Usage  : Read cMeshList from file
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         h                X   X   File handler
//
// Return : 0 == OK / -1 == read error / -2 memory error
//-----------------------------------------------------------------------------
int cMeshList::read( FILE * h )
{
    int rtn = 0;
    cMesh *pM;

    meshes.clear(); dwNMesh = 0;

    // TODO: CHECKME
    if( 1 != fread( &dwNGroup,  sizeof(dwNGroup), 1, h )) return( -1 );
    if( 1 != fread( &dwNMesh, sizeof(dwNMesh), 1, h )) return( -1 );

    if( dwNMesh ) {
        meshes.reserve( dwNMesh );
        if( meshes.capacity() < dwNMesh ) return( -2 );

        for( uint i = 0; 0 == rtn && i < dwNMesh; ++i ) {
            pM = new cMesh;
            if( pM ) {
                rtn = pM->read( h, i );
                if( 0 == rtn ) meshes.push_back( *pM );
                delete pM;
            } else {
                rtn =-2;
            }
        } // End for
    } // End if

    return( rtn );
} // End proc cMeshList::read

//-----------------------------------------------------------------------------
// Name   : cMeshList::write
// Usage  : Write cMeshList to file
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         h                X   X   File handler
//
// Return : 0 == OK / -1 == write error / -2 memory error
//-----------------------------------------------------------------------------
int cMeshList::write( FILE * h )
{
    int rtn = 0;

    // TODO: CHECKME
    if( 1 != fwrite( &dwNGroup,  sizeof(dwNGroup), 1, h )) return( -1 );
    if( 1 != fwrite( &dwNMesh, sizeof(dwNMesh), 1, h )) return( -1 );

    if( dwNMesh ) {
        if( dwNMesh != meshes.size() ) return( -2 );

        for( uint i = 0; 0 == rtn && i < dwNMesh; ++i ) {
            rtn = meshes[i].write( h );
        } // End for
    } // End if

    return( rtn );
} // End proc cMeshList::write

//-----------------------------------------------------------------------------
// Name   : cMeshList::clear
// Usage  : Delete all items from list
// Args   : None
//
// Return : Nothing
//-----------------------------------------------------------------------------
void cMeshList::clear( void )
{
    meshes.clear();
    dwNMesh  = 0;
    dwNGroup = 0;
} // End proc cMeshList::clear

#if !defined(NO_BLENDER)
//-----------------------------------------------------------------------------
// Name   : cMeshList::toBlender
// Usage  : Draw in Blender
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         track            X   -   Track object meshList belongs to
//
// Return : 0 in case of success, -1 otherwise
//-----------------------------------------------------------------------------
int cMeshList::toBlender( cTrack& track )
{
    uint meshIdx = 0;  // Mesh index
    int  rtn     = 0;

    for( meshIdx = 0; 0 == rtn && meshIdx < dwNMesh; ++meshIdx ) {
        rtn = meshes[meshIdx].toBlender( track );
    } // End for

    return( rtn );
} // End proc cMeshList::toBlender
#endif //NO_BLENDER

#if !defined(NO_BLENDER)
//-----------------------------------------------------------------------------
// Name   : cMeshList::fromBlender
// Usage  : Get dynamic object list from Blender
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         track            X   -   Track object meshList belongs to
//
// Return : 0 in case of success, -1 otherwise
//-----------------------------------------------------------------------------
int cMeshList::fromBlender( cTrack& track )
{
	int rtn = 0;
	int wng = 0;
    Object *ob;
    cMesh *pMesh;

	// Start with empty list
    clear();

	// --------------------------------------------------------------------
    //  Rebuild dynamic objects list
    // --------------------------------------------------------------------
    for( ob = (Object*)G.main->object.first; 0 == rtn && NULL != ob; ob = (Object*)ob->id.next ) {

		// Is this a mesh object ?
		if( !ob->data || NULL == ob->data || ID_ME != GS(((ID*)ob->data)->name) ) continue;

		// Process only dynamic objects
		if( OB_FO_DYNAMIC != ob->FOdynFlag ) continue;

		// Skip child objects...
		if( NULL != ob->parent ) continue;

		// Create new mesh
		pMesh = new cMesh;
		if( NULL == pMesh ) {
			printMsg( MSG_ALL, "Memory error @ %d\n", __LINE__ );
			rtn = -1;
			break;
		}

		rtn = pMesh->fromBlender( track, ob );
		if( rtn >= 0 ) {
			meshes.push_back( *pMesh );
			if( rtn > 0 ) { wng += 1; rtn = 0; }
		}

		delete pMesh;
	} // End for

	dwNMesh = meshes.size();

	return( rtn );
} // End proc cMeshList::fromBlender
#endif //NO_BLENDER

#if !defined(NO_BLENDER)
//-----------------------------------------------------------------------------
// Name   : cMeshList::findMeshObject
// Usage  : Look for a mesh by its name
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         name             X   -   Name of mesh to find
//
// Return : Pointer to Blender object, or NULL
//-----------------------------------------------------------------------------
Object *cMeshList::findMeshObject( const char* name )
{
	vector<cMesh>::iterator mesh;

	for( mesh = meshes.begin(); mesh != meshes.end(); mesh++ ) {
#if 0
		uint lg = strlen(name);
		if( 0 == strncmp( name, mesh->sName, lg ) &&
			( 0==mesh->sName[lg] || '@'==mesh->sName[lg] || '.'==mesh->sName[lg] ) ) {
			return( mesh->_obj );
		}
#endif //0
		if( 0 == strcasecmp( name, mesh->sName )) return( mesh->_obj );
	}
	return( NULL );
} // End proc cMeshList::findMeshObject
#endif //NO_BLENDER

//-----------------------------------------------------------------------------
// Name   : cMeshList::show
// Usage  : Dump cMeshList to screen
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         tab              X   -   (Optional) number of tabs before printing
//         where            X   -   (Optional) where to send data
//
// Return : Nothing
//-----------------------------------------------------------------------------
//
void cMeshList::show( const uint tab, FILE *where )
{
    const char * myTab = TABS(tab);
    FILE *out = ( where ? where : stdout );

    fprintf( out,
             "%sMeshes = { -- NB meshes: %d\n", myTab, dwNMesh );
    fprintf( out, "%s\tdwNGroup = 0x%08x (%d)\n", myTab, dwNGroup, dwNGroup );
    for( uint i = 0; i < dwNMesh; ++i ) {
        meshes[i].show( tab + 1, where );
    } // End for
    fprintf( out, "%s}, -- End Meshes\n", myTab );
	fflush( out );
} // End proc cMeshList::show

//-----------------------------------------------------------------------------
// Name   : cMesh::cMesh
// Usage  : Constructor
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         m                X   -   Source object
//
// Return : Nothing
//-----------------------------------------------------------------------------
cMesh::cMesh()
{
    dwID      = ID_MESH;
    sName     = NULL;
    sMadeOf   = NULL;
    dwFlag    = 0;
    dwGroup   = -1;
    dwUnk3    = 0;
    dwM2BBIdx = 0;

	_index    = 0;
	_filePos  = 0;
#if !defined(NO_BLENDER)
	_obj = NULL;
#endif //NO_BLENDER
} // End proc cMesh::cMesh

//-----------------------------------------------------------------------------
// Name   : cMesh::cMesh
// Usage  : Copy constructor
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         m                X   -   Source object
//
// Return : Nothing
//-----------------------------------------------------------------------------
cMesh::cMesh( const cMesh& m )
{
	dwID      = m.dwID;
	sName     = ( m.sName ? strdup( m.sName ) : NULL );
	sMadeOf   = ( m.sMadeOf ? strdup( m.sMadeOf ) : NULL );
	dwFlag    = m.dwFlag;
	dwGroup   = m.dwGroup;
	matrix    = m.matrix;
	dwUnk3    = m.dwUnk3;
	dwM2BBIdx = m.dwM2BBIdx;
	_index    = m._index;
	_filePos  = m._filePos;
#if !defined(NO_BLENDER)
	_obj      = m._obj;
#endif //NO_BLENDER
} // End proc cMesh::cMesh

//-----------------------------------------------------------------------------
// Name   : cMesh::~cMesh
// Usage  : Destructor
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         m                X   -   Source object
//
// Return : Nothing
//-----------------------------------------------------------------------------
cMesh::~cMesh()
{
	if( sName )   free( sName );
	if( sMadeOf ) free( sMadeOf );
} // End proc cMesh::cMesh

//-----------------------------------------------------------------------------
// Name   : cMesh::read
// Usage  : Read cMesh from file
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         h                X   X   File handler
//         idx              X   -   (Optiona) surface index
//
// Return : 0 == OK / -1 == read error / -2 == id error
//-----------------------------------------------------------------------------
int cMesh::read( FILE * h, const uint idx )
{
    int rtn = -1;

    do {
        _index    = idx;
        _filePos  = ftell( h );

        if(1 != fread( &dwID, sizeof(dwID), 1, h )) break;
        if( ID_MESH != dwID ) { rtn = -2; break; }

        if( NULL == ( sName   = readString( h )))       break;
        if( NULL == ( sMadeOf = readString( h )))       break;
        if(1 != fread( &dwFlag, sizeof(dwFlag), 1, h )) break;
        if(1 != fread( &dwGroup, sizeof(dwGroup), 1, h )) break;
        if(0 != matrix.read( h ))  break;
        if(1 != fread( &dwUnk3, sizeof(dwUnk3), 1, h )) break;
        if(1 != fread( &dwM2BBIdx, sizeof(dwM2BBIdx), 1, h )) break;

        rtn = 0;
    } while(0);

    return( rtn );
} // End proc cMesh::read

//-----------------------------------------------------------------------------
// Name   : cMesh::write
// Usage  : Write cMesh to file
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         h                X   X   File handler
//
// Return : 0 == OK / -1 == write error / -2 == id error
//-----------------------------------------------------------------------------
int cMesh::write( FILE * h )
{
    int rtn = -1;

    do {
        if( ID_MESH != dwID ) { rtn = -2; break; }
        if( 1 != fwrite( &dwID, sizeof(dwID), 1, h ))           break;

        if( 0 != writeString( h,sName ))                        break;
        if( 0 !=writeString( h, sMadeOf ))                      break;
        if( 1 != fwrite( &dwFlag, sizeof(dwFlag), 1, h ))       break;
        if( 1 != fwrite( &dwGroup, sizeof(dwGroup), 1, h ))     break;
        if( 0 != matrix.write( h ))                             break;
        if( 1 != fwrite( &dwUnk3, sizeof(dwUnk3), 1, h ))       break;
        if( 1 != fwrite( &dwM2BBIdx, sizeof(dwM2BBIdx), 1, h )) break;

        rtn = 0;
    } while(0);

    return( rtn );
} // End proc cMesh::write

#if !defined(NO_BLENDER)
//-----------------------------------------------------------------------------
// Name   : cMesh::toBlender
// Usage  : Draw in Blender
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         track            X   -   Track object meshList belongs to
//
// Return : 0 in case of success, -1 otherwise
//-----------------------------------------------------------------------------
int cMesh::toBlender( cTrack& track )
{
    uint          sIdx         = 0;  // Surface index
    cMeshToBBox  *pMeshToBB    = NULL;
    cBoundingBox *pBBox        = NULL;
    cModel       *pModel       = NULL;
    cBoundingBox *pBBoxDam     = NULL;
    cModel       *pModelDam    = NULL;
    Material     *ma           = NULL;
    Group        *gr           = NULL;
    int rtn = 0;

    do {
        // -----------------------------------------------------------
        // Get object data
        // -----------------------------------------------------------
        //   "meshToBB" instance
        pMeshToBB = &track.meshToBBoxList.meshToBBoxes[ dwM2BBIdx ];
        //   "boundingBox" instance
        pBBox = &track.boundingBoxList.boundingBoxes[ pMeshToBB->dwBBIdx[0] ];
        //   "model" instance
        pModel = &track.modelList.models[ pBBox->dwModelIdx[0] ];

#ifdef WITH_FO2_DBG
        printMsg( MSG_DBG, "=> Mesh %s\n", sName );
        printMsg( MSG_DBG, "\tMeshToBBox #%d (%s)\n",
                 pMeshToBB->_index, pMeshToBB->sMeshPrefix );
        printMsg( MSG_DBG, "\tBoundingBox #%d\n", pBBox->_index );
        printMsg( MSG_DBG, "\tModel #%d (%s)\n",
                 pModel->_index, pModel->sName );
#endif // WITH_FO2_DBG
        // -----------------------------------------------------------
        // Get damage data
        // -----------------------------------------------------------
        if( -1 != pMeshToBB->dwBBIdx[1] ) {
            //   "boundingBox" instance
            pBBoxDam = &track.boundingBoxList.boundingBoxes[ pMeshToBB->dwBBIdx[1] ];
            //   "model" instance
            pModelDam = &track.modelList.models[ pBBoxDam->dwModelIdx[0] ];
        } else {
            pBBoxDam     = NULL;
            pModelDam    = NULL;
        }

        // -----------------------------------------------------------
        // Create new Blender mesh
        // -----------------------------------------------------------
        cSurface *pSurf    = NULL;
        Object   *objP     = NULL;  // Parent object

        sIdx = (pModel->dwNSurfIdx - 1);
        pSurf = &track.surfaceList.surfaces[ pModel->dwSurfIdx[sIdx] ];

        objP = linkCopyObj( pSurf->_obj, LAYER_DYN_OBJ, &matrix, sName );
        if( NULL == objP ) { rtn = -1; break;}

		// For future use (objects)
		_obj = objP;

        // Copy flags
        objP->FOflag = dwFlag;
		objP->FOdynFlag = OB_FO_DYNAMIC;

        // -----------------------------------------------------------
        // Add "Lod" object(s)
        // -----------------------------------------------------------
        if( pBBox->dwNbIdx > 1 ) {
            cModel *pLod       = NULL;  // Lod model
            cSurface *pSurfLod = NULL;  // Lod surface
            int     dIdx       = 0;

            pLod = &track.modelList.models[ pBBox->dwModelIdx[1] ];
            dIdx = (pLod->dwNSurfIdx - 1);
            pSurfLod = &track.surfaceList.surfaces[ pLod->dwSurfIdx[dIdx] ];
            objP->lod1 = pSurfLod->_obj;
            if( NULL == pSurf->_obj->lod1 )
                pSurf->_obj->lod1 = pSurfLod->_obj;

            if( pBBox->dwNbIdx > 2 ) {
                pLod = &track.modelList.models[ pBBox->dwModelIdx[2] ];
                dIdx = (pLod->dwNSurfIdx - 1);
                pSurfLod = &track.surfaceList.surfaces[ pLod->dwSurfIdx[dIdx] ];
                objP->lod2 = pSurfLod->_obj;
                if( NULL == pSurf->_obj->lod2 )
                    pSurf->_obj->lod2 = pSurfLod->_obj;
            }
        } // End if (Lod)

        // -----------------------------------------------------------
        // Add "Damage" object(s)
        // -----------------------------------------------------------
        if( pBBoxDam ) {
            cModel *pDam       = NULL;  // Damage model
            cSurface *pSurfDam = NULL;  // Damage surface
            int     dIdx       = 0;

            pDam = &track.modelList.models[ pBBoxDam->dwModelIdx[0] ];
            dIdx = (pDam->dwNSurfIdx - 1);
            pSurfDam = &track.surfaceList.surfaces[ pDam->dwSurfIdx[dIdx] ];
            objP->damage = pSurfDam->_obj;
            if( NULL == pSurf->_obj->damage )
                pSurf->_obj->damage = pSurfDam->_obj;

            if( pBBoxDam->dwNbIdx > 1 ) {
                printMsg( MSG_ALL, "!! Damage BBox #%d has %d surfaces !!",
                        pBBoxDam->_index, pBBoxDam->dwNbIdx );
            }
        } // End if (damage)

        // -----------------------------------------------------------
        // Add dynamic physic
        // -----------------------------------------------------------
		objP->physic = findPhysic( sMadeOf );
		if( objP->physic ) {
			objP->physic->id.us++;	// Usage count
			if( objP->physic->id.us > 1 ) objP->physic->id.flag &= ~LIB_FAKEUSER;
		} else {
            printMsg( MSG_ALL, "[WNG] Cannot find physic '%s' for object '%s'\n", sMadeOf, objP->id.name+2 );
		}

        // -----------------------------------------------------------
        // Add group
        // -----------------------------------------------------------
        gr = NULL;
        if( -1 != dwGroup ) {
            char groupName[ID_NAME_LG];
            snprintf( groupName, sizeof(groupName),
                      FOGROUP_NAME "%c%03d", FO_SEPARATOR, dwGroup );
            gr = findGroup( groupName );
            if( NULL == gr ) {
                gr = add_group();
				gr->id.flag |= LIB_FAKEUSER;
                rename_id( &gr->id, groupName );
            }
        } // End if
        if( gr ) add_to_group( gr, objP );

        Object *objC  = NULL;  // Child object

        for( sIdx = 0; 0 == rtn && sIdx < (pModel->dwNSurfIdx-1); ++sIdx ) {
            // Model is made of several surfaces;
            // let's link them (parent/child link)
            pSurf = &track.surfaceList.surfaces[ pModel->dwSurfIdx[sIdx] ];
            // Do not use matrix, as children are linked to parent
            // (position && rotation)
            objC = linkCopyObj( pSurf->_obj, LAYER_DYN_OBJ, NULL, sName );

			objC->FOdynFlag = OB_FO_DYNAMIC;

            if( NULL == objC ) { rtn = -1; break;}
            if( gr ) add_to_group( gr, objC ); //20080323

            rtn = makeParent( objP, objC );

            // Set flags for group
            if( gr ) {
                if( objC ) objC->flag |= OB_FROMGROUP;
                // Update base too...
                for( Base *ba = (Base*)FIRSTBASE; //1st base in cur. scene
                     ba;
                     ba = ba->next ) {
                    if( ( objC && ( objC == ba->object )) ) {
                        ba->flag |= OB_FROMGROUP;
                    }
                } // End for
            }
        } // End for
        if( 0 != rtn ) break;

        if( gr ) {
            if( objP ) objP->flag |= OB_FROMGROUP;
            // Update base too...
            for( Base *ba = (Base*)FIRSTBASE; //1st base in cur. scene
                 ba;
                 ba = ba->next ) {
                if( ( objP && ( objP == ba->object )) ) {
                    ba->flag |= OB_FROMGROUP;
                }
            } // End for
        }

#if 0
        if( pModel->dwNSurfIdx > 1 ) {
            // Model is made of several surfaces;
            // let's link them (parent/child link)

            pSurf = &track.surfaceList.surfaces[ pModel->dwSurfIdx[sIdx-1] ];
            // Do not use matrix, as children are linked to parent
            // (position && rotation)
            objC = linkCopyObj( pSurf->_obj, LAYER_DYN_OBJ, NULL, sName );
            if( NULL == objC ) { rtn = -1; break;}

            if( gr ) add_to_group( gr, objC );

            objC->parent = objP;
            objC->recalc |= OB_RECALC_DATA;
        } // End if

        // Set flags for group
        if( gr ) {
            if( objP ) objP->flag |= OB_FROMGROUP;
            if( objC ) objC->flag |= OB_FROMGROUP;
            // Update base too...
            for( Base *ba = (Base*)FIRSTBASE; //1st base in cur. scene
                 ba;
                 ba = ba->next ) {
                if( ( objP && ( objP == ba->object )) ||
                    ( objC && ( objC == ba->object )) ) {
                    ba->flag |= OB_FROMGROUP;
                }
            } // End for
        }
#endif

#ifdef WITH_FO2_DBG
        printMsg( MSG_DBG, "mesh #%d (%s) : Model #%d (%s) x=%f y=%f z=%f\n",
                 _index, sName,
                 //pMeshToBB->dwBBIdx, pModel->sName,
				 pModel->_index, pModel->sName,
                 matrix.m[3][0],
                 matrix.m[3][1],
                 matrix.m[3][2] );

        printMsg( MSG_DBG, "objP: x=%f y=%f z=%f\n",
                 objP->loc[0],
                 objP->loc[1],
                 objP->loc[2] );
#endif // WITH_FO2_DBG

    } while(0);

    return( rtn );
} // End proc cMesh::toBlender
#endif //NO_BLENDER

#if !defined(NO_BLENDER)
//-----------------------------------------------------------------------------
// Name   : cMesh::fromBlender
// Usage  : Get dynamic object data from Blender object
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         track            X   -   Track object mesh belongs to
//         ob               X   -   Pointer to Blender object
//
// Return : 0 in case of success, -1 otherwise
//-----------------------------------------------------------------------------
int cMesh::fromBlender( cTrack& track, Object *ob )
{
	int rtn = 0;
	int wng = 0;
    Group   *gr = NULL;	// Blender group
    int group   = -1;	// FO2 group
    int m2bbIdx = -1;	// Index of "mesh to bouding box"

	// Assuming Blender object is correct!

	do {
		// -----------------------------------------------------------
		// Find group
		// -----------------------------------------------------------
		for( gr = (Group*)G.main->group.first;
				0 == rtn && gr;
				gr = (Group*)gr->id.next ) {
			if( object_in_group( ob, gr) ) {
				if( (strlen(gr->id.name+2) > LG_FOGROUP_NAME+1) &&
					0 == strncmp(gr->id.name+2,FOGROUP_NAME,LG_FOGROUP_NAME) &&
					FO_SEPARATOR == gr->id.name[2+LG_FOGROUP_NAME] ) {

					char *c = NULL;
					group = strtol( gr->id.name + 2 + LG_FOGROUP_NAME + 1,
									&c, 10 );
					if( NULL == c ||
						0 != *c ||
						ERANGE == errno ) {
						group = -1;

						printMsg( MSG_ALL, "[WNG] Object '%s': Invalid group (%s)\n",
								ob->id.name+2,
								gr->id.name+2 );
						rtn = 1;	// TODO: WARN ONLY?
						break;
					} else {
#ifdef WITH_FO2_DBG
						printMsg( MSG_DBG, "Group #%d\n", group );
#endif // WITH_FO2_DBG
						break;
					}
				}
			}
		} // Enf for (gr)
		if( 0 != rtn ) break;

		// -----------------------------------------------------------
		// Find mesh to bounding box index
		// -----------------------------------------------------------
		uint bbNameLg = 0;
		for( uint i=0; i < track.meshToBBoxList.dwNMeshToBBox; ++i ) {
			if( 0 == strncmp( track.meshToBBoxList.meshToBBoxes[i].sMeshPrefix,
								ob->id.name+2,
								strlen( track.meshToBBoxList.meshToBBoxes[i].sMeshPrefix ))) {
				// Find longest match...
				uint lg = strlen( track.meshToBBoxList.meshToBBoxes[i].sMeshPrefix );
				if( lg > bbNameLg ) {
					bbNameLg = lg;
					m2bbIdx = i;
				}
			}
		} // End for
		if( -1 == m2bbIdx ) {
			printMsg( MSG_ALL, "Object '%s': Cannot link mesh to bounding box\n",
					ob->id.name+2 );
			rtn = -1;
			break;
		}

		// -----------------------------------------------------------
		// Get dynamic object data
		// -----------------------------------------------------------
        sName     = strdup( ob->id.name+2 );
        if( NULL == ob->physic ) {
            printMsg( MSG_ALL, "[WNG] No physic assigned to object @ %d\n", __LINE__ );
            wng = 1;
        }
		sMadeOf   = strdup( ob->physic ? ob->physic->id.name+2 : "" );
        dwFlag    = ob->FOflag;
        dwGroup   = group;
        dwUnk3    = 1;
        dwM2BBIdx = m2bbIdx;

        // Update number of groups
        if( group > 0 && (uint)group >= track.meshList.dwNGroup ) {
            track.meshList.dwNGroup = group + 1;

#ifdef WITH_FO2_DBG
            printMsg( MSG_DBG, "dwNGroup #%d\n", track.meshList.dwNGroup );
#endif // WITH_FO2_DBG
        }

		// -----------------------------------------------------------
		// Object orientation & location
		// -----------------------------------------------------------
		memcpy( matrix.m, ob->obmat, sizeof(matrix.m) );

	} while(0);

	return( 0 == rtn ? wng : rtn );
} // End proc cMesh::fromBlender
#endif //NO_BLENDER

//-----------------------------------------------------------------------------
// Name   : cMesh::show
// Usage  : Dump cMesh to screen
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         tab              X   -   (Optional) number of tabs before printing
//         where            X   -   (Optional) where to send data
//
// Return : Nothing
//-----------------------------------------------------------------------------
void cMesh::show( const uint tab, FILE *where )
{
    const char * myTab = TABS(tab);
    FILE *out = ( where ? where : stdout );

    fprintf( out, "%s{ -- mesh #%d @ 0x%08x\n",
             myTab, _index, _filePos );

    fprintf( out, "%s\tdwID      = 0x%08x (%c%c%c%c)\n",
             myTab, dwID, SHOW_ID(dwID) );
    fprintf( out, "%s\tsName     = %s\n",          myTab, sName );
    fprintf( out, "%s\tsMadeOf   = %s\n",          myTab, sMadeOf );
    fprintf( out, "%s\tdwFlag    = 0x%08x (%d)\n", myTab, dwFlag,  dwFlag );
    fprintf( out, "%s\tdwGroup   = 0x%08x (%d)\n", myTab, dwGroup, dwGroup );
    fprintf( out, "%s\tmatrix  = ", myTab );
    matrix.show( tab + 1, where );
    fprintf( out, "%s\tdwUnk3    = 0x%08x (%d)\n", myTab, dwUnk3, dwUnk3 );
    fprintf( out, "%s\tdwM2BBIdx = 0x%08x (%d)\n",
             myTab, dwM2BBIdx, dwM2BBIdx );

    fprintf( out,
             "%s},\n", myTab );
	fflush( out );
} // End proc cMesh::show

//-----------------------------------------------------------------------------
// Name   : cStreamList::~cStreamList
// Usage  : cStreamList desctructor
// Args   : None
// Return : Nothing
//-----------------------------------------------------------------------------
//cStreamList::~cStreamList()
//{
//    if( streams ) {
//        for( uint i = 0; i < dwNstreams; ++i ) {
//            delete streams[i];
//        } // End for
//        free( streams );
//    } // End if
//} // End proc cStreamList::~cStreamList

//-----------------------------------------------------------------------------
// Name   : cStreamList::clear
// Usage  : Clear streams in list
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         track            X   -   Pointer to track stream list belongs to
//         modelsOnly       X   -   Clear only model streams
//
// Return : Nothing
//-----------------------------------------------------------------------------
void cStreamList::clear( cTrack *track, const bool modelsOnly ) {

	_vIdx = _iIdx = -1;
	if( !modelsOnly || NULL == track ) {
		for(unsigned int i =0; i < streams.size(); ++i ) {
			if( streams[i] ) delete streams[i];
		}
		streams.clear();
		dwNstreams = 0;
	} else if( modelsOnly && track ) {

		// Delete model vertex streams
		for(unsigned int i =0; i < streams.size(); ++i ) {
			if( STREAM_VERTEX_MODEL == streams[i]->dwFormat ) {
				delete streams[i];
				streams[i] = NULL;
				//CM dwNstreams--;
			}
		}

		// Delete index streams for models (assuming indexes for models
		// are at end of index stream)
		for( uint idx = 0; idx < streams.size(); ++idx ) {
			if( !streams[idx] || STREAM_TYPE_INDEX != streams[idx]->dwType ) continue;

			uint first = (uint)-1;
			for( uint i = 0; i < track->surfaceList.surfaces.size(); ++i ) {
				if( STREAM_VERTEX_MODEL == track->surfaceList.surfaces[i].dwFormat &&
					track->surfaceList.surfaces[i].dwIStreamIdx == idx ) {
					if( track->surfaceList.surfaces[i]._IStreamOffset < first ) first = track->surfaceList.surfaces[i]._IStreamOffset;
				}
			}

			cIndexStream *iS = (cIndexStream*)streams[idx];
			if( first < iS->indexes.size() ) {
				iS->indexes.erase( iS->indexes.begin() + first, iS->indexes.end() );
				iS->dwNum = iS->indexes.size();
				if( iS->indexes.empty() ) {
					delete streams[idx];
					streams[idx] = NULL;
					//CM dwNstreams--;
				}
			}
		}
	} // End if
} // End proc cStreamList::clear

//-----------------------------------------------------------------------------
// Name   : cStreamList::read
// Usage  : Read cStreamList from file
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         h                X   X   File handler
//
// Return : 0 == OK / -1 == read error / -2 memory error / -3 unknown stream
//-----------------------------------------------------------------------------
int cStreamList::read( FILE * h )
{
    int rtn = 0;
    uint streamType;

    _filePos = ftell( h );

	do {
		clear(NULL,false);

		if( 1 != fread( &dwNstreams, sizeof(dwNstreams), 1, h )) {
			rtn = -1;
			break;
		}

		if( dwNstreams ) {
			streams.reserve( dwNstreams );
			if( streams.capacity() < dwNstreams ) {
				errLine = __LINE__;
				rtn = -2;
				break;
			}

			for( uint i = 0; 0 == rtn && i < dwNstreams; ++i ) {
				// Look ahead stream type...
				if( 1 != fread( &streamType, sizeof(streamType), 1, h )) break;
				int seekPos = sizeof(streamType);
				fseek( h, - seekPos, SEEK_CUR );

				switch( streamType ) {
					case STREAM_TYPE_VERTEX: {
						cVertexStream *vS = new cVertexStream(0); // Stream format does not matter here (will be read from file)

						if( NULL == vS ) { rtn = -2; break; }
						vS->_index = streams.size();
						rtn = vS->read( h );
						if( 0 == rtn ) streams.push_back( vS );
						break;
					} // End case STREAM_TYPE_VERTEX

					case STREAM_TYPE_INDEX: {
						cIndexStream *iS = new cIndexStream;

						if( NULL == iS ) { rtn = -2; break; }
						iS->_index = streams.size();
						rtn = iS->read( h );
						if( 0 == rtn ) streams.push_back( iS );
						break;
					} // End case STREAM_TYPE_INDEX

					case STREAM_TYPE_UNKNOWN: {
						cUnknownStream *uS = new cUnknownStream;

						if( NULL == uS ) { rtn = -2; break; }
						uS->_index = streams.size();
						rtn = uS->read( h );
						if( 0 == rtn ) streams.push_back( uS );
						break;
					} // End case STREAM_TYPE_UNKNOWN

					default: {
						rtn = -3;
						break;
					}
				} // End switch
			} // End for
		} // End if (dwNstreams);
	} while(0);

    return( rtn );
} // End proc cStreamList::read

//-----------------------------------------------------------------------------
// Name   : cStreamList::write
// Usage  : Write cStreamList to file
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         h                X   X   File handler
//
// Return : 0 == OK / -1 == write error / -2 memory error / -3 unknown stream
//-----------------------------------------------------------------------------
int cStreamList::write( FILE * h )
{
    int rtn = 0;

    if( 1 != fwrite( &dwNstreams, sizeof(dwNstreams), 1, h )) return(-1);

    if( 0 == streams.size() ) return( -2 );

    for( uint i = 0; 0 == rtn && i < dwNstreams; ++i ) {

        switch( streams[i]->dwType ) {
        case STREAM_TYPE_VERTEX: {
            cVertexStream *pStream = (cVertexStream *)streams[i];
            if( NULL == pStream ) return( -2 );

            rtn = pStream->write( h );
            break;
        } // End case STREAM_TYPE_VERTEX

        case STREAM_TYPE_INDEX: {
            cIndexStream *pStream = (cIndexStream *)streams[i];
            if( NULL == pStream ) return( -2 );

            rtn = pStream->write( h );
            break;
        } // End case STREAM_TYPE_INDEX

        case STREAM_TYPE_UNKNOWN: {
            cUnknownStream *pStream = (cUnknownStream *)streams[i];
            if( NULL == pStream ) return( -2 );

            rtn = pStream->write( h );
            break;
        } // End case STREAM_TYPE_UNKNOWN

        default: {
            return( -3 );
        }
        } // End switch

    } // End for

    return( rtn );
} // End proc cStreamList::write

//-----------------------------------------------------------------------------
// Name   : cStreamList::show
// Usage  : Dump cStreamList to screen
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         tab              X   -   (Optional) number of tabs before printing
//         where            X   -   (Optional) where to send data
//
// Return : Nothing
//-----------------------------------------------------------------------------
void cStreamList::show( const uint tab, FILE *where )
{
    const char * myTab = TABS(tab);
    FILE *out = ( where ? where : stdout );

    fprintf( out, "%sStreams = { -- NB streams: %d\n",
             myTab, dwNstreams );
    for( uint i = 0; i < dwNstreams; ++i ) {
        if( streams[i] ) {
            streams[i]->show(i, tab + 1, where);
        }
    } // End for
    fprintf( out, "%s}, -- End Streams\n", myTab );
	fflush( out );
} // End proc cStreamList::show

#if !defined(NO_BLENDER)
typedef struct {
    int dwIStreamIdx;		// Index of index stream to use
	int _IStreamOffset;		// Index of 1st index in index stream
    uint dwFormat;			// Surface format
	uint lg;				// Stream data length

	cSurface *surf;			// Pointer to surface
} iStream_info;

static int cmpInfo( const void *a, const void *b )
{
	// Sort on stream index
	if( ((iStream_info*)a)->dwIStreamIdx != ((iStream_info*)b)->dwIStreamIdx ) {
		return( ((iStream_info*)a)->dwIStreamIdx < ((iStream_info*)b)->dwIStreamIdx ? -1 : 1 );
	}
	// Sort on format
	if( ((iStream_info*)a)->dwFormat != ((iStream_info*)b)->dwFormat ) {
		if( STREAM_VERTEX_MODEL == ((iStream_info*)a)->dwFormat ) return( 1 );
		if( STREAM_VERTEX_MODEL == ((iStream_info*)b)->dwFormat ) return( -1 );
		return( ((iStream_info*)a)->dwFormat < ((iStream_info*)b)->dwFormat ? -1 : 1 );
	}
	// Sort on index in list
	if( ((iStream_info*)a)->_IStreamOffset == ((iStream_info*)b)->_IStreamOffset ) return( 0 );
	return( ((iStream_info*)a)->_IStreamOffset < ((iStream_info*)b)->_IStreamOffset ? -1 : 1 );
}

static int cmpPos( const void *a, const void *b )
{
	// Sort on stream index
	if( ((iStream_info*)a)->dwIStreamIdx != ((iStream_info*)b)->dwIStreamIdx ) {
		return( ((iStream_info*)a)->dwIStreamIdx < ((iStream_info*)b)->dwIStreamIdx ? -1 : 1 );
	}
	// Sort on index in list
	if( ((iStream_info*)a)->_IStreamOffset == ((iStream_info*)b)->_IStreamOffset ) return( 0 );
	return( ((iStream_info*)a)->_IStreamOffset < ((iStream_info*)b)->_IStreamOffset ? -1 : 1 );
}
#endif // NO_BLENDER

#if !defined(NO_BLENDER)
//-----------------------------------------------------------------------------
// Name   : cStreamList::organize
// Usage  : Move models index at end of index streams
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         track            X   -   Track streamList belongs to
//
// Return : 0 in case of success, -1 otherwise
//-----------------------------------------------------------------------------
int cStreamList::organize( cTrack& track )
{
	int rtn = 0;
	iStream_info *info;
	uint nSurf = track.surfaceList.surfaces.size();

	if( nSurf ) {
		info = new iStream_info[nSurf];
		if( !info ) { SET_ERROR_AND_RETURN(-1); }

		for( uint i=0; i < nSurf; ++i ) {
			info[i].dwFormat       = track.surfaceList.surfaces[i].dwFormat;
			info[i].dwIStreamIdx   = track.surfaceList.surfaces[i].dwIStreamIdx;
			info[i]._IStreamOffset = track.surfaceList.surfaces[i].dwIStreamOffset/sizeof(ushort);
			info[i].surf           = &track.surfaceList.surfaces[i];
			info[i].lg             = 0;
			if( track.surfaceList.surfaces[i].dwIsNotMesh ) {
				info[i].dwIStreamIdx = info[i]._IStreamOffset = -1;
			}
		}

		qsort( info, nSurf, sizeof(iStream_info), cmpPos );

		// Compute length
		for( uint i=0; i < nSurf; ++i ) {
			if( info[i].dwIStreamIdx >= 0 ) {
				if( i == (nSurf-1) ||
					info[i].dwIStreamIdx != info[i+1].dwIStreamIdx ) {
					info[i].lg = ((cIndexStream*)track.streamList.streams[info[i].dwIStreamIdx])->indexes.size() - info[i]._IStreamOffset;
				} else {
					info[i].lg = info[i+1]._IStreamOffset - info[i]._IStreamOffset;
				}
			}
		}

		qsort( info, nSurf, sizeof(iStream_info), cmpInfo );

		// Rebuild streams
		vector<ushort> newIdx;
		uint j = 0;

		for( uint i = 0; i < track.streamList.streams.size(); ++i ) {
			if( STREAM_TYPE_INDEX == track.streamList.streams[i]->dwType ) {
				newIdx.clear();
				cIndexStream* iS = (cIndexStream*)track.streamList.streams[i];

				for( ; j < nSurf; j++ ) {
					if( info[j].dwIStreamIdx < 0 ) continue;
					if( info[j].dwIStreamIdx == i ) {
						vector<ushort>::iterator val = iS->indexes.begin()+info[j]._IStreamOffset;

						for( uint k = 0; k < info[j].lg; ++k, ++val ) {
							if( 0 == k ) {
								info[j].surf->_IStreamOffset  = newIdx.size();
								info[j].surf->dwIStreamOffset = info[j].surf->_IStreamOffset * sizeof(ushort);
							}
							newIdx.push_back( *val );
						} // End for (k)
					} else {
						break;
					}
				} // End for (j)

				iS->indexes = newIdx;
			}
		} // End for (i)

		if( info ) delete[] info;
	}

	return( rtn );
} // End proc cStreamList::show
#endif // NO_BLENDER

//-----------------------------------------------------------------------------
// Name   : cTrackHeader::read
// Usage  : Read cTrackHeader from file
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         h                X   X   File handler
//
// Return : 0 == OK / -1 == read error
//-----------------------------------------------------------------------------
int cTrackHeader::read( FILE *h )
{
    if( 1 != fread( &dwType,    sizeof(dwType), 1, h ))    return( -1 );
    if( 1 != fread( &dwUnknown, sizeof(dwUnknown), 1, h )) return( -1 );

    return( 0 );
} // End proc cTrackHeader::read

//-----------------------------------------------------------------------------
// Name   : cTrackHeader::write
// Usage  : Write cTrackHeader to file
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         h                X   X   File handler
//
// Return : 0 == OK / -1 == write error
//-----------------------------------------------------------------------------
int cTrackHeader::write( FILE *h )
{
    if( 1 != fwrite( &dwType,    sizeof(dwType), 1, h ))    return( -1 );
    if( 1 != fwrite( &dwUnknown, sizeof(dwUnknown), 1, h )) return( -1 );

    return( 0 );
} // End proc cTrackHeader::write

//-----------------------------------------------------------------------------
// Name   : cTrackHeader::show
// Usage  : Dump cTrackHeader to screen
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         tab              X   -   (Optional) number of tabs before printing
//         where            X   -   (Optional) where to send data
//
// Return : Nothing
//-----------------------------------------------------------------------------
void cTrackHeader::show( const uint tab, FILE *where )
{
    const char * myTab = TABS(tab);
    FILE *out = ( where ? where : stdout );

    fprintf( out, "%sFileHeader = {\n",     myTab );
    fprintf( out, "%s\tdwType    = 0x%08x\n", myTab, dwType );
    fprintf( out, "%s\tdwUnknown = 0x%08x\n", myTab, dwUnknown );
    fprintf( out, "%s}\n",                  myTab );
	fflush( out );
} // End proc cTrackHeader::show

//-----------------------------------------------------------------------------
// Name   : cTrack::read
// Usage  : Read cTrack from file
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         fileName         X   -   File to read
//         printOnLoad      X   -   (OPtional) Print data blocks while reading
//         name             X   -   (Optional) Track name
//         logFile          X   -   (Optional) log file name
//
// Return : 0 == OK / -1 == read error / -2 == file access error
//                    -3 == read data does not match file length
//-----------------------------------------------------------------------------
int cTrack::read( const char * const fileName,
                  const int printOnLoad,
                  const char * const name,
                  const char * const logFile )
{
    FILE *h;
    int rtn = 0;
    FILE *out = NULL;
    uint fileLength = 0;

    // For file identification...
    uint id;
    uint unknown;
    uint firstMat;

    do {
        if( name ) {
            if( trackName ) free( trackName );
            trackName = strdup( name );
        }

        if( logFile ) {
            if( _logFile ) free( _logFile );
            _logFile = strdup( logFile );
        }

        if( NULL == ( h = fopen( fileName, "rb" ))) { rtn = -2; break; }

        fseek( h, 0, SEEK_END );
        fileLength = ftell( h );
        fseek( h, 0, SEEK_SET );

        // Check this is a FlatOut2 track...
        if( 1 != fread( &id,       sizeof(id),       1, h ) ||
            1 != fread( &unknown,  sizeof(unknown),  1, h ) ||
            1 != fread( &firstMat, sizeof(firstMat), 1, h ) ||
            1 != fread( &firstMat, sizeof(firstMat), 1, h ) ||
            FO2_TRACK_FILE_ID      != id ||
            ID_MATERIAL            != firstMat ) {
            rtn = -1;
            break;
        }
        fseek( h, 0, SEEK_SET );


        if( printOnLoad && _logFile ) {
            out = fopen( _logFile, "w");
//             if( out ) printf( "Log file: %s\n", _logFile );
        }

        if( printOnLoad ) {
            out = (out ? out : stdout);
            fprintf( out, "{ -- Track '%s'\n", trackName );
        }

        // Read header
        rtn = header.read( h );
        _filePos = ftell( h );
        if( printOnLoad ) header.show( 1, out );
        if( rtn ) break;

        // Read materials
        rtn = materialList.read( h );
        _filePos = ftell( h );
        if( printOnLoad ) materialList.show( 1, out );
        if( rtn ) break;

        // Read streams
        rtn = streamList.read( h );
        _filePos = ftell( h );
        if( printOnLoad ) streamList.show( 1, out );
        if( rtn ) break;

        // Read surfaces
        rtn = surfaceList.read( h, streamList );
        _filePos = ftell( h );
        if( printOnLoad ) surfaceList.show( 1, out );
        if( rtn ) break;

        // Read surface centers
        rtn = surfcenterList.read( h );
        _filePos = ftell( h );
        if( printOnLoad ) surfcenterList.show( 1, out );
        if( rtn ) break;

        // Read unknown2s
        rtn = unknown2List.read( h );
        _filePos = ftell( h );
        if( printOnLoad ) unknown2List.show( 1, out );
        if( rtn ) break;

        // Read unknown3s
        rtn = unknown3List.read( h );
        _filePos = ftell( h );
        if( printOnLoad ) unknown3List.show( 1, out );
        if( rtn ) break;

        // Read unknown4s
        rtn = unknown4List.read( h );
        _filePos = ftell( h );
        if( printOnLoad ) unknown4List.show( 1, out );
        if( rtn ) break;

        // Read unknown5s
        rtn = unknown5List.read( h );
        _filePos = ftell( h );
        if( printOnLoad ) unknown5List.show( 1, out );
        if( rtn ) break;

        // Read models
        rtn = modelList.read( h );
        _filePos = ftell( h );
        if( printOnLoad ) modelList.show( 1, out );
        if( rtn ) break;

        // Read objects
        rtn = objectList.read( h );
        _filePos = ftell( h );
        if( printOnLoad ) objectList.show( 1, out );
        if( rtn ) break;

        // Read boundingBoxes
        rtn = boundingBoxList.read( h );
        _filePos = ftell( h );
        if( printOnLoad ) boundingBoxList.show( 1, out );
        if( rtn ) break;

        // Read meshToBBoxs
        rtn = meshToBBoxList.read( h );
        _filePos = ftell( h );
        if( printOnLoad ) meshToBBoxList.show( 1, out );
        if( rtn ) break;

        // Read meshes
        rtn = meshList.read( h );
        _filePos = ftell( h );
        if( printOnLoad ) meshList.show( 1, out );
        if( rtn ) break;

    } while(0);

    if( 0 == rtn &&
        printOnLoad ) fprintf( (out ? out : stdout),
                               "} -- FilePos = 0x%08x (FileLen = 0x%08x)\n",
                               _filePos, fileLength );
    if( h ) fclose( h );
    if( out && out != stdout ) fclose( out );

    if( 0 == rtn &&  _filePos != fileLength ) rtn = -3;

    return( rtn );
} // End proc cTrack::read

//-----------------------------------------------------------------------------
// Name   : cTrack::write
// Usage  : Write cTrack to file
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         fileName         X   -   File to read
//         printOnWrite     X   -   (OPtional) Print data blocks while writing
//
// Return : 0 == OK / -1 == write error / -2 == file access error
//-----------------------------------------------------------------------------
int cTrack::write( const char * const fileName,
                   const int printOnWrite )
{
    FILE *h;
    int rtn = 0;
    FILE *out = NULL;
    do {
        if( NULL == ( h = fopen( fileName, "wb" ))) { rtn = -2; break; }

        if( printOnWrite && _logFile ) {
            char *c;
            c = strstr( _logFile, FO2_READ_EXT FO2_LOG_EXT ); //TODO: CHANGEME?
            if( c ) sprintf( c, "%s%s", FO2_WRITE_EXT, FO2_LOG_EXT );
            out = fopen( _logFile, "w");
//             if( out ) printf( "Log file: %s\n", _logFile );
        }

        if( printOnWrite ) {
            out = (out ? out : stdout);
            fprintf( out, "{ -- Track '%s'\n", trackName );
        }

        // Write header
        rtn = header.write( h );
        if( printOnWrite ) header.show( 1, out );
        if( rtn < 0 ) { SET_ERROR_AND_BREAK(rtn,rtn); }

        // Write materials
        rtn = materialList.write( h );
        _filePos = ftell( h );
        if( printOnWrite ) materialList.show( 1, out );
        if( rtn < 0 ) { SET_ERROR_AND_BREAK(rtn,rtn); }

        // Write streams
        rtn = streamList.write( h );
        _filePos = ftell( h );
        if( printOnWrite ) streamList.show( 1, out );
        if( rtn < 0 ) { SET_ERROR_AND_BREAK(rtn,rtn); }

        // Write surfaces
        rtn = surfaceList.write( h );
        _filePos = ftell( h );
        if( printOnWrite ) surfaceList.show( 1, out );
        if( rtn < 0 ) { SET_ERROR_AND_BREAK(rtn,rtn); }

        // Write surface centers
        rtn = surfcenterList.write( h );
        _filePos = ftell( h );
        if( printOnWrite ) surfcenterList.show( 1, out );
        if( rtn < 0 ) { SET_ERROR_AND_BREAK(rtn,rtn); }

        // Write unknown2s
        rtn = unknown2List.write( h );
        _filePos = ftell( h );
        if( printOnWrite ) unknown2List.show( 1, out );
        if( rtn < 0 ) { SET_ERROR_AND_BREAK(rtn,rtn); }

        // Write unknown3s
        rtn = unknown3List.write( h );
        _filePos = ftell( h );
        if( printOnWrite ) unknown3List.show( 1, out );
        if( rtn < 0 ) { SET_ERROR_AND_BREAK(rtn,rtn); }

        // Write unknown4s
        rtn = unknown4List.write( h );
        _filePos = ftell( h );
        if( printOnWrite ) unknown4List.show( 1, out );
        if( rtn < 0 ) { SET_ERROR_AND_BREAK(rtn,rtn); }

        // Write unknown5s
        rtn = unknown5List.write( h );
        _filePos = ftell( h );
        if( printOnWrite ) unknown5List.show( 1, out );
        if( rtn < 0 ) { SET_ERROR_AND_BREAK(rtn,rtn); }

        // Write models
        rtn = modelList.write( h );
        _filePos = ftell( h );
        if( printOnWrite ) modelList.show( 1, out );
        if( rtn < 0 ) { SET_ERROR_AND_BREAK(rtn,rtn); }

        // Write objects
        rtn = objectList.write( h );
        _filePos = ftell( h );
        if( printOnWrite ) objectList.show( 1, out );
        if( rtn < 0 ) { SET_ERROR_AND_BREAK(rtn,rtn); }

        // Write boundingBoxes
        rtn = boundingBoxList.write( h );
        _filePos = ftell( h );
        if( printOnWrite ) boundingBoxList.show( 1, out );
        if( rtn < 0 ) { SET_ERROR_AND_BREAK(rtn,rtn); }

        // Write meshToBBoxs
        rtn = meshToBBoxList.write( h );
        _filePos = ftell( h );
        if( printOnWrite ) meshToBBoxList.show( 1, out );
        if( rtn < 0 ) { SET_ERROR_AND_BREAK(rtn,rtn); }

        // Write meshes
        rtn = meshList.write( h );
        _filePos = ftell( h );
        if( printOnWrite ) meshList.show( 1, out );
        if( rtn < 0 ) { SET_ERROR_AND_BREAK(rtn,rtn); }

    } while(0);

    if( printOnWrite ) {
        if( 0 == rtn ) {
            fprintf( (out ? out : stdout),
                     "} -- FilePos = 0x%08lx\n",
                     ftell(h) );
        } else {
            fprintf( (out ? out : stdout),
                     "!! ERROR on line %d (rtn=%d)!!\n", errLine,rtn );
        }
    }

    if( h ) fclose( h );
    if( out && out != stdout ) fclose( out );

    return( rtn );
} // End proc cTrack::write

//-----------------------------------------------------------------------------
// Name   : cTrack::show
// Usage  : Dump cTrack to screen
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         tab              X   -   (Optional) number of tabs before printing
//         where            X   -   (Optional) where to send data
//
// Return : Nothing
//-----------------------------------------------------------------------------
void cTrack::show( const uint tab, FILE *where )
{
    FILE *out = ( where ? where : stdout );

    fprintf( out, "{ -- Track '%s'\n", trackName );

    header.show( 1, where );
    materialList.show( 1, where );
    streamList.show( 1, where );
    surfaceList.show( 1, where );
    surfcenterList.show( 1, where );
    unknown2List.show( 1, where );
    unknown3List.show( 1, where );
    unknown4List.show( 1, where );
    unknown5List.show( 1, where );
    modelList.show( 1, where );
    objectList.show( 1, where );
    boundingBoxList.show( 1, where );
    meshToBBoxList.show( 1, where );
    meshList.show( 1, where );

    fprintf( out, "} -- FilePos = 0x%08x\n", _filePos );
	fflush( out );
} // End proc cTrack::show

//-----------------------------------------------------------------------------
// Name   : cTrack::calcLayers
// Usage  : Compute layers for objects
// Args   : None
//
// Return : 0 in case of success, -1 otherwise
//-----------------------------------------------------------------------------
int cTrack::calcLayers( void )
{
    uint curLayer = 1;
    uint prevStream = 0;
    int  rtn = -1;

    if( streamList.dwNstreams ) layers = (uint*)calloc( streamList.dwNstreams, sizeof(int));

    if( layers ) { // If it failed, then all in layer 0...
		uint l;

		for( l = 0; l < streamList.dwNstreams; ++l ) {
			if( STREAM_TYPE_INDEX != streamList.streams[l]->dwType ) {

				if( 0 != l &&
					( streamList.streams[prevStream]->dwNum < 32000 ||
					streamList.streams[prevStream]->dwFormat != streamList.streams[l]->dwFormat ) &&
					curLayer < LAST_USABLE_LAYER ) {
					// If not 1st stream and previous stream
					// is not full or is different, and not layer overflow,
					// go to next layer
					layers[l] = ++curLayer;
				} else {
					layers[l] = curLayer;
				}
				prevStream = l;
			} // End if (!STREAM_TYPE_INDEX)
		} // End for

		// Send dynamic object models to specific layer
		int dynObjLayer = getDynObjModelLayer();
		if( -1 != dynObjLayer ) {
			for( l = 0; l < streamList.dwNstreams; ++l ) {
				if( (uint)(dynObjLayer) == layers[l] ) {
					layers[l] = LAYER_DYN_MODELS;
				} else if( layers[l] > (uint)(dynObjLayer) ) {
					--layers[l];
				}
			}
		}

#ifdef WITH_FO2_DBG
		for( l = 0; l < streamList.dwNstreams; ++l ) {
			printMsg( MSG_DBG, "stream #%d: layer %d\n", l, layers[l] );
		}
#endif // WITH_FO2_DBG

		rtn = 0;
    } // End if

    return( rtn );
} // End proc cTrack::calcLayers

//-----------------------------------------------------------------------------
// Name   : cTrack::getDynObjModelLayer
// Usage  : Return index of layer for dynamic objects
// Args   : None
//
// Return : Dynamic objects layer index in case of success, -1 otherwise
//-----------------------------------------------------------------------------
int cTrack::getDynObjModelLayer( void ) {
    int layer = -1;

    cMeshToBBox  *pMeshToBB   = NULL;
    cBoundingBox *pBBox       = NULL;
    cModel       *pModel      = NULL;
    cSurface     *pSurf       = NULL;

    if( layers ) {
        pMeshToBB = &meshToBBoxList.meshToBBoxes[ 0 ];

        if( pMeshToBB ) {
            pBBox = &boundingBoxList.boundingBoxes[pMeshToBB->dwBBIdx[0]];

            if( pBBox ) {
                pModel = &modelList.models[ pBBox->dwModelIdx[0] ];

                if( pModel ) {
                    pSurf=&surfaceList.surfaces[pModel->dwSurfIdx[0]];

                    if( pSurf ) layer = layers[pSurf->dwVStreamIdx];
               }
            }
        }

    } // End if

    return( layer );
} // End cTrack::getDynObjModelLayer

//-----------------------------------------------------------------------------
// Name   : cTrack::sortStreams
// Usage  : Reorganize streams
// Args   : None
//
// Return : 0 in case of success, -1 otherwise
//-----------------------------------------------------------------------------
int cTrack::sortStreams( void )
{
	int rtn = 0;

	cStream **sList;
	uint    *sIdx;
	uint     nStreams,j;

	sList = new cStream*[streamList.streams.size()];
	sIdx  = new uint[streamList.streams.size()];

	for( nStreams = 0; nStreams < streamList.streams.size(); ++nStreams ) {
		sList[nStreams] = NULL;
		sIdx[nStreams]  = -1;
	}

	if( sList && sIdx ) {
		nStreams = 0;
		// Search for vertex streams
		for( j = 0; j < streamList.dwNstreams; ++j ) {
			if( streamList.streams[j] && STREAM_TYPE_VERTEX == streamList.streams[j]->dwType ) {
				sList[nStreams] = streamList.streams[j];
				sIdx[j]  = nStreams;
				++nStreams;
			}
		}
		// Search for unknown stream
		for( j = 0; j < streamList.dwNstreams; ++j ) {
			if( streamList.streams[j] && STREAM_TYPE_UNKNOWN == streamList.streams[j]->dwType ) {
				sList[nStreams] = streamList.streams[j];
				sIdx[j]  = nStreams;
				++nStreams;
			}
		}
		// Search for index streams
		for( j = 0; j < streamList.dwNstreams; ++j ) {
			if( streamList.streams[j] && STREAM_TYPE_INDEX == streamList.streams[j]->dwType ) {
				sList[nStreams] = streamList.streams[j];
				sIdx[j]  = nStreams;
				++nStreams;
			}
		}

		// Put back sorted list
		for( j = 0; j < nStreams; ++j ) {
			streamList.streams[j] = sList[j];
			streamList.streams[j]->_index = j;
		}
		// Fix list size
		streamList.dwNstreams = nStreams;
		streamList.streams.erase( streamList.streams.begin()+nStreams, streamList.streams.end() );
		// Fix surfaces
		vector<cSurface>::iterator surf;
		for( surf = surfaceList.surfaces.begin(); surf != surfaceList.surfaces.end(); surf++ ) {
			surf->dwIStreamIdx = sIdx[surf->dwIStreamIdx];
			surf->dwVStreamIdx = sIdx[surf->dwVStreamIdx];
		}
	} else {
		rtn = -1;
	}

	if( sList ) delete[] sList;
	if( sIdx ) delete[] sIdx;

	return( rtn );
} // End proc cTrack::sortStreams

#if !defined(NO_BLENDER)
//-----------------------------------------------------------------------------
// Name   : sendToLayer
// Usage  : Move Blender object (Mesh) to given layer
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         name             X   -   Track name
//
// Note   : If layer is wrong, nothing is done
//
// Return : Nothing
//-----------------------------------------------------------------------------
void sendToLayer( Object *obj, const int layer ) {
    Base *base;
    int local;

    // From movetolayer() @ source/blender/src/editobject.c

    if( layer >= LAYER_MIN && layer <= LAYER_MAX ) {
        /* update any bases pointing to our object */
        base = (Base*)FIRSTBASE;  /* first base in current scene */
        while( base ){
            if( base->object == obj ) {
                local = base->lay &= 0xFF000000;
                base->lay = local | (1 << (layer - 1));
                obj->lay = base->lay;
            }
            base = base->next;
        } // End while
    } // End if
} // End sendToLayer
#endif // NO_BLENDER

#if !defined(NO_BLENDER)
//-----------------------------------------------------------------------------
// Name   : findGroup
// Usage  : Find group object by its name
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         name             X   -   Name of group
//
// Return : Pointer to Group, or NULL
//-----------------------------------------------------------------------------
Group *findGroup( const char *name )
{
    Group *gr = NULL;

    if( name && *name ) {
        // Try to find group in group list
        for( gr = (Group*)G.main->group.first;
             NULL != gr;
             gr = (Group*)gr->id.next ) {
            // 2 first chars of id are object type...
            if( 0 == strcmp( gr->id.name + 2, name )) break;
        } // End for
    }

    return( gr );
} // End findGroup
#endif // NO_BLENDER

#if !defined(NO_BLENDER)
//-----------------------------------------------------------------------------
// Name   : findCurve
// Usage  : Find curve
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         name             X   -   Name of curve
//         object           -   X   (Optional) Pointer to object pointer
//
// Return : Pointer to Curve, or NULL
//-----------------------------------------------------------------------------
Curve *findCurve( const char *name, Object **object )
{
    Object *ob = NULL;

    if( name && *name ) {
        // Try to find curve in curve list
        for( ob = (Object*)G.main->object.first;
             NULL != ob;
             ob = (Object*)ob->id.next ) {
            // 2 first chars of id are object type...
            if( OB_CURVE == ob->type &&
                0 == strcmp( ob->id.name+2, name )) break;
        } // End for
    }

    if( object ) *object = ob;
    return( (Curve*)( ob ? ob->data : NULL ) );
} // End findCurve
#endif // NO_BLENDER

#if !defined(NO_BLENDER)
//-----------------------------------------------------------------------------
// Name   : findObject
// Usage  : Find Object by its name
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         name             X   -   Name of object
//
// Return : Pointer to Object, or NULL
//-----------------------------------------------------------------------------
Object *findObject( const char *name )
{
    Object *ob = NULL;

    if( name && *name ) {
        // Try to find object in object list
        for( ob = (Object*)G.main->object.first;
             NULL != ob;
             ob = (Object*)ob->id.next ) {
            // 2 first chars of id are object type...
            if( 0 == strcmp( ob->id.name+2, name )) break;
        } // End for
    }

    return( ob );
} // End findObject
#endif // NO_BLENDER

#if !defined(NO_BLENDER)
//-----------------------------------------------------------------------------
// Name   : findObjectMesh
// Usage  : Find Object by its name or mesh name
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         name             X   -   Name of object
//         mesh             X   -   Name of mesh object
//
// Return : Pointer to Object, or NULL
//-----------------------------------------------------------------------------
Object *findObjectMesh( const char *name, const char *mesh )
{
    Object *ob = NULL;

    if( name && *name ) {
        // Try to find object in object list
        for( ob = (Object*)G.main->object.first;
             NULL != ob;
             ob = (Object*)ob->id.next ) {
            // 2 first chars of id are object type...
            if( 0 == strcmp( ob->id.name+2, name ) ||
				( ob->data &&
				  NULL != ob->data &&
				  ID_ME == GS(((ID*)ob->data)->name) &&
				  0 == strcmp( ((ID*)ob->data)->name+2, name ))) {
				break;
			}
        } // End for
    }

    return( ob );
} // End findObjectMesh
#endif // NO_BLENDER

#if !defined(NO_BLENDER)
#ifdef WITH_FO2_DBG
extern "C" {
    void FO2_test_function(void);
}

//-----------------------------------------------------------------------------
// Name   : FO2_test_function
// Usage  : Print selected object(s) data
// Args   : None
//
// Return : Nothing
//-----------------------------------------------------------------------------
void FO2_test_function(void)
{

//     Curve *cu = NULL;
//     Object *ob = NULL;

    printf("\n\n---------------------------------\n");

	// Modify track data
	char *name  = "Q:\\FO2_Dev\\data\\tracks\\arena\\arena3\\test\\geometry\\track_geom.w32";
    cTrack   track;
    FO2_Path_t FO2path;
	int rtn = 0;

	// Read track
    rtn = buildPath( name, &FO2path, 1 );
	if( rtn < 0 ) { printf("Cannot build path\n"); return; }
	track._logFile = strdup( FO2path.logFile );
    dbg = fopen( FO2path.dbgFile, "w" );
    if( NULL == dbg ) dbg = stdout;
    rtn = track.read( name, 1, FO2path.trackName, FO2path.logFile );
    printMsg( MSG_ALL, "fo2_read_track() = %d (errLine = %d)\n", rtn, errLine );
    if( dbg != stdout ) fclose(dbg);
	if( track._logFile ) { free(track._logFile); track._logFile=NULL; }
	if( rtn < 0 ) { printf("Cannot read track\n"); return; }

	// Change some stuff here...
	for( uint i=0; i < track.streamList.dwNstreams; ++i ) {
		if( STREAM_TYPE_VERTEX == track.streamList.streams[i]->dwType ) {
			cVertexStream *vS = (cVertexStream *)track.streamList.streams[i];
			if( VERTEX_BLEND & vS->dwFormat ) {
				for( uint j=0; j < vS->dwNum; ++j ) {
					vS->vertices[j].fBlend = 0.0;
				}
			}
		}
	}

	// Write track
    rtn = buildPath( name, &FO2path, 0 );
	if( rtn < 0 ) { printf("Cannot build path\n"); return; }
	track._logFile = strdup( FO2path.logFile );
    dbg = fopen( FO2path.dbgFile, "w" );
    if( NULL == dbg ) dbg = stdout;
    rtn = track.write( name, 1 );
    printMsg( MSG_ALL, "fo2_write_track() = %d (errLine = %d)\n", rtn, errLine );
    if( dbg != stdout ) fclose(dbg);
	if( track._logFile ) { free(track._logFile); track._logFile=NULL; }
	if( rtn < 0 ) { printf("Cannot write track\n"); return; }


// ============================================================================
#if 0
	Physic *phy;
    // Print physic list
    for( phy = (Physic*)G.main->physic.first;
         NULL != phy;
         phy = (Physic*)phy->id.next ) {
		printf("Physic (%d) = %s\n", phy->id.us, phy->id.name );
    } // End for
#endif

// ============================================================================
#if 0
    // Print object and curve names
    for( ob = (Object*)G.main->object.first;
         NULL != ob;
         ob = (Object*)ob->id.next ) {
        // 2 first chars of id are object type...
        if( OB_CURVE == ob->type ) {
            cu = (Curve*)ob->data;
            printf("Curve = %s (%s)\n", ob->id.name, cu->id.name );
        }
    } // End for
#endif

// ============================================================================
#if 0
    // Add a poly curve
    // see addNurbprim() @ 3360 src/editcurve.c
#define NU_PT 23

    Object *ob;
    Curve *cu;
    Nurb *nu;
    BPoint *bp;

    float points[NU_PT][3] = {
        { 43.448532, 7.044425, 0.0 },
        { -73.252747, 9.976728, 0.0  },
        { -120.093109, 34.655659, 0.0  },
        { -132.788803, 72.447800, 0.0  },
        { -106.124367, 106.068802, 0.0  },
        { -60.014496, 97.308502, 0.0  },
        { -43.860035, 38.215496, 0.0  },
        { -45.678062, -68.492867, 0.0  },
        { -68.580338, -104.972534, 0.0  },
        { -113.182137, -103.496597, 0.0  },
        { -132.867676, -67.855453, 0.0  },
        { -119.452934, -32.150982, 0.0  },
        { -62.293510, -7.527875, 0.0  },
        { 95.879822, -11.144650, 0.0  },
        { 143.605286, -58.420052, 0.0  },
        { 124.697952, -103.375488, 0.0  },
        { 83.271057, -104.989319, 0.0  },
        { 58.691425, -65.157257, 0.0  },
        { 56.546890, 52.834045, 0.0  },
        { 79.766556, 102.947617, 0.0  },
        { 125.205170, 102.046341, 0.0  },
        { 144.850800, 63.941181, 0.0  },
        { 114.273529, 20.056503, 0.0  }
    };

    ob = add_object( OB_CURVE );
    cu = (Curve*)ob->data;
    cu->flag= CU_3D;

    nu = (Nurb*)MEM_callocN(sizeof(Nurb), "FO2_Nurb");
    nu->type= CU_POLY;
    nu->flagu= CU_CYCLIC;
    nu->resolu= 6;
    nu->resolv= 6;
    nu->pntsu= NU_PT;
    nu->pntsv= 1;
    nu->orderu= 4;
    nu->bp= (BPoint*)MEM_callocN(NU_PT*sizeof(BPoint), "FO2_Nurb3");

    bp= nu->bp;
    for( uint a = 0; a < NU_PT;a++, bp++) {
        VECCOPY(bp->vec, points[a]);
        bp->vec[3]= 1.0;
        bp->f1= 1;
    }

    BLI_addtail( &cu->nurb, nu );

    rename_id( &ob->id, "FO2_Curve" );
#endif

// ============================================================================
#if 0
    // Print curve properties
    Base *base;
    Curve *cu;
    Nurb *nu;

    printf("\n");
    base = (Base*)FIRSTBASE;  /* first base in current scene */
    while( base ){
        if( base->object &&
            (base->object->flag & SELECT) ) {
            printf( "------------------\n"
                    " %s\n", base->object->id.name );
            cu =  (Curve*)base->object->data;

            printf( "\tcu->flag = 0x%08x\n", cu->flag );

            nu= (Nurb*)cu->nurb.first;

            printf( "\tnu->type   = 0x%08x\n", nu->type );
            printf( "\tnu->flagu  = 0x%08x\n", nu->flagu );
            printf( "\tnu->resolu = %d\n", nu->resolu );
            printf( "\tnu->resolv = %d\n", nu->resolv );
            printf( "\tnu->pntsu  = %d\n", nu->pntsu );
            printf( "\tnu->pntsv  = %d\n", nu->pntsv );
            printf( "\tnu->orderu = %d\n", nu->orderu );

            for( uint i = 0; i < (uint)nu->pntsu; ++i ) {
                printf( "Point #%d : (%f, %f, %f)\n", i,
                        nu->bp[i].vec[0], nu->bp[i].vec[1], nu->bp[i].vec[2] );
            }
        }
        base = base->next;
    } // End while
#endif

// ============================================================================
#if 0
    // Print object properties
    Base *base;
    Material *ma;
    Group    *gr;
    Mesh     *me;
    int layernum;

    printf("\n");
    base = (Base*)FIRSTBASE;  /* first base in current scene */
    while( base ){
        if( base->object &&
            (base->object->flag & SELECT) ) {
            printf( "------------------\n"
                    " %s\n", base->object->id.name );
            printf( "\tob->FOflag    = 0x%08x\n", base->object->FOflag );
            printf( "\tob->flag      = 0x%08x\n", base->object->flag );
            printf( "\tba->flag      = 0x%08x\n", base->flag );
            printf( "\tob->trackflag = 0x%08x\n", base->object->trackflag );
            printf( "\tob->upflag    = 0x%08x\n", base->object->upflag );

            for( gr = (Group*)G.main->group.first;
                 gr;
                 gr = (Group*)gr->id.next ) {
		if( object_in_group( base->object, gr) ) break;
            }
           printf( "\tgroup = %s\n",
                    ( gr ? gr->id.name : "<none>" ));

            ma = give_current_material(base->object, base->object->actcol);
            printf( "\tmaterial = %s\n",
                    ( ma ? ma->id.name : "<none>" ));

            me = get_mesh( base->object );
            if( me ) {
                layernum= CustomData_number_of_layers(&me->fdata, CD_MTFACE);
                printf("\tnb CD_MTFACE = %d\n", layernum );
                layernum= CustomData_number_of_layers(&me->fdata, CD_MFACE);
                printf("\tnb CD_MFACE  = %d\n", layernum );
                printf("\tme->mr       = 0x%08x\n", (uint)me->mr );
            }
        }
        base = base->next;
    } // End while
#endif

} // End FO2_test_function
#endif // WITH_FO2_DBG
#endif // NO_BLENDER

#if !defined(NO_BLENDER)
//-----------------------------------------------------------------------------
// Name   : loadPhysics
// Usage  : Load dynamic objects file
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         path             X   -   Full path to file
//         dir              X   -   Directory name
//         verbose          X   -   Verbose mode
//
// Return : 0 in case of success, -1 if error, +1 if warning
//
// Note: If path is NULL, "dynamic_objects.bed" is searched in "dir" directory,
//       else "dir" is ignored
//-----------------------------------------------------------------------------
int loadPhysics( char *path, char *dir, int verbose )
{
    char name[256]; // File name
    int rtn = 0;

    char buffer[1024];
    char *c;
    regex_t re;
    regmatch_t pmatch[5];
	Physic *phy;

    do {
		int phyFnd= 0;	// Number of physic items found
		int phyAdd= 0;	// Number of physic items added
		int phyDel= 0;	// Number of physic items deleted
		int phyUse= 0;	// Number of physic items deleted while in use
		int phyErr= 0;	// Number of physic items failed to load
		int phyNam= 0;	// Number of physic items loaded with different name

		if( path ) {
			if( !*path ) { errLine = __LINE__; break; }
			snprintf( name, sizeof(name), "%s", path );
		} else if( dir && *dir ) {
			snprintf( name, sizeof(name),
				      "%s%cdynamic_objects.bed", dir, DIR_SEP );
		} else {
			errLine = __LINE__;
			break;
		}
        if( 0 != regcomp(&re, "^([a-z0-9A-Z_]+) += +{", REG_EXTENDED)) {
			errLine = __LINE__;
			rtn = -1;
			break;
		}

        FILE *h = fopen( name, "r" );

        if( !h ) {
			printMsg( MSG_ALL, "[ERR] Load physic: cannot read '%s'\n", name );
            errLine = __LINE__;
			rtn = -1;
			break;
        }

		// Switch off flag
        for( phy = (Physic*)G.main->physic.first; NULL != phy; phy = (Physic*)phy->id.next ) phy->par1= 0;

        while( fgets( buffer, sizeof(buffer), h )) {
            if( NULL != ( c = strrchr( buffer, '\n' ))) *c = 0;
            if( NULL != ( c = strrchr( buffer, '\r' ))) *c = 0;
            while( NULL != ( c = strchr( buffer, '\t' ))) *c = ' ';

            if( 0 == regexec( &re, buffer, 5, pmatch, 0 )) {
                char *matName = &buffer[pmatch[1].rm_so];
                buffer[pmatch[1].rm_eo]= 0;

                if( 0 != strncmp( "Bonus", matName, 5 )) {

					phyFnd++;
                    phy = findPhysic( matName );
					if( NULL == phy ) {
						phy = add_physic( matName );
						if( phy ) {
							phyAdd++;
							phy->id.flag |= LIB_FAKEUSER;
							phy->id.us = 1;
						}
					}
					if( NULL == phy ) {
						phyErr++;
						printMsg( MSG_ALL, "[ERR] Physic '%s' failed to load\n", matName );
						rtn = -1;
					} else {
						phy->par1= 1;
						if( 0 != strcmp( matName, phy->id.name+2 )) {
							phyNam++;
							printMsg( MSG_ALL, "[WNG] Physic '%s' loaded as '%s'\n", matName, phy->id.name+2 );
							if( rtn >= 0 ) rtn= 1;
						} else {
							if( verbose ) printMsg( MSG_CONSOLE, "      Physic '%s' loaded\n", matName );
						}
					}
                } // End if (Bonus)
            } // End if
        } // End while

		// Check for deleted items
		Physic *phyNext;
		for( phy = (Physic*)G.main->physic.first; NULL != phy; phy= phyNext ) {
			phyNext = (Physic*)phy->id.next; // Because current item may be deleted
			if( 0 == phy->par1 ) {
				phyDel++;
				if( phy->id.us > 1 ) {
					phyUse++;
					printMsg( MSG_ALL, "[WNG] Physic '%s' deleted (used by %d objects)\n", phy->id.name+2, phy->id.us-1 );
					if( rtn >= 0 ) rtn= 1;
				} else {
					if( verbose ) printMsg( MSG_CONSOLE, "      Physic '%s' deleted\n", phy->id.name+2 );
				}
				// Remove links to item
				Object *ob;
				for( ob = (Object*)G.main->object.first; NULL != ob; ob= (Object*)ob->id.next ) {
					if( ob->physic == phy ) ob->physic = NULL;
				}
				free_libblock(&G.main->physic, phy);
			}
		}

		if( verbose || 0!=rtn ) {
			printMsg( MSG_ALL, "\nLoad physic status for file '%s': %s\n",
				name, (0==rtn ? "OK" : (rtn>0 ? "<< WARNING >>" : "<< ERROR >>" )));
			if( verbose ) {
				printMsg( MSG_CONSOLE, "\tItems found in file          : %d\n"
						"\tItems added                  : %d\n"
						"\tItems removed                : %d\n"
						"\tItems removed while in use   : %d%s\n"
						"\tItems loaded with other name : %d\n"
						"\tItems failed to load         : %d\n\n",
						phyFnd, phyAdd, phyDel, phyUse, (phyUse?" <<<<":""),phyNam, phyErr );
			}
		}

        fclose( h );
        rtn = 0;
    } while(0);

    return( rtn );
} // End loadPhysics
#endif // NO_BLENDER

#if !defined(NO_BLENDER)
//-----------------------------------------------------------------------------
// Name   : makeParent
// Usage  : Link parent and child objects
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         parent           X   X   Parent object
//         child            X   X   Child object
//         moveChild        X   -   (Optional) Change coords & rot of child
//
// Return : 0 in case of success, -1 otherwise
//-----------------------------------------------------------------------------
int makeParent( Object *parent, Object *child /*, const int moveChild*/ ) {
	int rtn = 0;

	if( NULL == parent || NULL == child ) {
		errLine = __LINE__;
		rtn = -1;
	} else {
		// From make_parent() @ editobject.c
		// clearparentandkeeptransform child object
		if( child->parent ) {
			child->parent = NULL;
			apply_obmat( child );
		}

		parent->recalc |= OB_RECALC_OB;

		if( test_parent_loop( parent, child )) {
			errLine = __LINE__;
			rtn = -1; // Loop in parents
		} else {
			bool moveChild = true;	// TODO: This may not be necessary for cars!!

			child->recalc |= OB_RECALC_OB|OB_RECALC_DATA;

			child->partype = PAROBJECT;
			child->parent  = parent;
			what_does_parent(child);

			Mat4Invert(child->parentinv, workob.obmat);

			if( moveChild ) {
				child->loc[0] = parent->loc[0];
				child->loc[1] = parent->loc[1];
				child->loc[2] = parent->loc[2];

				child->rot[0] = parent->rot[0];
				child->rot[1] = parent->rot[1];
				child->rot[2] = parent->rot[2];
			}
		}
	}

	return( rtn );
} // End proc makeParent
#endif // NO_BLENDER

#if !defined(NO_BLENDER)
//-----------------------------------------------------------------------------
// Name   : fo2_read_track
// Usage  : Read FlatOut2 track file into Blender
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         name             X   -   Track name
//
// Return : 0 in case of success, -1 otherwise
//-----------------------------------------------------------------------------
static int fo2_read_track(char *name)
{
	cTrack   track;
	cTrackAI trackAI;
	FO2_Path_t FO2path;
	time_t duration;
	int rtn = 0;
	int wng = 0;

	// Make sure all models will be visible...
	if( G.vd ) {
		G.vd->trackvis = G.vd->trackvisact =( FO2_LAY_TRACK_A | FO2_LAY_TRACK_B | FO2_LAY_TRACK_C );
		G.vd->fobjectvis = FO2_FOB_VIS;
	}

	CLEAR_ERROR;
	waitcursor(1);

	printMsg( MSG_CONSOLE, "--- Start importing track '%s'\n", name );
	duration = time(NULL);

	do {
		// ---------------------------------------------------------------
		// ---   Get track paths
		// ---------------------------------------------------------------
		rtn = buildPath( name, &FO2path, 1 );
		BREAK_ON_ERROR(rtn,wng);

		track._logFile = strdup( FO2path.logFile );

#ifdef WITH_FO2_DBG
		dbg = fopen( FO2path.dbgFile, "w" );
		if( NULL == dbg ) dbg = stdout;

		printMsg( MSG_DBG, "--- Start importing track '%s'\n", name );
		printMsg( MSG_DBG, "Track file  : <%s>\n", name );
		printMsg( MSG_DBG, "Track name  : <%s>\n", FO2path.trackName );
		printMsg( MSG_DBG, "Base dir    : <%s>\n", FO2path.baseDir );
		printMsg( MSG_DBG, "Track dir   : <%s>\n", FO2path.geomDir );
		printMsg( MSG_DBG, "Light dir   : <%s>\n", FO2path.lightDir );
		printMsg( MSG_DBG, "Textures dir: <%s>\n", FO2path.texDir );
		printMsg( MSG_DBG, "Dynamics dir: <%s>\n", FO2path.dynDir );
		printMsg( MSG_DBG, "Log file    : <%s>\n", FO2path.logFile );
		printMsg( MSG_DBG, "Dbg file    : <%s>\n", FO2path.dbgFile );
#endif // WITH_FO2_DBG

		// ---------------------------------------------------------------
		// ---   Set scene name
		// ---------------------------------------------------------------
		Scene *sce = (Scene*)G.main->scene.first;
		if( sce ) {
			rename_id( &sce->id, FO2path.trackName );
		}

		// ---------------------------------------------------------------
		// ---   Read track data
		// ---------------------------------------------------------------
#ifdef WITH_FO2_DBG
		rtn = track.read( name, 1, FO2path.trackName, FO2path.logFile );
		if(0 == rtn) rtn = trackAI.read( FO2path.baseDir, 1, FO2path.logFile );
#else
		rtn = track.read( name, 0, FO2path.trackName, FO2path.logFile );
		if(0 == rtn) rtn = trackAI.read( FO2path.baseDir, 0, FO2path.logFile );
#endif // WITH_FO2_DBG
		BREAK_ON_ERROR(rtn,wng);

		// ---------------------------------------------------------------
		// ---   Read dynamic objects physic file
		// ---------------------------------------------------------------
		rtn = loadPhysics( NULL, FO2path.dynDir, 0 );
		BREAK_ON_ERROR(rtn,wng);

		// ---------------------------------------------------------------
		// ---   Process materials & texture images
		// ---------------------------------------------------------------
		rtn = track.materialList.toBlender( FO2path.lightDir, FO2path.texDir );
		BREAK_ON_ERROR(rtn,wng);

		// ---------------------------------------------------------------
		// ---   Compute layers
		// ---------------------------------------------------------------
		track.calcLayers();

		// ---------------------------------------------------------------
		// ---   Process surfaces (creates 3D objects)
		// ---------------------------------------------------------------
		rtn = track.surfaceList.toBlender( track );
		BREAK_ON_ERROR(rtn,wng);

		// ---------------------------------------------------------------
		// ---   Process models
		// ---------------------------------------------------------------
		rtn = track.modelList.toBlender( track );
		BREAK_ON_ERROR(rtn,wng);

		// ---------------------------------------------------------------
		// ---   Process dynamic objects (Meshes)
		// ---------------------------------------------------------------
		rtn = track.meshList.toBlender( track );
		BREAK_ON_ERROR(rtn,wng);

		// ---------------------------------------------------------------
		// ---   Process objects (must be after meshes!!)
		// ---------------------------------------------------------------
		rtn = track.objectList.toBlender( track );
		BREAK_ON_ERROR(rtn,wng);

		// ---------------------------------------------------------------
		// ---   Load AI data (splines, etc...)
		// ---------------------------------------------------------------
		trackAI.toBlender();

	} while(0);

	// ---------------------------------------------------------------
	// ---   Mark unused materials/physics for saving
	// ---------------------------------------------------------------
	if( rtn >= 0 ) {
		setFakeUser( ID_MA );	/* Materials */
		setFakeUser( ID_TE );	/* Textures */
		setFakeUser( ID_PH );	/* Physics */
	}

	allqueue(REDRAWVIEW3D, 0);
	allqueue(REDRAWOOPS, 0);

	DAG_scene_sort(G.scene);
//     DAG_scene_flush_update(G.scene, screen_view3d_layers());

	waitcursor(0);

	duration = time(NULL) - duration;
	printMsg( MSG_ALL, "--- End importing track '%s' (%ld'%02ld\") - %s\n",
			name, duration/60, duration%60, (0==rtn?(wng?"WARNING":"OK"):"ERROR"));
#ifdef WITH_FO2_DBG
	printMsg( MSG_DBG, "fo2_read_track() = %d (errLine = %d)\n", rtn, errLine );
	if( dbg != stdout ) fclose(dbg);

	if( 0 != rtn ) {
		printMsg( MSG_CONSOLE, "Error in fo2_read_track(), rtn=%d, line=%d\n", rtn, errLine );
		fflush( stdout );
	}
#endif // WITH_FO2_DBG

	return( rtn < 0 ? rtn : wng );
} // End proc fo2_read_track
#endif // NO_BLENDER

#if !defined(NO_BLENDER)
//-----------------------------------------------------------------------------
// Name   : fo2_write_track
// Usage  : Write FlatOut2 track file from Blender
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         name             X   -   Track name
//
// Return : 0 in case of success, -1 otherwise
//-----------------------------------------------------------------------------
static int fo2_write_track(char *name)
{
    cTrack   track;
    cTrackAI trackAI;
    FO2_Path_t FO2path;
    time_t duration;
    int rtn = 0;
	int wng = 0;

	CLEAR_ERROR;
    waitcursor(1);

    printMsg( MSG_CONSOLE, "--- Start exporting track '%s'\n", name );
    duration = time(NULL);

    do {
        // ---------------------------------------------------------------
        // ---   Get track paths
        // ---------------------------------------------------------------
        rtn = buildPath( name, &FO2path, 0 );
        BREAK_ON_ERROR(rtn,wng);

        if( trackAI._logFile ) free( trackAI._logFile );
        trackAI._logFile = strdup( FO2path.logFile );

#ifdef WITH_FO2_DBG
        dbg = fopen( FO2path.dbgFile, "w" );
        if( NULL == dbg ) dbg = stdout;

        printMsg( MSG_DBG, "--- Start exporting track '%s'\n", name );
        printMsg( MSG_DBG, "Track file  : <%s>\n", name );
        printMsg( MSG_DBG, "Track name  : <%s>\n", FO2path.trackName );
        printMsg( MSG_DBG, "Base dir    : <%s>\n", FO2path.baseDir );
        printMsg( MSG_DBG, "Track dir   : <%s>\n", FO2path.geomDir );
        printMsg( MSG_DBG, "Light dir   : <%s>\n", FO2path.lightDir );
        printMsg( MSG_DBG, "Textures dir: <%s>\n", FO2path.texDir );
        printMsg( MSG_DBG, "Dynamics dir: <%s>\n", FO2path.dynDir );
        printMsg( MSG_DBG, "Log file    : <%s>\n", FO2path.logFile );
        printMsg( MSG_DBG, "Dbg file    : <%s>\n", FO2path.dbgFile );
#endif // WITH_FO2_DBG

        // TODO: REMOVEME ... This is for alpha..
        rtn = track.read( name, 0, FO2path.trackName, FO2path.logFile );
        BREAK_ON_ERROR(rtn,wng);

        // TODO: REMOVEME ... This is for alpha..
		rtn = track.surfaceList.linkMaterials( track );
        BREAK_ON_ERROR(rtn,wng);
		wng = 0; // Cancel any read warning...

        // --------------------------------------------------------------------
        //  Rebuild material list
        // --------------------------------------------------------------------
		rtn = track.materialList.fromBlender();
        BREAK_ON_ERROR(rtn,wng);

//#ifdef FO2_WRITE_SURFACES
        // --------------------------------------------------------------------
        //  Sort index streams
        // --------------------------------------------------------------------
		rtn = track.streamList.organize( track );
        BREAK_ON_ERROR(rtn,wng);

		// --------------------------------------------------------------------
        //  Rebuild surface list
        // --------------------------------------------------------------------
		rtn = track.surfaceList.fromBlender( track, true );
        BREAK_ON_ERROR(rtn,wng);

        // --------------------------------------------------------------------
        //  Rebuild model list
        // --------------------------------------------------------------------
		rtn = track.modelList.fromBlender( track );
        BREAK_ON_ERROR(rtn,wng);

        // --------------------------------------------------------------------
        //  Sort streams
        // --------------------------------------------------------------------
		track.sortStreams();
		BREAK_ON_ERROR(rtn,wng);
//#endif // FO2_WRITE_SURFACES

        // --------------------------------------------------------------------
        //  Rebuild object list
        // --------------------------------------------------------------------
		rtn = track.objectList.fromBlender( track );
        BREAK_ON_ERROR(rtn,wng);

        // --------------------------------------------------------------------
        //  Rebuild dynamic objects list
        // --------------------------------------------------------------------
		rtn = track.meshList.fromBlender( track );
        BREAK_ON_ERROR(rtn,wng);

#ifdef WITH_FO2_DBG
        rtn = track.write( name, 1 );
#else
        rtn = track.write( name, 0 );
#endif // WITH_FO2_DBG
        BREAK_ON_ERROR(rtn,wng);

//             if( trackAI._logFile ) free( trackAI._logFile );
//             trackAI._logFile = strdup( logFile );

		rtn = trackAI.fromBlender();
        BREAK_ON_ERROR(rtn,wng);

#ifdef WITH_FO2_DBG
		rtn = trackAI.write( FO2path.baseDir, 1 );
#else
		rtn = trackAI.write( FO2path.baseDir, 0 );
#endif // WITH_FO2_DBG
    } while(0);

    waitcursor(0);

    duration = time(NULL) - duration;
    printMsg( MSG_ALL, "--- End exporting track '%s' (%ld'%02ld\") - %s",
		name, duration/60, duration%60, (0==rtn?(wng?"WARNING":"OK"):"ERROR"));
	if( 0 != rtn ) printMsg( MSG_ALL, " at line %d", errLine );
	printMsg( MSG_ALL, "\n" );

#ifdef WITH_FO2_DBG
    if( dbg && dbg != stdout ) fclose(dbg);
#endif // WITH_FO2_DBG

	return( rtn < 0 ? rtn : wng );
} // End proc fo2_write_track
#endif // NO_BLENDER

#if !defined(NO_BLENDER)
//-----------------------------------------------------------------------------
// Name   : FO2_read_track
// Usage  : Read FlatOut2 track into Blender (from Blender GUI)
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         name             X   -   Track file name
//
// Return : Nothing
//-----------------------------------------------------------------------------
void FO2_read_track( char *name )
{
    int rtn = fo2_read_track( name );

    if( rtn < 0 )
        error("Reading '%s', check console for details", name );
	else if( rtn > 0 )
        confirm("Attention", "There were some warnings reading '%s', check console for details", name );
} // End proc FO2_read_track
#endif // NO_BLENDER

#if !defined(NO_BLENDER)
//-----------------------------------------------------------------------------
// Name   : FO2_write_track
// Usage  : Write FlatOut2 track out of Blender (from Blender GUI)
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         name             X   -   Track file name
//
// Return : Nothing
//-----------------------------------------------------------------------------
void FO2_write_track( char *name )
{
    int rtn = -1;

    if(BLI_testextensie(name,".blend")) name[ strlen(name)-6]= 0;
    if(BLI_testextensie(name,".ble")) name[ strlen(name)-4]= 0;
    if(BLI_testextensie(name,".w32")==0) strcat(name, ".w32");

    if (!during_script()) {
        if (BLI_exists(name))
            if(saveover(name)==0)
                return;
    }

    rtn = fo2_write_track( name );
    if( rtn < 0 )
        error("Saving '%s', check console for details", name );
	else if( rtn > 0 )
        confirm("Attention", "There were some warnings saving '%s', check console for details", name );
} // End proc FO2_write_track
#endif // NO_BLENDER

#if !defined(NO_BLENDER)
//-----------------------------------------------------------------------------
// Name   : FO2_read_physic
// Usage  : Read FlatOut2 dynamic object physic into Blender (from Blender GUI)
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         name             X   -   Physic file name
//
// Return : Nothing
//-----------------------------------------------------------------------------
void FO2_read_physic( char *name )
{
    int rtn = loadPhysics( name, NULL, 1 );

    if( rtn < 0) {
        error("Reading '%s', check console for details", name );
	} else if( rtn > 0 ) {
		okee("Reading '%s', check console for details", name );
	}
} // End proc FO2_read_physic
#endif // NO_BLENDER

//-- End of file : FO2trackData.cpp  --
