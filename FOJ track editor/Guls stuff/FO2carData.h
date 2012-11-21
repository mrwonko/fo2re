//
//-=-=-=-=-=-=-=-=-=-=-=--=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// File        : $Id: FO2carData.h 605 2012-02-24 22:49:19Z GulBroz $
//   Revision  : $Revision: 605 $
//   Name      : $Name$
//
// Description : Class definitions to access FlatOut2 car files
// Author      :
// Date        : Tue Apr 17 21:17:12 2007
// Reader      :
// Evolutions  :
//
//  $Log$
//  Revision 1.1  2007/11/27 18:55:35  GulBroz
//  + Begin support for FO2 cars
//
//-=-=-=-=-=-=-=-=-=-=-=--=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//

#ifndef FO2_CAR_DATA_H
#define FO2_CAR_DATA_H

#ifdef WITH_FO2_CARS

//
//=============================================================================
//                              INCLUDE FILES
//=============================================================================
//
#ifdef __cplusplus
#include <vector>
#include <string>
#endif // __cplusplus

#include "LuaPlus.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus
#include "src/lua.h"
#include "src/lualib.h"
#include "src/lauxlib.h"
#ifdef __cplusplus
}
#endif // __cplusplus

#include "FO2trackData.h"

//
//=============================================================================
//                                CONSTANTS
//=============================================================================
//
#define FO2_CAR_FILE_ID      0x00020000

#define FO2_CARFILE          "body.bgm"
#define FO2_CARCAM           "camera.ini"  // Car camera file
#define FO2_CARCAM_OBJ       "Cameras"     // Car camera LUA object
#define FO2_CARCAM_SETUP     "CameraSetup"

#define FO2_CARHANDLER_NAME  "Car"
#define FO2_TIREHANDLER_NAME "Tire"

// TODO: CHECKME
#define FO2_EMPTY_FLAGS 0
#define FO2_EMPTY_SIZE  0

//
//=============================================================================
//                      TYPE DEFINITIONS AND GLOBAL DATA
//=============================================================================
//

#ifdef __cplusplus  // The following is just for c++

using namespace std;
using namespace LuaPlus;

//=============================================================================

//-----------------------------------------------------------------------------
// Name   : cCamera
// Usage  : Camera class definition
//-----------------------------------------------------------------------------
class cCamera
{
public:
    cCamera(void);
    ~cCamera();

    int  read( const char * const fileName );
    int  write( const char * const fileName );
    void show( const uint tab = 0, FILE *where = stdout );
#if !defined(NO_BLENDER)
    int toBlender( Group* gr = NULL, Object* carHdl = NULL );
//     int  fromBlender( void );
#endif //NO_BLENDER

private:
    LuaState* luaState;
    lua_State *L;
    LuaStateAuto stateOwner;
}; // End class cCamera

class cCar;

//-----------------------------------------------------------------------------
// Name   : cCarMesh / cCarMeshList
// Usage  : Car mesh stream class definition
//-----------------------------------------------------------------------------
class cCarMesh
{
public:
    uint  dwID;
    char  *sName;
    char  *sMadeOf;
    uint  dwFlag;
    int  dwGroup;          // Group number
    cMatrix4x4 matrix;
    uint  dwNmodels;
    vector<uint> dwModelIdx;

    uint _index;           // Index in list
    uint _filePos;

#if !defined(NO_BLENDER)
    Object *_obj;          // Pointer to Blender object
#endif // !NO_BLENDER

    cCarMesh()  { dwID = ID_MESH; };
    ~cCarMesh() { if( sName ) free( sName ); if( sMadeOf ) free( sMadeOf ); };
    cCarMesh( const cCarMesh& m );   // copy constructor

    int  read( FILE * h, const uint idx = 0 );
    int  write( FILE * h );
#if !defined(NO_BLENDER)
    int toBlender( cCar& car );
//     int  fromBlender( void );
#endif //NO_BLENDER
    void show( const uint tab = 0, FILE *where = stdout );
}; // End class cCarMesh

class cCarMeshList
{
public:
    uint             dwNMesh;
    vector<cCarMesh> meshes;

    cCarMeshList()  { dwNMesh = 0; }
    ~cCarMeshList() { }

    int  read( FILE *h );
    int  write( FILE * h );
#if !defined(NO_BLENDER)
    int toBlender( cCar& car );
//     int  fromBlender( void );
#endif //NO_BLENDER
    void show( const uint tab = 0, FILE *where = stdout );
}; // End class cCarMeshList

//-----------------------------------------------------------------------------
// Name   : cCarGroup
// Usage  : CarGroup class definition
//-----------------------------------------------------------------------------
class cCarGroup
{
public:
#if !defined(NO_BLENDER)
    Group* car;
    Group* windows;
    Group* lights;
    Group* shadow;
    Group* damage;
    Group* holders;
    Group* cameras;
#endif //NO_BLENDER

    cCarGroup( const FO2_FileType_t fileType );
    ~cCarGroup() {};

private:
#if !defined(NO_BLENDER)
    char groupName[ID_NAME_LG];
#endif //NO_BLENDER
}; // End class cCarGroup

//-----------------------------------------------------------------------------
// Name   : cCarHeader
// Usage  : CarHeader class definition
//-----------------------------------------------------------------------------
class cCarHeader
{
public:
    uint dwType;

    cCarHeader()  { dwType = 0; };
    ~cCarHeader() {};

    int  read( FILE * h );
    int  write( FILE * h );
    void show( const uint tab = 0, FILE *where = stdout );
}; // End class cCarHeader

//-----------------------------------------------------------------------------
// Name   : cCar
// Usage  : Car class definition
//-----------------------------------------------------------------------------
class cCar
{
public:
    char            *carName;
    cCarHeader       header;
    cMaterialList    materialList;
    cStreamList      streamList;
    cSurfaceList     surfaceList;
    cModelList       modelList;
    cCarMeshList     meshList;
    cObjectList      objectList;
//     cBoundingBoxList boundingBoxList;
//     cMeshToBBoxList  meshToBBoxList;
    cCamera          cameras;

    uint _filePos;
    char *_logFile;

    cCar()                          { carName = NULL;
                                        _logFile  = NULL;
                                        _filePos  = 0; };
    cCar( const char * const name ) { carName = strdup( name );
                                        _filePos = 0; };
    ~cCar()                         { if( carName ) free( carName );
                                        if( _logFile )  free( _logFile ); };

    int  read( const char * const fileName,
               const int printOnLoad = 0,
               const char * const name = NULL,
               const char * const logFile = NULL,
               const FO2_FileType_t fileType = FO2_CAR );
    int  write( const char * const fileName,
                const int printOnWrite = 0 );
    void show( const uint tab = 0, FILE *where = stdout );
}; // End class cCar
#endif // __cplusplus

#endif // WITH_FO2_CARS

#endif // FO2_CAR_DATA_H

//-- End of file : FO2carData.h  --
