//
//-=-=-=-=-=-=-=-=-=-=-=--=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// File        : $Id: FO2carData.cpp 605 2012-02-24 22:49:19Z GulBroz $
//   Revision  : $Revision: 605 $
//   Name      : $Name$
//
// Description : Class bodies to access FlatOut2 car files
// Author      :
// Date        : Tue Apr 17 21:17:12 2007
// Reader      :
// Evolutions  :
//
//  $Log$
//  Revision 1.1  2007/11/27 18:55:36  GulBroz
//  + Begin support for FO2 cars
//
//-=-=-=-=-=-=-=-=-=-=-=--=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//

// TODO: REMOVEME
//#define WITH_FO2_DBG

#ifdef WITH_FO2_CARS

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

// #include "FO2regex.h"
#include "FO2data.h"
#include "FO2carData.h"

#if !defined(NO_BLENDER)

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include "MEM_guardedalloc.h"

#include "MTC_matrixops.h"

#include "DNA_object_types.h"
#include "DNA_camera_types.h"
#include "DNA_image_types.h"
#include "DNA_mesh_types.h"
#include "DNA_material_types.h"
#include "DNA_meshdata_types.h"
#include "DNA_scene_types.h"
#include "DNA_group_types.h"
#include "DNA_texture_types.h"
#include "DNA_constraint_types.h"

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
#include "BKE_depsgraph.h"
#include "BKE_constraint.h"

#include "BDR_editface.h"   // default_tface()

#include "blendef.h" // FIRSTBASE

#include "BLI_blenlib.h"  // BLI_addhead()
#include "BLI_arithb.h"   // M_PI
#include "BIF_editconstraint.h"

#include "BIF_screen.h"   // screen_view3d_layers()
#include "mydevice.h"

#ifdef WITH_FO2_DBG
#include "DNA_curve_types.h"
#include "BKE_curve.h"
#include "BKE_utildefines.h" // VECCOPY
#endif // WITH_FO2_DBG

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // NO_BLENDER

//
//=============================================================================
//                                CONSTANTS
//=============================================================================
//

//
//=============================================================================
//                      TYPE DEFINITIONS AND GLOBAL DATA
//=============================================================================
//
char *readString( FILE * h );
int readString( FILE * h, string &str );
int writeString( FILE * h, const char *pStr );
int writeString( FILE * h, const string &str );

//
//=============================================================================
//                            INTERNAL ROUTINES
//=============================================================================
//

//-----------------------------------------------------------------------------
// Name   : buildPath
// Usage  : Compute track related paths
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         name             X   -   Path to track_geom.w32 file
//         pPath            -   X   Pointer to path structure
//         forReading       X   -   Read flag
//         fileType         X   -   Type of file
//
// Return : 0 == OK / -1 == error
//-----------------------------------------------------------------------------
static int buildPath( const char* name, FO2_Path_t *pPath,
                      const bool forReading, FO2_FileType_t fileType ) {
    int rtn = 0;
    char *pSep;

    do {
        if( NULL == name || NULL == pPath ) {
            errLine = __LINE__;
            rtn = -1;
            break;
        }

        memset( pPath, 0, sizeof(*pPath) );

// printf("=> name = %s \n", name );
        // Compute car directory
        snprintf( pPath->geomDir, sizeof(pPath->geomDir),
                  "%s", name );
        pSep = (char*)strrchr( pPath->geomDir, DIR_SEP );
        if( !pSep ) { errLine = __LINE__; rtn = -1; break; }
        *pSep = 0;
// printf("=> geomDir = %s \n", pPath->geomDir );

        // Compute base directory
        snprintf( pPath->baseDir, sizeof(pPath->baseDir),
                  "%s", pPath->geomDir );
        pSep = strrchr( pPath->baseDir, DIR_SEP );
        if( !pSep ) { errLine = __LINE__; rtn = -1; break; }
        *pSep = 0;
// printf("=> baseDir = %s \n", pPath->baseDir );

        // Compute car name
        if( FO2_MENUCAR == fileType ) {
            pSep = (char*)strrchr( name, DIR_SEP );
            if( !pSep ||
                0 != strncmp( pSep+1, "menucar_", 8 )) {
                errLine = __LINE__;
                rtn = -1;
                break;
            }

            snprintf( pPath->trackName, sizeof(pPath->trackName),
                      "%s", (pSep+5) );
            pSep = strrchr( pPath->trackName, '.' );
            if( !pSep ) { errLine = __LINE__; rtn = -1; break; }
            *pSep = 0;
        } else {
            snprintf( pPath->trackName, sizeof(pPath->trackName),
                      "%s", (pSep+1) );
        }
// printf("=> carName = %s \n", pPath->trackName );

        // Compute log file name
        snprintf( pPath->logFile, sizeof(pPath->logFile),
                  "%s", pPath->baseDir );
        pSep = strrchr( pPath->logFile, DIR_SEP );
        if( !pSep ) { errLine = __LINE__; rtn = -1; break; }
        *pSep = 0;

        snprintf( pSep, sizeof(pPath->logFile)-strlen(pPath->logFile),
                  "%clogs%c%s%s%s",
                  DIR_SEP, DIR_SEP, pPath->trackName,
                  (forReading ? FO2_READ_EXT : FO2_WRITE_EXT), FO2_LOG_EXT );
// printf("=> logFile = %s \n", logFile );

#ifdef WITH_FO2_DBG
        snprintf( pPath->dbgFile, sizeof(pPath->dbgFile),
                  "%s", pPath->logFile );
        pSep = strstr( pPath->dbgFile, FO2_LOG_EXT );
        if( pSep ) sprintf( pSep, "%s", FO2_DBG_EXT );
#endif // WITH_FO2_DBG

        // Compute shared directory
        if( FO2_MENUCAR == fileType ) {
            snprintf( pPath->lightDir, sizeof(pPath->lightDir),
                      "%s", pPath->baseDir );
            pSep = strrchr( pPath->lightDir, DIR_SEP );
            if( !pSep ) { errLine = __LINE__; rtn = -1; break; }
            *pSep = 0;

            snprintf( pSep, sizeof(pPath->lightDir)-strlen(pPath->lightDir),
                      "%ccars%cshared", DIR_SEP, DIR_SEP );
        } else {
            snprintf( pPath->lightDir, sizeof(pPath->lightDir),
                      "%s%cshared", pPath->baseDir, DIR_SEP );
        }

        // Compute texture directory
        if( FO2_MENUCAR == fileType ) {
            snprintf( pPath->texDir, sizeof(pPath->texDir),
                      "%s%c%s", pPath->geomDir, DIR_SEP, pPath->trackName );
        } else {
            snprintf( pPath->texDir, sizeof(pPath->texDir),
                      "%s", pPath->geomDir );
        }

        // Compute dynamics directory
        pPath->dynDir[0] = 0;
    } while(0);

#ifdef WITH_FO2_DBG
    if( pPath ) {
        printf( "Car file    : <%s>\n", name );
        printf( "Car name    : <%s>\n", pPath->trackName );
        printf( "Base dir    : <%s>\n", pPath->baseDir );
        printf( "Car dir     : <%s>\n", pPath->geomDir );
        printf( "Shared dir  : <%s>\n", pPath->lightDir );
        printf( "Textures dir: <%s>\n", pPath->texDir );
        printf( "Dynamics dir: <%s>\n", pPath->dynDir );
        printf( "Log file    : <%s>\n", pPath->logFile );
        printf( "Dbg file    : <%s>\n", pPath->dbgFile );
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
// Name   : cCamera::cCamera
// Usage  : cCamera class constructor
// Args   : None
//
// Return :
//-----------------------------------------------------------------------------
cCamera::cCamera( void )
{
    stateOwner = LuaState::Create(true);
    luaState = stateOwner;

    L = ( luaState ? luaState->GetCState() : NULL );

    // Problem... TODO: FIXME
//     if( !luaState || !L ) return( -1 );

    // Open libs
    lua_gc(L, LUA_GCSTOP, 0);  /* stop collector during initialization */
    luaL_openlibs(L);  /* open libraries */
    lua_gc(L, LUA_GCRESTART, 0);
} // End proc cCamera::cCamera

//-----------------------------------------------------------------------------
// Name   : cCamera::~cCamera
// Usage  : cCamera class destructor
// Args   : None
//
// Return : Nothing
//-----------------------------------------------------------------------------
cCamera::~cCamera( void )
{
    // Nothing, all is managed by stateOwner desctruction...
} // End proc cCamera::~cCamera

//-----------------------------------------------------------------------------
// Name   : cCamera::read
// Usage  : Read cCamera from file
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         fileName         X   -   Name of camera file to load
//
// Return : 0 == OK / -1 == read error
//-----------------------------------------------------------------------------
int cCamera::read( const char * const fileName )
{
    int rtn = -1;

    if( luaState &&
        0 == luaState->DoFile( fileName )) rtn = 0;

    return( rtn );
} // End proc cCamera::read

//-----------------------------------------------------------------------------
// Name   : cCamera::write
// Usage  : Write cCamera from file
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         fileName         X   -   Name of camera file to write
//
// Return : 0 == OK / -1 == read error / -2 memory error
//-----------------------------------------------------------------------------
int cCamera::write( const char * const fileName )
{
    (void)fileName;

    return(0);
} // End proc cCamera::write

#if !defined(NO_BLENDER)
#define MAX_DEF_CAM 5
float camOffsets[MAX_DEF_CAM][3] = {
    {  0.0,  0.51, 0.18 },
    {  0.0,  0.51, 0.18 },
    {  0.0,  0.66, 0.18 },
    {  0.0,  0.51, 0.18 },
    { -0.39, 0.27, 0.60 }
};
uint camLens[MAX_DEF_CAM] = { 15, 14, 13, 14, 15 };

//-----------------------------------------------------------------------------
// Name   : cCamera::toBlender
// Usage  : Draw in Blender
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         gr               X   -   (Optional) Object group
//         carHdl           X   -   (Optional) Car handler
//
// Return : 0 in case of success, -1 otherwise
//-----------------------------------------------------------------------------
int cCamera::toBlender( Group* gr, Object *carHdl )
{
    int rtn = -1;
	LuaObject cameras;
    uint i = 0;
    uint nbCam = 0;
    string str;
    LuaObject camClip;
    char name[ID_NAME_LG];

    Object *fl    = findObject( "placeholder_tire_fl" );
    Object *fr    = findObject( "placeholder_tire_fr" );
    Object *rr    = findObject( "placeholder_tire_rr" );
    Object *hood  = findObject( "hood_joint" );
    Object *wheel = findObject( "steering_wheel_lo" );

    float carLength = fr->loc[1] - rr->loc[1];
    camOffsets[0][1] = carLength/6;
    camOffsets[1][1] = carLength/6;
    camOffsets[2][1] = carLength/4.6;
    camOffsets[3][1] = carLength/6;
    camOffsets[4][1] = carLength/11.2;

    camOffsets[0][2] = hood->loc[2]/5.6;
    camOffsets[1][2] = hood->loc[2]/5.6;
    camOffsets[2][2] = hood->loc[2]/5.6;
    camOffsets[3][2] = hood->loc[2]/5.6;
    camOffsets[4][2] = hood->loc[2]/1.7;

    camOffsets[4][0] = wheel->loc[0];


    do {
        if( NULL == luaState ) { errLine = __LINE__; break; }

        cameras = luaState->GetGlobal( FO2_CARCAM_OBJ );
        if( !cameras.IsTable() ) { errLine = __LINE__; break; }

        nbCam = cameras.GetCount();
        for( i = 1; i <= nbCam; ++i ) {
            LuaObject camera = cameras[i];
            LuaObject camPos = camera.Lookup( "PositionFrames.1.Offset" );
            LuaObject camTgt = camera.Lookup( "TargetFrames.1.Offset" );

            if( !camPos.IsTable() ) { errLine = __LINE__; break; }

            Object *obTgt = NULL;
            Object *ob    = NULL;
            Camera *ca    = NULL;

            ob = add_object( OB_CAMERA );
            if( ob ) ca = (Camera*)ob->data;
            if( NULL == ob || NULL == ca ) { errLine = __LINE__; break; }
            if( gr ) add_to_group( gr, ob );

            snprintf( name, sizeof(name), "Camera.%03d", i );
            rename_id( &ob->id, name );
            rename_id( &ca->id, name );

            // Compute cameras offsets

            ob->loc[0] = camPos[1].GetFloat(); // X
            ob->loc[1] = camPos[3].GetFloat(); // Y (Y and Z are inverted)
            ob->loc[2] = camPos[2].GetFloat(); // Z
//#if 0
            if( i <= MAX_DEF_CAM ) {
                ob->loc[0] += camOffsets[i-1][0];
                ob->loc[1] += camOffsets[i-1][1];
                ob->loc[2] += camOffsets[i-1][2];
            }
//#endif
            ob->rot[0] = M_PI/2; // Rx 90 deg
            ob->rot[1] = 0;      // Ry
            ob->rot[2] = 0;      // Rz

            if( carHdl ) makeParent( carHdl, ob );

            if( !camTgt.IsNil() ) {
                obTgt = add_object( OB_EMPTY );
                if( NULL == obTgt ) { errLine = __LINE__; break; }
                if( gr ) add_to_group( gr, obTgt );

                obTgt->empty_drawtype = FO2_EMPTY_FLAGS;
                obTgt->empty_drawsize = FO2_EMPTY_SIZE;

                // Get real camera name...
//                 snprintf( name, sizeof(name), "Target.Camera.%03d", i );
                snprintf( name, sizeof(name), "Target.%s", ob->id.name+2 );
                rename_id( &obTgt->id, name );

                obTgt->loc[0] = camTgt[1].GetFloat(); // X
                obTgt->loc[1] = camTgt[3].GetFloat(); // Y (Y & Z are inverted)
                obTgt->loc[2] = camTgt[2].GetFloat(); // Z
                if( i <= MAX_DEF_CAM ) {
                    obTgt->loc[0] += camOffsets[i-1][0];
                    obTgt->loc[1] += camOffsets[i-1][1];
                    obTgt->loc[2] += camOffsets[i-1][2];
                }

                ob->rot[0] = 0;

                // Add constraint so that camera always
                // aims at target object
                bConstraint* co = NULL;
                bTrackToConstraint* coT = NULL;

                co = add_new_constraint(CONSTRAINT_TYPE_TRACKTO);
                if( co ) coT = (bTrackToConstraint*)co->data;

                if( NULL == co || NULL == coT ) { errLine = __LINE__; break; }

                coT->reserved1 = TRACK_nZ; // trackflag
                coT->reserved2 = UP_Y;     // upflag
                set_constraint_target( co, obTgt, NULL );
                add_constraint_to_object( co, ob );

                if( carHdl ) makeParent( carHdl, obTgt );
            }

            ca->type = CAM_PERSP;
            ca->flag = ( CAM_SHOWPASSEPARTOUT |
                         CAM_SHOWTITLESAFE | CAM_SHOWNAME );
            ca->passepartalpha = 0.75;
#if 0
            ca->lens           = ( i < MAX_DEF_CAM ? camLens[i] : 15 );
#else
            ca->lens           = 15;
#endif
            ca->drawsize       = 0.2;

            camClip = camera["NearClipping"];
            if( camClip.IsNumber() ) ca->clipsta = camClip.GetFloat();
            camClip = camera["FarClipping"];
            if( camClip.IsNumber() ) ca->clipend = camClip.GetFloat();

#ifdef WITH_FO2_DBG
            luaState->DumpObject( str, FO2_CARCAM_SETUP, camera,
                                  LuaState::DUMP_ALPHABETICAL, 0, 0 );

            printf("Inserting camera #%d at x=%f y=%f z=%f (%s)\n",
                   i, camPos[1].GetFloat(), camPos[3].GetFloat(),
                   camPos[2].GetFloat(), str.c_str() );
#endif // WITH_FO2_DBG
        }
        if( i <= nbCam ) break;

        rtn = 0;
    } while(0);

    return(0);
} // End proc cCamera::toBlender
#endif // NO_BLENDER

//-----------------------------------------------------------------------------
// Name   : cCamera::show
// Usage  : Dump cCamera to screen
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         tab              X   -   (Optional) number of tabs before printing
//         where            X   -   (Optional) where to send data
//
// Return : Nothing
//-----------------------------------------------------------------------------
void cCamera::show( const uint tab, FILE *where )
{
    const char * myTab = TABS(tab);
    FILE *out = ( where ? where : stdout );
    string strCam;

    if( luaState ) {
        LuaObject Cameras = luaState->GetGlobal( FO2_CARCAM_OBJ );
        if( !luaState->DumpObject( strCam, FO2_CARCAM_OBJ,
                                   Cameras, LuaState::DUMP_ALPHABETICAL,
                                   tab+1, 10 )) strCam = "\t<none>";
    }

    fprintf( out, "%s{ -- cameras\n", myTab );

    fprintf( out, "%s%s\n", (*myTab ? myTab+1 : myTab), strCam.c_str() );
    fprintf( out,
             "%s},\n", myTab );
} // End proc cCamera::show


//-----------------------------------------------------------------------------
// Name   : cCarMeshList::read
// Usage  : Read cCarMeshList from file
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         h                X   X   File handler
//
// Return : 0 == OK / -1 == read error / -2 memory error
//-----------------------------------------------------------------------------
int cCarMeshList::read( FILE * h )
{
    int rtn = 0;
    cCarMesh *pM;

    meshes.clear(); dwNMesh = 0;

    // TODO: CHECKME
    if( 1 != fread( &dwNMesh, sizeof(dwNMesh), 1, h )) return( -1 );

    if( dwNMesh ) {
        meshes.reserve( dwNMesh );
        if( meshes.capacity() < dwNMesh ) return( -2 );

        for( uint i = 0; 0 == rtn && i < dwNMesh; ++i ) {
            pM = new cCarMesh;
            if( pM ) {
                rtn = pM->read( h, i );
                if( 0 == rtn ) meshes.push_back( *pM );
                delete pM;
            } else {
                rtn =-2;
            }
        } // End for
    } // End if
//     m.raz(); // To avoid memory leak

    return( rtn );
} // End proc cCarMeshList::read

//-----------------------------------------------------------------------------
// Name   : cCarMeshList::write
// Usage  : Write cCarMeshList to file
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         h                X   X   File handler
//
// Return : 0 == OK / -1 == write error / -2 memory error
//-----------------------------------------------------------------------------
int cCarMeshList::write( FILE * h )
{
    int rtn = 0;

    // TODO: CHECKME
    if( 1 != fwrite( &dwNMesh, sizeof(dwNMesh), 1, h )) return( -1 );

    if( dwNMesh ) {
        if( dwNMesh != meshes.size() ) return( -2 );

        for( uint i = 0; 0 == rtn && i < dwNMesh; ++i ) {
            rtn = meshes[i].write( h );
        } // End for
    } // End if

    return( rtn );
} // End proc cCarMeshList::write

#if !defined(NO_BLENDER)
//-----------------------------------------------------------------------------
// Name   : cCarMeshList::toBlender
// Usage  : Draw in Blender
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         car              X   -   Car object meshList belongs to
//
// Return : 0 in case of success, -1 otherwise
//-----------------------------------------------------------------------------
int cCarMeshList::toBlender( cCar& car )
{
    uint meshIdx = 0;  // Mesh index
    int  rtn     = 0;

    for( meshIdx = 0; 0 == rtn && meshIdx < dwNMesh; ++meshIdx ) {
        rtn = meshes[meshIdx].toBlender( car );
    } // End for

    return( rtn );
} // End proc cCarMeshList::toBlender
#endif //NO_BLENDER

//-----------------------------------------------------------------------------
// Name   : cCarMeshList::show
// Usage  : Dump cCarMeshList to screen
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         tab              X   -   (Optional) number of tabs before printing
//         where            X   -   (Optional) where to send data
//
// Return : Nothing
//-----------------------------------------------------------------------------
//
void cCarMeshList::show( const uint tab, FILE *where )
{
    const char * myTab = TABS(tab);
    FILE *out = ( where ? where : stdout );

    fprintf( out,
             "%sMeshes = { -- NB meshes: %d\n", myTab, dwNMesh );
    for( uint i = 0; i < dwNMesh; ++i ) {
        meshes[i].show( tab + 1, where );
    } // End for
    fprintf( out, "%s}, -- End Meshes\n", myTab );
} // End proc cCarMeshList::show

//-----------------------------------------------------------------------------
// Name   : cCarMesh::cCarMesh
// Usage  : Copy constructor
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         m                X   -   Source object
//
// Return : Nothing
//-----------------------------------------------------------------------------
cCarMesh::cCarMesh( const cCarMesh& m )
{
    dwID       = m.dwID;
    sName      = ( m.sName ? strdup( m.sName ) : NULL );
    sMadeOf    = ( m.sMadeOf ? strdup( m.sMadeOf ) : NULL );
    dwFlag     = m.dwFlag;
    dwGroup    = m.dwGroup;
    matrix     = m.matrix;
    dwNmodels  = m.dwNmodels;
    dwModelIdx = m.dwModelIdx;
} // End proc cCarMesh::cCarMesh

//-----------------------------------------------------------------------------
// Name   : cCarMesh::read
// Usage  : Read cCarMesh from file
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         h                X   X   File handler
//         idx              X   -   (Optiona) surface index
//
// Return : 0 == OK / -1 == read error / -2 == id error
//-----------------------------------------------------------------------------
int cCarMesh::read( FILE * h, const uint idx )
{
    int rtn = -1;
    uint modelIdx = 0;

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
        if(1 != fread( &dwNmodels, sizeof(dwNmodels), 1, h )) break;

        rtn = 0;
        for( uint i = 0; 0 == rtn && i < dwNmodels; ++ i ) {
            if(1 == fread( &modelIdx, sizeof(modelIdx), 1, h )) {
                dwModelIdx.push_back( modelIdx );
            } else {
                rtn = -1;
            }
        }

    } while(0);

    return( rtn );
} // End proc cCarMesh::read

//-----------------------------------------------------------------------------
// Name   : cCarMesh::write
// Usage  : Write cCarMesh to file
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         h                X   X   File handler
//
// Return : 0 == OK / -1 == write error / -2 == id error
//-----------------------------------------------------------------------------
int cCarMesh::write( FILE * h )
{
    int rtn = -1;
    uint modelIdx = 0;

    do {
        if( ID_MESH != dwID ) { rtn = -2; break; }
        if( 1 != fwrite( &dwID, sizeof(dwID), 1, h ))           break;

        if( 0 != writeString( h,sName ))                        break;
        if( 0 !=writeString( h, sMadeOf ))                      break;
        if( 1 != fwrite( &dwFlag, sizeof(dwFlag), 1, h ))       break;
        if( 1 != fwrite( &dwGroup, sizeof(dwGroup), 1, h ))     break;
        if( 0 != matrix.write( h ))                             break;
        if( 1 != fwrite( &dwNmodels, sizeof(dwNmodels), 1, h )) break;

        rtn = 0;
        for( uint i = 0; 0 == rtn && i < dwNmodels; ++ i ) {
            modelIdx = dwModelIdx[i];
            if(1 != fwrite( &modelIdx, sizeof(modelIdx), 1, h )) {
                rtn = -1;
            }
        }
    } while(0);

    return( rtn );
} // End proc cCarMesh::write

#if !defined(NO_BLENDER)
//-----------------------------------------------------------------------------
// Name   : cCarMesh::toBlender
// Usage  : Draw in Blender
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         car              X   -   Car object meshList belongs to
//
// Return : 0 in case of success, -1 otherwise
//-----------------------------------------------------------------------------
int cCarMesh::toBlender( cCar& car )
{
//     int           sIdx         = 0;  // Surface index
//     cCarMeshToBBox  *pMeshToBB    = NULL;
//     cBoundingBox *pBBox        = NULL;
    cModel       *pModel       = NULL;
//     cBoundingBox *pBBoxDam     = NULL;
//     cModel       *pModelDam    = NULL;
//     Material     *ma           = NULL;
//     Group        *gr           = NULL;
//     Object       *pObj         = NULL;

    int rtn = 0;

    do {
        // -----------------------------------------------------------
        // Get object data
        // -----------------------------------------------------------
        //   "model" instance
        pModel = &car.modelList.models[ dwModelIdx[0] ];
        //   "object" instance
        _obj = car.surfaceList.surfaces[ pModel->dwSurfIdx[0] ]._obj;

#ifdef WITH_FO2_DBG
        fprintf( dbg, "=> Mesh %s\n", sName );
        fprintf( dbg, "\tModel #%d (%s)\n",
                 pModel->_index, pModel->sName );
#endif // WITH_FO2_DBG

        // -----------------------------------------------------------
        // Rotate and translate object to its position
        // -----------------------------------------------------------
        (void)moveRotateObj( _obj, &matrix, pModel->sName );

#ifdef WITH_FO2_DBG
        fprintf( dbg, "mesh #%d (%s) : Model #%d (%s) x=%f y=%f z=%f\n",
                 _index, sName,
                 dwModelIdx[0], pModel->sName,
                 matrix.m[3][0],
                 matrix.m[3][1],
                 matrix.m[3][2] );
#endif // WITH_FO2_DBG

    } while(0);

    return( rtn );
} // End proc cCarMesh::toBlender
#endif //NO_BLENDER

//-----------------------------------------------------------------------------
// Name   : cCarMesh::show
// Usage  : Dump cCarMesh to screen
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         tab              X   -   (Optional) number of tabs before printing
//         where            X   -   (Optional) where to send data
//
// Return : Nothing
//-----------------------------------------------------------------------------
void cCarMesh::show( const uint tab, FILE *where )
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
    fprintf( out, "%s\tdwNmodels = 0x%08x (%d)\n", myTab, dwNmodels,dwNmodels);
    for( uint i = 0; i < dwNmodels; ++i ) {
        fprintf( out, "%s\t\tdwModelIdx[%d] = 0x%08x (%d)\n",
                 myTab, i, dwModelIdx[i], dwModelIdx[i] );
    }

    fprintf( out,
             "%s},\n", myTab );
} // End proc cCarMesh::show

//-----------------------------------------------------------------------------
// Name   : cCarGroup::cCarGroup
// Usage  : CarGroup class constructor
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         fileType         X   -   Type of file being processed
//
// Return : 0 == OK / -1 == read error
//-----------------------------------------------------------------------------
cCarGroup::cCarGroup( const FO2_FileType_t fileType )
{
#if !defined(NO_BLENDER)
    car     = NULL;
    windows = NULL;
    lights  = NULL;
    shadow  = NULL;
    damage  = NULL;
    holders = NULL;
    cameras = NULL;

    snprintf( groupName, sizeof(groupName), FO2_CARHANDLER_NAME );
    if( NULL == ( car = findGroup( groupName )) &&
        FO2_CAR == fileType ) {
        car = add_group();
        rename_id( &car->id, groupName );
    }

    if( FO2_CAR == fileType ) {
        snprintf( groupName, sizeof(groupName),
                    FO2_CARHANDLER_NAME "_windows" );
        if( NULL == ( windows = findGroup( groupName ))) {
            windows = add_group();
            rename_id( &windows->id, groupName );
        }

        snprintf( groupName, sizeof(groupName),
                    FO2_CARHANDLER_NAME "_lights" );
        if( NULL == ( lights = findGroup( groupName ))) {
            lights = add_group();
            rename_id( &lights->id, groupName );
        }

        snprintf( groupName, sizeof(groupName),
                    FO2_CARHANDLER_NAME "_damages" );
        if( NULL == ( damage = findGroup( groupName ))) {
            damage = add_group();
            rename_id( &damage->id, groupName );
        }

        snprintf( groupName, sizeof(groupName),
                    FO2_CARHANDLER_NAME "_placeholders" );
        if( NULL == ( holders = findGroup( groupName ))) {
            holders = add_group();
            rename_id( &holders->id, groupName );
        }
    }

    if( FO2_CAR  == fileType ||
        FO2_TIRE == fileType ) {
        snprintf( groupName, sizeof(groupName), "%s_shadows",
                    ( FO2_CAR == fileType ?
                    FO2_CARHANDLER_NAME : FO2_TIREHANDLER_NAME ));
        if( NULL == ( shadow = findGroup( groupName ))) {
            shadow = add_group();
            rename_id( &shadow->id, groupName );
        }
    }

    if( FO2_CAR    == fileType ||
        FO2_CAMERA == fileType ) {
        snprintf( groupName, sizeof(groupName),
                    FO2_CARHANDLER_NAME "_cameras" );
        if( NULL == ( cameras = findGroup( groupName ))) {
            cameras = add_group();
            rename_id( &cameras->id, groupName );
        }
    }
#endif //NO_BLENDER
} // End proc cCarGroup::cCarGroup

//-----------------------------------------------------------------------------
// Name   : cCarHeader::read
// Usage  : Read cCarHeader from file
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         h                X   X   File handler
//
// Return : 0 == OK / -1 == read error
//-----------------------------------------------------------------------------
int cCarHeader::read( FILE *h )
{
    if( 1 != fread( &dwType,    sizeof(dwType), 1, h ))    return( -1 );

    return( 0 );
} // End proc cCarHeader::read

//-----------------------------------------------------------------------------
// Name   : cCarHeader::write
// Usage  : Write cCarHeader to file
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         h                X   X   File handler
//
// Return : 0 == OK / -1 == write error
//-----------------------------------------------------------------------------
int cCarHeader::write( FILE *h )
{
    if( 1 != fwrite( &dwType,    sizeof(dwType), 1, h ))    return( -1 );

    return( 0 );
} // End proc cCarHeader::write

//-----------------------------------------------------------------------------
// Name   : cCarHeader::show
// Usage  : Dump cCarHeader to screen
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         tab              X   -   (Optional) number of tabs before printing
//         where            X   -   (Optional) where to send data
//
// Return : Nothing
//-----------------------------------------------------------------------------
void cCarHeader::show( const uint tab, FILE *where )
{
    const char * myTab = TABS(tab);
    FILE *out = ( where ? where : stdout );

    fprintf( out, "%sFileHeader = {\n",     myTab );
    fprintf( out, "%s\tdwType    = 0x%08x\n", myTab, dwType );
    fprintf( out, "%s}\n",                  myTab );
} // End proc cCarHeader::show

//-----------------------------------------------------------------------------
// Name   : cCar::read
// Usage  : Read cCar from file
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         fileName         X   -   File to read
//         printOnLoad      X   -   (OPtional) Print data blocks while reading
//         name             X   -   (Optional) Track name
//         logFile          X   -   (Optional) log file name
//         fileType         X   -   (Optional) Type of file
//
// Return : 0 == OK / -1 == read error / -2 == file access error
//                    -3 == read data does not match file length
//-----------------------------------------------------------------------------
int cCar::read( const char * const fileName,
                const int printOnLoad,
                const char * const name,
                const char * const logFile,
                const FO2_FileType_t fileType )
{
    FILE *h = NULL;
    int rtn = 0;
    FILE *out = NULL;
    uint fileLength = 0;

    // For file identification...
    uint id;
    uint firstMat;

    errLine = 0;
    do {
        if( name ) {
            if( carName ) free( carName );
            carName = strdup( name );
        }

        if( logFile ) {
            if( _logFile ) free( _logFile );
            _logFile = strdup( logFile );
        }

        if( printOnLoad && _logFile ) {
            out = fopen( _logFile, "w");
//             if( out ) printf( "Log file: %s\n", _logFile );
        }

        if( printOnLoad ) {
            out = (out ? out : stdout);
            fprintf( out, "{ -- Car '%s'\n", carName );
        }

        if( FO2_CAMERA != fileType  ) {
            if( NULL == ( h = fopen( fileName, "rb" ))) {
                errLine = __LINE__;
                rtn = -2;
                break;
            }

            fseek( h, 0, SEEK_END );
            fileLength = ftell( h );
            fseek( h, 0, SEEK_SET );

            // Check this is a FlatOut2 car...
            if( 1 != fread( &id,       sizeof(id),       1, h ) ||
                1 != fread( &firstMat, sizeof(firstMat), 1, h ) ||
                1 != fread( &firstMat, sizeof(firstMat), 1, h ) ||
                FO2_CAR_FILE_ID      != id ||
                ID_MATERIAL          != firstMat ) {
                rtn = -1;
                break;
            }
            fseek( h, 0, SEEK_SET );

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

            // Read models
            rtn = modelList.read( h );
            _filePos = ftell( h );
            if( printOnLoad ) modelList.show( 1, out );
            if( rtn ) break;

            // Read meshes
            rtn = meshList.read( h );
            _filePos = ftell( h );
            if( printOnLoad ) meshList.show( 1, out );
            if( rtn ) break;

            // Read objects
            rtn = objectList.read( h );
            _filePos = ftell( h );
            if( printOnLoad ) objectList.show( 1, out );
            if( rtn ) break;
        } // End if (!CAMERA)

        // Read cameras
        if( FO2_CAR == fileType || FO2_CAMERA == fileType ) {
            char camFile[FO2_PATH_LEN];
            char *pSep = NULL;
            snprintf( camFile, sizeof(camFile), "%s", fileName );
            if( FO2_CAR == fileType ) {
                pSep = strrchr( camFile, DIR_SEP );
                if( pSep ) *(++pSep) = 0;
                snprintf( pSep, sizeof(camFile)-strlen(camFile),
                          "%s", FO2_CARCAM );
            }

            rtn = cameras.read( camFile );
            if( printOnLoad ) cameras.show( 1, out );
            if( rtn ) break;
        }

    } while(0);

    if( FO2_CAMERA != fileType &&
        0 == rtn &&
        printOnLoad ) fprintf( (out ? out : stdout),
                               "} -- FilePos = 0x%08x (FileLen = 0x%08x)\n",
                               _filePos, fileLength );
    if( h ) fclose( h );
    if( out && out != stdout ) fclose( out );

    if( 0 == rtn &&  _filePos != fileLength ) { errLine = __LINE__; rtn = -3; }

    return( rtn );
} // End proc cCar::read

//-----------------------------------------------------------------------------
// Name   : cCar::write
// Usage  : Write cCar to file
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         fileName         X   -   File to read
//         printOnWrite     X   -   (OPtional) Print data blocks while writing
//
// Return : 0 == OK / -1 == write error / -2 == file access error
//-----------------------------------------------------------------------------
int cCar::write( const char * const fileName,
                   const int printOnWrite )
{
    FILE *h = NULL;
    int rtn = 0;
    FILE *out = NULL;

    errLine = 0;
    do {
        if( NULL == ( h = fopen( fileName, "wb" ))) {
            errLine = __LINE__;
            rtn = -2;
            break;
        }

        if( printOnWrite && _logFile ) {
            char *c;
            c = strstr( _logFile, FO2_READ_EXT FO2_LOG_EXT ); //TODO: CHANGEME?
            if( c ) sprintf( c, "%s%s", FO2_WRITE_EXT, FO2_LOG_EXT );
            out = fopen( _logFile, "w");
//             if( out ) printf( "Log file: %s\n", _logFile );
        }

        if( printOnWrite ) {
            out = (out ? out : stdout);
            fprintf( out, "{ -- Car '%s'\n", carName );
        }

        // Write header
        rtn = header.write( h );
        if( printOnWrite ) header.show( 1, out );
        if( rtn ) { errLine = __LINE__; break; }

        // Write materials
        rtn = materialList.write( h );
        _filePos = ftell( h );
        if( printOnWrite ) materialList.show( 1, out );
        if( rtn ) { errLine = __LINE__; break; }

        // Write streams
        rtn = streamList.write( h );
        _filePos = ftell( h );
        if( printOnWrite ) streamList.show( 1, out );
        if( rtn ) { errLine = __LINE__; break; }

        // Write surfaces
        rtn = surfaceList.write( h );
        _filePos = ftell( h );
        if( printOnWrite ) surfaceList.show( 1, out );
        if( rtn ) { errLine = __LINE__; break; }

        // Write models
        rtn = modelList.write( h );
        _filePos = ftell( h );
        if( printOnWrite ) modelList.show( 1, out );
        if( rtn ) { errLine = __LINE__; break; }

        // Write meshes
        rtn = meshList.write( h );
        _filePos = ftell( h );
        if( printOnWrite ) meshList.show( 1, out );
        if( rtn ) { errLine = __LINE__; break; }

        // Write objects
        rtn = objectList.write( h );
        _filePos = ftell( h );
        if( printOnWrite ) objectList.show( 1, out );
        if( rtn ) { errLine = __LINE__; break; }

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
} // End proc cCar::write

//-----------------------------------------------------------------------------
// Name   : cCar::show
// Usage  : Dump cCar to screen
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         tab              X   -   (Optional) number of tabs before printing
//         where            X   -   (Optional) where to send data
//
// Return : Nothing
//-----------------------------------------------------------------------------
void cCar::show( const uint tab, FILE *where )
{
    FILE *out = ( where ? where : stdout );

    fprintf( out, "{ -- Car '%s'\n", carName );

    header.show( 1, where );
    materialList.show( 1, where );

#if 0
    streamList.show( 1, where );
    surfaceList.show( 1, where );
    unknown1List.show( 1, where );
    unknown2List.show( 1, where );
    unknown3List.show( 1, where );
    unknown4List.show( 1, where );
    unknown5List.show( 1, where );
    modelList.show( 1, where );
    objectList.show( 1, where );
    boundingBoxList.show( 1, where );
    meshToBBoxList.show( 1, where );
    meshList.show( 1, where );
#endif

    fprintf( out, "} -- FilePos = 0x%08x\n", _filePos );
} // End proc cCar::show

#if !defined(NO_BLENDER)
//-----------------------------------------------------------------------------
// Name   : fo2_read_car
// Usage  : Read FlatOut2 car file into Blender
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         name             X   -   Car name
//         fileType         X   -   Type of file
//
// Return : 0 in case of success, -1 otherwise
//-----------------------------------------------------------------------------
static int fo2_read_car(char *name, const FO2_FileType_t fileType )
{
    cCar   car;
    FO2_Path_t FO2path;
    uint *layers  = NULL;
    time_t duration;
    int rtn = 0;
	int wng = 0;

    waitcursor(1);

    printf( "--- Start importing car '%s'\n", name );
    duration = time(NULL);

    do {
        if( fileType >= FO2_END_OF_LIST ) {
            errLine = __LINE__;
            rtn = -1;
            break;
        }

        // ---------------------------------------------------------------
        // ---   Get car paths
        // ---------------------------------------------------------------
        rtn = buildPath( name, &FO2path, true, fileType );
        if( 0 != rtn ) break;

        car._logFile = strdup( FO2path.logFile );

#ifdef WITH_FO2_DBG
        dbg = fopen( FO2path.dbgFile, "w" );
        if( NULL == dbg ) dbg = stdout;

        fprintf( dbg, "--- Start importing car '%s'\n", name );
        fprintf( dbg, "Car file    : <%s>\n", name );
        fprintf( dbg, "Car name    : <%s>\n", FO2path.trackName );
        fprintf( dbg, "Base dir    : <%s>\n", FO2path.baseDir );

        fprintf( dbg, "Track dir   : <%s>\n", FO2path.geomDir );
        fprintf( dbg, "Light dir   : <%s>\n", FO2path.lightDir );
        fprintf( dbg, "Textures dir: <%s>\n", FO2path.texDir );
        fprintf( dbg, "Dynamics dir: <%s>\n", FO2path.dynDir );
        fprintf( dbg, "Log file    : <%s>\n", FO2path.logFile );
        fprintf( dbg, "Dbg file    : <%s>\n", FO2path.dbgFile );
#endif // WITH_FO2_DBG

        // ---------------------------------------------------------------
        // ---   Read car data
        // ---------------------------------------------------------------
#ifdef WITH_FO2_DBG
        rtn = car.read(name, 1, FO2path.trackName, FO2path.logFile, fileType);
#else
        rtn = car.read(name, 0, FO2path.trackName, FO2path.logFile, fileType);
#endif // WITH_FO2_DBG
        BREAK_ON_ERROR(rtn,wng);

        // ---------------------------------------------------------------
        // ---   Create dummy object to handle the car as a whole
        // ---------------------------------------------------------------
        Object* carHandler = findObject( FO2_CARHANDLER_NAME );
        if( NULL == carHandler &&
            FO2_CAR == fileType ) {
            carHandler = add_object( OB_EMPTY );
            if( NULL == carHandler ) { errLine = __LINE__; break; }
            rename_id( &carHandler->id, FO2_CARHANDLER_NAME );
        }

        // ---------------------------------------------------------------
        // ---   Process materials & texture images
        // ---------------------------------------------------------------
        rtn = car.materialList.toBlender( FO2path.lightDir, FO2path.texDir, true );
        BREAK_ON_ERROR(rtn,wng);

        // ---------------------------------------------------------------
        // ---   Initialize groups
        // ---------------------------------------------------------------
        cCarGroup grp( fileType );

        // ---------------------------------------------------------------
        // ---   Load vertices & surfaces
        // ---------------------------------------------------------------
        if( FO2_CAMERA != fileType ) {
            uint      surfIdx    = 0;
            cSurface *pSurface   = NULL;
            cIndexStream  *iS    = 0;  // Index stream
            cVertexStream *vS    = 0;  // Vertex stream
            uint      vO         = 0;  // Index of vertex in stream
            MVert  *vertdata     = NULL;  // Vertex array
            MFace  *facedata     = NULL;  // Face array
            MTFace *tfacedata[2] = { NULL, NULL };  // Texture array

            for( surfIdx = 0;
                 surfIdx < car.surfaceList.dwNSurface;
                 ++surfIdx) {
//             for(surfIdx = 1; surfIdx < 2; ++surfIdx) {
                pSurface = &car.surfaceList.surfaces[surfIdx];
                if( NULL == pSurface ) { errLine = __LINE__; rtn = -1; break; }

#ifdef WITH_FO2_DBG
                fprintf( dbg, "Processing surface #%d\n", surfIdx );
                fflush(dbg);
#endif // WITH_FO2_DBG

                iS = (cIndexStream*)car.streamList.streams[ pSurface->dwIStreamIdx ];
                vS = (cVertexStream*)car.streamList.streams[ pSurface->dwVStreamIdx ];
                if( NULL == iS || NULL == vS ) { errLine = __LINE__; rtn = -1; break; }

//                 fprintf( dbg, "iS->dwType=%d  vS->dwType=%d\n",
//                          iS->dwType, vS->dwType );

                if( STREAM_TYPE_INDEX == iS->dwType &&
                    STREAM_TYPE_INDEX != vS->dwType ) {

//                     pSurface->show( 0, dbg );
//                     vS->show(0, 0, dbg );
//                     fprintf( dbg, "pSurface->dwVStreamOffset=%d\n",
//                              pSurface->dwVStreamOffset);

                    // Find first vertex in vertex list
                    for( vO = 0; vO < vS->dwNum; ++vO ) {
                        if( vS->vertices[vO]._offsetStream == pSurface->dwVStreamOffset )
                            break;
                    } // End for
                    if(vO == vS->dwNum) {errLine = __LINE__; rtn = -1; break;}

#ifdef WITH_FO2_DBG
                    fprintf( dbg,
                             "\tindex  stream 0x%08x\n"
                             "\tvertex stream 0x%08x\n"
                             "\tvertex index  %d\n",
                             iS, vS, vO );
                    fflush(dbg);
#endif // WITH_FO2_DBG

                    // TODO: shorten vertdata and facedata arrays
                    Object *ob;
                    Mesh   *me;

                    ob = add_object( OB_MESH );
                    add_to_group( grp.car, ob );

                    me = get_mesh(ob); //(Mesh*)ob->data;
                    me->totvert = pSurface->dwVertNum;
                    me->mvert   = (MVert*)CustomData_add_layer(&me->vdata,
                                                               CD_MVERT,
                                                               CD_DEFAULT,
                                                               NULL,
                                                               me->totvert);
                    if( NULL == me->mvert ) {
                        errLine = __LINE__;
                        rtn = -1;
                        break;
                    }

                    me->totface = pSurface->dwPolyNum; // faceIdx; TODO: BLENDER243 CHECK
                    me->mface   = (MFace*)CustomData_add_layer(&me->fdata,
                                                               CD_MFACE,
                                                               CD_DEFAULT,
                                                               NULL,
                                                               me->totface);
                    if( NULL == me->mface ) {
                        errLine = __LINE__;
                        rtn = -1;
                        break;
                    }

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
//                 me->mtface = ( tfacedata[0] ? tfacedata[0] : tfacedata[1] );

                    CustomData_set_layer_active(&me->fdata, CD_MTFACE, 0);
                    mesh_update_customdata_pointers(me);

#ifdef WITH_FO2_DBG
                    fprintf( dbg,
                             "\tobject       0x%08x\n"
                             "\tmesh         0x%08x\n"
                             "\tvertdata     0x%08x\n"
                             "\tfacedata     0x%08x\n"
                             "\ttfacedata[0] 0x%08x\n"
                             "\ttfacedata[1] 0x%08x\n",
                             ob, me, vertdata, facedata,
                             tfacedata[0], tfacedata[1] );
                    fflush(dbg);
#endif // WITH_FO2_DBG

#ifdef WITH_FO2_DBG
                    // REMOVEME
                    fprintf( dbg, "\t-> Add vertices\n" );
                    fflush(dbg);
#endif // WITH_FO2_DBG

                    // -------------------------------------------------------
                    // -- Add vertices
                    // -------------------------------------------------------
                    for( uint i = 0; i < pSurface->dwVertNum; ++i ) {
                        cVertex *pVertex = &vS->vertices[ vO + i ];

                        // Vertex coord
                        vertdata[i].co[0] = pVertex->pos.x;
//                      vertdata[i].co[1] = pVertex->pos.z;// y && z inverted!!
//                      vertdata[i].co[2] = pVertex->pos.y;
                        vertdata[i].co[1] = pVertex->pos.y;
                        vertdata[i].co[2] = pVertex->pos.z;

                        // Vertex normal
                        vertdata[i].no[0] = (short)(32767 * pVertex->normal.x);
                        // y && z inverted!!
                        vertdata[i].no[1] = (short)(32767 * pVertex->normal.z);
                        vertdata[i].no[2] = (short)(32767 * pVertex->normal.y);

//                         fprintf( dbg, "DBG[%d]: \tx=%f y=%f z=%f\n",
//                                  i,pVertex->pos.x,pVertex->pos.y,pVertex->pos.z);
                    } // End for (i)

#ifdef WITH_FO2_DBG
                    // REMOVEME
                    fprintf( dbg, "\t-< Add vertices\n" );
                    fflush(dbg);
#endif // WITH_FO2_DBG

                    // -------------------------------------------------------
                    // -- Find textures
                    // -------------------------------------------------------
                    cMaterial *pMaterial = NULL;
                    Image     *pImg[2]   = { NULL, NULL };
                    uint       matIdx    = 0;
                    uint       tIdx      = 0;  // Texture index

                    const char *pTexName    = NULL;
                    uint alphaMode    = 0;
                    uint tileMode     = 0;
                    pImg[0] = pImg[1] = NULL;
//                     uint m = 0;

                    for( tIdx = 0; tIdx < 2; ++tIdx ) {
                        if( tfacedata[tIdx] ) {
                            pTexName = car.materialList.materials[ pSurface->dwMatIdx ].sTexture[tIdx].c_str();
#ifdef WITH_FO2_DBG
                            // REMOVEME
                            fprintf( dbg, "\t-> Add texture (%s)"
                                     " materialIdx=%d texture#%d\n",
                                     pTexName, pSurface->dwMatIdx, tIdx );
                            fflush(dbg);
#endif // WITH_FO2_DBG

                            if( pTexName && *pTexName ) {
                                if( 0 == strcasecmp( pTexName, TEX_WINDOWS ) ||
                                    strstr( pTexName, TEX_WINDOWS2 )) {
                                    add_to_group( grp.windows, ob );
                                    rem_from_group( grp.car, ob);
                                } else if( 0==strcasecmp( pTexName, TEX_LIGHTS ) ||
                                           strstr( pTexName, TEX_LIGHTS2 )) {
                                    add_to_group( grp.lights, ob );
                                    rem_from_group( grp.car, ob);
                                }

                                for( matIdx = 0;
                                     matIdx < car.materialList.dwNmaterials;
                                     ++matIdx) {
                                    pMaterial = &car.materialList.materials[matIdx];

                                    if( 0 == strcasecmp(pTexName, TEX_COLORMAP)) {
                                        pTexName = TEX_LIGHTMAP;
                                    }

                                    if(pMaterial->_img[tIdx] &&
                                       0==strcasecmp(pTexName,
                                                     pMaterial->_img[tIdx]->id.name+2)) {
                                        pImg[tIdx] = pMaterial->_img[tIdx];
                                        alphaMode  = pMaterial->dwAlpha;
                                        //tileMode   = ( MATERIAL_TILE ==
                                        //               pMaterial->dwMode ? 1 : 0 );
										tileMode   = 0;
                                        break;
                                    }
                                } // End for
#ifdef WITH_FO2_DBG
                                // REMOVEME
                                fprintf( dbg, "\t-< Add texture (0x%08X)\n", pMaterial );
                                fflush(dbg);
#endif // WITH_FO2_DBG
                            } // End if

//                         printf( "Surface #%d: Image %s %sfound!!\n",
//                                 surfIdx, pTexName[0], (pImg ? "" : "NOT " ));

                        } // End if (tfacedata[tIdx])
                    } // End for (tIdx)

                    // -------------------------------------------------------
                    // -- Add polygons
                    // -------------------------------------------------------
                    uint n = pSurface->dwIStreamOffset / sizeof(ushort);
                    uint s[3];
                    uint faceIdx = 0;
                    uint keepMe = 0;

#ifdef WITH_FO2_DBG
                    // REMOVEME
                    fprintf( dbg, "\t-> Add polygons\n" );
                    fflush(dbg);
#endif // WITH_FO2_DBG

                    for( uint i = 0; i < pSurface->dwPolyNum; ++i ) {
                        s[0] = iS->indexes[n]   - vO;
                        s[1] = iS->indexes[n+1] - vO;
                        s[2] = iS->indexes[n+2] - vO;

                        if( 4 == pSurface->dwPolyMode ) {
                            n += 3;
                        } else {
                            n += 1;
                        }
                        if( s[0] != s[1] && s[1] != s[2] && s[2] != s[0] ) {
                            if( 5 == pSurface->dwPolyMode && 0 == (n % 2)) {

                                uint tmp = s[0];
                                s[0] = s[2];
                                s[2] = tmp;

                                if( 0 == i ) keepMe = 1;
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
//                             facedata[faceIdx].edcode = ME_V1V2|ME_V2V3;
//                             fprintf( dbg, "DBG[%d]: \ts0=%d s1=%d s2=%d\n",
//                                      faceIdx,s[0],s[1],s[2]);

                            ++faceIdx;
                        } // End if

                    } // End for (i)

#ifdef WITH_FO2_DBG
                    // REMOVEME
                    fprintf( dbg, "\t-< Add polygons\n" );
                    fflush(dbg);
#endif // WITH_FO2_DBG

                    // Flip surface?
                    if( !keepMe ) {
                        for( uint i = 0; i < faceIdx; ++i ) {
                            uint  tmpI;

                            tmpI = facedata[i].v1;
                            facedata[i].v1 = facedata[i].v2;
                            facedata[i].v2 = tmpI;
                        }
                    } // End if (keepMe)

//                 // TODO: shorten vertdata and facedata arrays
//                 Object *ob;
//                 Mesh   *me;
//                 ob = add_object( OB_MESH );
//                 me = (Mesh*)ob->data;
//                 me->mvert   = vertdata;
//                 me->totvert = pSurface->dwVertNum;
//                 me->mface   = facedata;
//                 me->totface = faceIdx; //pSurface->dwPolyNum;
//                 me->tface   = tfacedata;
// // TODO??                    if( tfacedata ) me->flags |= ME_UVEFFECT;

#ifdef WITH_FO2_DBG
                    // REMOVEME
                    fprintf( dbg, "\t-> make edges\n" );
                    fflush(dbg);
#endif // WITH_FO2_DBG

                    make_edges(me, 0);

#ifdef WITH_FO2_DBG
                    // REMOVEME
                    fprintf( dbg, "\t-< make edges\n" );
                    fflush(dbg);
#endif // WITH_FO2_DBG

                    // Change object & mesh name
                    char objName[ID_NAME_LG];
                    snprintf( objName, sizeof(objName),
                              "surface_%d", surfIdx );
                    rename_id( &ob->id, objName );
                    rename_id( &me->id, objName );

                    // Save pointer to Blender object
                    pSurface->_obj = ob;

                    // --------------------------------------------------------
                    // ---   Move object to layer
                    // --------------------------------------------------------
                    if( layers ) sendToLayer( ob,
                                              layers[pSurface->dwVStreamIdx] );

                    // --------------------------------------------------------
                    // -- Add uv coords for textured faces
                    // --------------------------------------------------------
                    for( tIdx = 0; tIdx < 2; ++tIdx ) {
                        if( tfacedata[tIdx] ) {
                            cVertex *pVertex = NULL;

#ifdef WITH_FO2_DBG
                            // REMOVEME
                            fprintf( dbg, "\t-> UV mapping\n" );
                            fflush(dbg);
#endif // WITH_FO2_DBG

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

                                pMTFace->tpage  = pImg[tIdx];
                                pMTFace->mode   = ( TF_TEX | TF_DYNAMIC );
                                //pMTFace->flag   = 0;
                                pMTFace->transp = (alphaMode ? TF_ALPHA:TF_SOLID);
                                if( tileMode ) pMTFace->mode |= TF_TILES;
                            } // End for(j)

#ifdef WITH_FO2_DBG
                            // REMOVEME
                            fprintf( dbg, "\t-< UV mapping\n" );
                            fflush(dbg);
#endif // WITH_FO2_DBG

                        } // End if
                    } // End for (tIdx)
#ifdef WITH_FO2_DBG
                    fprintf( dbg,
                             "totFace = %d  faceIdx = %d layer = 0x%08X\n",
                             me->totface, faceIdx, ob->lay );
#endif // WITH_FO2_DBG

                } // End if

            } // End for (surfIdx)

            // ---------------------------------------------------------------
            // ---   Name models
            // ---------------------------------------------------------------
            uint modIdx = 0;
            cSurface *pSurf = NULL;
            cModel   *pMod  = NULL;
            uint      sIdx  = 0;  // Surface index

            for( modIdx = 0;
                 0 == rtn && modIdx < car.modelList.dwNModel;
                 ++modIdx ) {

                pMod  = &car.modelList.models[modIdx];
                if( pMod->dwNSurfIdx ) {

                    sIdx = 0;
                    pSurf = &car.surfaceList.surfaces[pMod->dwSurfIdx[sIdx]];

                    if( pSurf->_obj ) {
                        char tmpName[ID_NAME_LG];

                        snprintf( tmpName, sizeof(tmpName),
                                  "_%s", pMod->sName );
                        rename_id( &pSurf->_obj->id, tmpName );

                        if( strstr( pMod->sName, TEX_SHADOW )) {
                            add_to_group( grp.shadow, pSurf->_obj );
                            rem_from_group( grp.car, pSurf->_obj );
                            // Make it invisible
                            pSurf->_obj->restrictflag |= OB_RESTRICT_VIEW;
                        }

#ifdef WITH_FO2_DBG
                        fprintf( dbg,
                                 "Renaming surface #%d (model #%d) to '%s'\n",
                                 pSurf->_index, modIdx, tmpName );
#endif // WITH_FO2_DBG
                    } else {
                        printf( "!! Surface #%d (model #%d)"
                                " has no object !!\n",
                                pSurf->_index, modIdx );
#ifdef WITH_FO2_DBG
                        fprintf( dbg, "!! Surface #%d (model #%d)"
                                 " has no object !!\n",
                                 pSurf->_index, modIdx );
#endif // WITH_FO2_DBG
                    } // End else
                } // End if

                for( sIdx = 1; 0 == rtn && sIdx < pMod->dwNSurfIdx; ++sIdx ) {
                    // Model is made of several surfaces;
                    // let's link them (parent/child link)
                    Object *child;
                    child = car.surfaceList.surfaces[pMod->dwSurfIdx[sIdx]]._obj;

                    rtn = makeParent( pSurf->_obj, child );
                } // End for

                if( carHandler && NULL == pSurf->_obj->parent ) {
                    makeParent( carHandler, pSurf->_obj );
                }

            } // End for
            if( 0 != rtn ) break;

            // ---------------------------------------------------------------
            // ---   Load "dynamic objects" (Meshes)
            // ---------------------------------------------------------------
            rtn = car.meshList.toBlender( car );
            if( 0 != rtn ) break;
            if( carHandler ) {
                for( uint i = 0; i < car.meshList.dwNMesh; ++i ) {
                    if( NULL == car.meshList.meshes[i]._obj->parent ) {
                        makeParent( carHandler, car.meshList.meshes[i]._obj );
                    }
                } // End for
            } // End if

            // ---------------------------------------------------------------
            // ---   Load "objects"
            // ---------------------------------------------------------------
            //GZ: TODO
			//rtn = car.objectList.toBlender( grp.holders );
            if( 0 != rtn ) break;
            if( carHandler ) {
                for( uint i = 0; i < car.objectList.dwNObject; ++i ) {
                    if( NULL == car.objectList.objects[i]._obj->parent ) {
                        makeParent( carHandler, car.objectList.objects[i]._obj );
                    }
                } // End for
            } // End if
        } // End if (CAMERA)

        // ---------------------------------------------------------------
        // ---   Load cameras
        // ---------------------------------------------------------------
        if( FO2_CAR    == fileType ||
            FO2_CAMERA == fileType ) {
            rtn = car.cameras.toBlender( grp.cameras, carHandler );
            if( 0 != rtn ) break;
        }

        // ---------------------------------------------------------------
        // ---   Attach tires
        // ---------------------------------------------------------------
        if( FO2_TIRE == fileType ) {
            Object* tire        = NULL;
            Object* placeholder = NULL;
            Object* shadow      = NULL;
            char buffer[ID_NAME_LG];
            char* tireType[4] = { "fl", "fr", "rl", "rr" };

            for( uint i = 0; i < 4; ++i ) {
                snprintf( buffer, sizeof(buffer),
                          "placeholder_tire_%s", tireType[i] );
                tire = findObject( buffer + 12 );
                placeholder = findObject( buffer );

                snprintf( buffer, sizeof(buffer),
                          "tire_%s_shadow", tireType[i] );
                shadow = findObject( buffer );

                if( tire ) {
                    if( shadow ) {
                        makeParent( tire, shadow );
                    }

                    if( placeholder ) {
                        tire->loc[0] = placeholder->loc[0];
                        tire->loc[1] = placeholder->loc[1];
                        tire->loc[2] = placeholder->loc[2];
                    }
                }
            } // End for
        }

    } while(0);

    if( layers ) free( layers );

    allqueue(REDRAWVIEW3D, 0);
    allqueue(REDRAWOOPS, 0);

    DAG_scene_sort(G.scene);
    DAG_scene_flush_update(G.scene, screen_view3d_layers());

    waitcursor(0);

    duration = time(NULL) - duration;
    printf( "--- End importing car '%s' (%ld'%02ld\") - %s\n",
            name, duration/60, duration%60, (0==rtn?"OK":"ERROR"));
#ifdef WITH_FO2_DBG
    fprintf( dbg, "--- End importing car '%s' (%ld'%02ld\") - %s\n",
             name, duration/60, duration%60, (0==rtn?"OK":"ERROR"));

    fprintf( dbg, "fo2_read_car() = %d (errLine = %d)\n", rtn, errLine );
    if( dbg != stdout ) fclose(dbg);

    if( 0 != rtn ) {
        printf( "Error in fo2_read_car(), rtn=%d, line=%d\n", rtn, errLine );
        fflush( stdout );
    }
#endif // WITH_FO2_DBG

    return( rtn );
} // End proc fo2_read_car

//-----------------------------------------------------------------------------
// Name   : fo2_write_car
// Usage  : Write FlatOut2 car file from Blender
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         name             X   -   Car name
//
// Return : 0 in case of success, -1 otherwise
//-----------------------------------------------------------------------------
int fo2_write_car(char *name)
{
    cCar   car;
    FO2_Path_t FO2path;
    time_t duration;
    int rtn = 0;

    waitcursor(1);

    printf( "--- Start exporting car '%s'\n", name );
    duration = time(NULL);

    do {
        // ---------------------------------------------------------------
        // ---   Get track paths
        // ---------------------------------------------------------------
        rtn = buildPath( name, &FO2path, false, FO2_CAR );
        if( 0 != rtn ) break;

    } while(0);

    waitcursor(0);

    duration = time(NULL) - duration;
    printf( "--- End exporting car '%s' (%ld'%02ld\") - %s\n",
            name, duration/60, duration%60, (0==rtn?"OK":"ERROR"));

#ifdef WITH_FO2_DBG
    fprintf( dbg, "--- End exporting car '%s' (%ld'%02ld\") - %s\n",
             name, duration/60, duration%60, (0==rtn?"OK":"ERROR"));
    if( dbg && dbg != stdout ) fclose(dbg);
#endif // WITH_FO2_DBG

    return( rtn );
} // End proc fo2_write_car

//-----------------------------------------------------------------------------
// Name   : FO2_read_cameras
// Usage  : Read FlatOut2 car camera file into Blender (from Blender GUI)
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         name             X   -   Camera file name
//
// Return : Nothing
//-----------------------------------------------------------------------------
void FO2_read_cameras( char *name )
{
    int rtn = fo2_read_car( name, FO2_CAMERA );

    if( 0 != rtn ) {
        error("Reading '%s', check console for details", name );
    }
} // End FO2_read_cameras

//-----------------------------------------------------------------------------
// Name   : FO2_write_cameras
// Usage  : Write FlatOut2 car camera file out of Blender (from Blender GUI)
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         name             X   -   Camera file name
//
// Return : Nothing
//-----------------------------------------------------------------------------
void FO2_write_cameras( char *name )
{
    int rtn = 0; //fo2_write_car( name, FO2_CAMERA );

    if( 0 != rtn ) {
        error("Writing '%s', check console for details", name );
    }
} // End FO2_write_cameras

//-----------------------------------------------------------------------------
// Name   : FO2_read_car
// Usage  : Read FlatOut2 car into Blender (from Blender GUI)
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         name             X   -   Car file name
//
// Return : Nothing
//-----------------------------------------------------------------------------
void FO2_read_car( char *name )
{
    int rtn = fo2_read_car( name, FO2_CAR );

    if( 0 != rtn ) {
        error("Reading '%s', check console for details", name );
    }
} // End proc FO2_read_car

//-----------------------------------------------------------------------------
// Name   : FO2_read_menucar
// Usage  : Read FlatOut2 menu car into Blender (from Blender GUI)
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         name             X   -   Menu car file name
//
// Return : Nothing
//-----------------------------------------------------------------------------
void FO2_read_menucar( char *name )
{
    int rtn = fo2_read_car( name, FO2_MENUCAR );

    if( 0 != rtn ) {
        error("Reading '%s', check console for details", name );
    }
} // End proc FO2_read_menucar

//-----------------------------------------------------------------------------
// Name   : FO2_read_tires
// Usage  : Read FlatOut2 tires into Blender (from Blender GUI)
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         name             X   -   Tires file name
//
// Return : Nothing
//-----------------------------------------------------------------------------
void FO2_read_tires( char *name )
{
    int rtn = fo2_read_car( name, FO2_TIRE );

    if( 0 != rtn ) {
        error("Reading '%s', check console for details", name );
    }
} // End proc FO2_read_tires

//-----------------------------------------------------------------------------
// Name   : FO2_read_ragdoll
// Usage  : Read FlatOut2 ragdoll into Blender (from Blender GUI)
// Args   :--------------------------------------------------------------------
//              NAME      |IN.|OUT|          DESCRIPTION
//         --------------------------------------------------------------------
//         name             X   -   Ragdoll file name
//
// Return : Nothing
//-----------------------------------------------------------------------------
void FO2_read_ragdoll( char *name )
{
    int rtn = fo2_read_car( name, FO2_RAGDOLL );

    if( 0 != rtn ) {
        error("Reading '%s', check console for details", name );
    }
} // End proc FO2_read_ragdoll


#endif // NO_BLENDER

#endif // WITH_FO2_CARS

//-- End of file : FO2carData.cpp  --
