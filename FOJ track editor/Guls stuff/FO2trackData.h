//
//-=-=-=-=-=-=-=-=-=-=-=--=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// File        : $Id: FO2trackData.h 605 2012-02-24 22:49:19Z GulBroz $
//   Revision  : $Revision: 605 $
//   Name      : $Name$
//
// Description : Class definitions to access FlatOut2 track files
// Author      :
// Date        : Sat Oct 07 09:54:12 2006
// Reader      :
// Evolutions  :
//
//  $Log$
//  Revision 1.3  2007/11/27 18:52:49  GulBroz
//  + Begin support for FO2 cars
//
//  Revision 1.2  2007/03/23 23:03:50  GulBroz
//    + Add support for multi-UV
//
//  Revision 1.1  2007/03/13 21:27:58  GulBroz
//    + Port to Blender 2.43
//
//  ----- Switch to Blender 2.43
//
//  Revision 1.7  2007/02/20 19:48:52  GulBroz
//    + Add export and load to AI .bed files
//
//  Revision 1.6  2007/02/17 17:03:27  GulBroz
//    + AI stuff (almost final)
//
//  Revision 1.5  2007/01/22 22:15:10  GulBroz
//    + Move matrices to FO2data
//
//  Revision 1.4  2007/01/01 11:34:40  GulBroz
//    + Update headers
//    + Fix group management in Meshes
//
//  Revision 1.3  2006/12/26 13:08:36  GulBroz
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
//  Revision 1.4  2006/10/14 16:09:47  GulBroz
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

#ifndef FO2_TRACK_DATA_H
#define FO2_TRACK_DATA_H

//
//=============================================================================
//                              INCLUDE FILES
//=============================================================================
//
// #ifdef BUILD_DLL

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef _MSC_VER
#define ANSI
#endif //_MSC_VER
#include <stdarg.h>

#ifdef __cplusplus
#include <vector>
#include <string>
#endif // __cplusplus

#include "FO2data.h"

#if !defined(NO_BLENDER)

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include "DNA_image_types.h"
#include "DNA_object_types.h"
#include "DNA_group_types.h"
#include "DNA_listBase.h"
#ifdef FO2_USE_ARMATURE
#include "BIF_editarmature.h"
#endif // FO2_USE_ARMATURE

#ifdef __cplusplus
}
#endif // __cplusplus

/* GS reads the memory pointed at in a specific ordering.
   only use this definition, makes little and big endian systems
   work fine, in conjunction with MAKE_ID */
#define GS(a)	(*((short *)(a)))

#endif // NO_BLENDER


//
//=============================================================================
//                                CONSTANTS
//=============================================================================
//

// Separator (group number, ... )
#define FO_SEPARATOR '@'

// Group name for FO
#define FOGROUP_NAME    "FOGroup"
#define LG_FOGROUP_NAME 7  // Length if FOGROUP_NAME

#ifdef __cplusplus  // The following is just for c++

using namespace std;

#define STREAM_TYPE_VERTEX   1
#define STREAM_TYPE_INDEX    2
#define STREAM_TYPE_UNKNOWN  3

// Vertex stream flags
#define VERTEX_POSITION			(1<<1)
#define VERTEX_UV				(1<<8)
#define VERTEX_UV2				(1<<9)
#define VERTEX_NORMAL			(1<<4)
#define VERTEX_BLEND			(1<<6)
#define VERTEX_FOUC             (1<<13)
#define STREAM_VERTEX_DECAL		(VERTEX_POSITION | VERTEX_UV)
#define STREAM_VERTEX_MODEL		(VERTEX_POSITION | VERTEX_UV  | VERTEX_NORMAL)
#define STREAM_VERTEX_STATIC	(VERTEX_POSITION | VERTEX_UV  | VERTEX_BLEND)
#define STREAM_VERTEX_WINDOW	(VERTEX_POSITION | VERTEX_UV  | VERTEX_NORMAL | VERTEX_BLEND)
#define STREAM_VERTEX_TERRAIN	(VERTEX_POSITION | VERTEX_UV2 | VERTEX_NORMAL)
#define STREAM_VERTEX_TERRAIN2	(VERTEX_POSITION | VERTEX_UV2)

#define FOUC_FACTOR  8192

#define TEXTURE_TILE     0x01
#define TEXTURE_NORMAL   0x02
#define TEXTURE_DYNAMIC  0x03

#define FO2_MAKE_ID(a,b,c,d) ( ((((((d) << 8) | (c)) << 8) | (b)) << 8) | (a))
#define ID_MATERIAL FO2_MAKE_ID( 'M', 'A', 'T', 'C' )
#define ID_MODEL    FO2_MAKE_ID( 'B', 'M', 'O', 'D' )
#define ID_MESH     FO2_MAKE_ID( 'M', 'E', 'S', 'H' )
#define ID_OBJECT   FO2_MAKE_ID( 'O', 'B', 'J', 'C' )

#define SHOW_ID(id) (char)(((id) & 0x000000FF) ),      \
                    (char)(((id) & 0x0000FF00) >>  8), \
                    (char)(((id) & 0x00FF0000) >> 16), \
                    (char)(((id) & 0xFF000000) >> 24)

#define FO2_TRACK_FILE_ID      0x00020001

// Track textures
#define TEX_COLORMAP "colormap.tga"
#define TEX_LIGHTMAP "lightmap1_w2.tga"
// Car textures
#define TEX_SKIN     "skin"
#define TEX_WINDOWS  "windows.tga"
#define TEX_WINDOWS2 "windows_"
#define TEX_LIGHTS   "lights.tga"
#define TEX_LIGHTS2  "lights_"
#define TEX_SHADOW   "_shadow"

#define MAX_NB_TEXTURES 2			// Max number of textures per FO2 material
#define TEXTURE1_NAME "Texture1"
#define TEXTURE2_NAME "Texture2"

#define DEF_OBJECT_NAME "surface_%d"	// Default object name

#define SURF_MODE_INDEX	4		// Vertex indexes
#define SURF_MODE_STRIP	5		// Vertex strips

#define MODEL_LOD_SUFFIX     "_lod"		// End of name for level of detail#1 models
#define MODEL_LOD1_SUFFIX    "_lod1"	// End of name for level of detail#1 models
#define MODEL_LOD2_SUFFIX    "_lod2"	// End of name for level of detail#2 models
#define MODEL_LOD_SUFFIX_LG   4			// Level of detail name suffix length
#define MODEL_LODN_SUFFIX_LG  4			// Level of detail name suffix length

#define MODEL_IS_DAMAGE		(1<<0)
#define MODEL_IS_CHILD		(1<<1)
#define MODEL_IS_LOD		(1<<2)

//
//=============================================================================
//                      TYPE DEFINITIONS AND GLOBAL DATA
//=============================================================================
//

// Notes:
//   1) in FlatOut y and z are inverted, compared to Blender.
//      Internally, the Blender order is used (x,y,z). Conversions are made
//      to and from FlatOut order (x,z,y) on reading and writing data to file.
//      This is done for cPoint3D, cMatrix3x3 and cMatrix4x4
//   2) During modles/surfaces export, the following blender object flags are used:
//        ob->par1 : Export flag
//        ob->par2 : Surface index
//        ob->par3 : Model index
//        ob->par4 : boundingbox index
//        ob->par5 : 'Is damage' (1<<0), 'Is child' (1<<1) or 'Is LOD' (1<<2) flag


class cTrack;

//-----------------------------------------------------------------------------
// Name   : cPoint3D
// Usage  : 3D point class definition
//-----------------------------------------------------------------------------
class cPoint3D
{
public:
    float x,y,z;

    cPoint3D() { x = y = z = 0.0; };
    ~cPoint3D() {};

    int  read(FILE * h, bool fouc= false);
    int  write(FILE * h, bool fouc= false);
    void show(const char * prefix = NULL, FILE *where = stdout);
}; // End class cPoint3D

//-----------------------------------------------------------------------------
// Name   : cPoint2D
// Usage  : 2D point class definition
//-----------------------------------------------------------------------------
class cPoint2D
{
public:
    float u,v;

    cPoint2D() { u = v = 0.0; };
    ~cPoint2D() {};

    int  read(FILE * h, bool fouc= false);
    int  write(FILE * h, bool fouc= false);
    void show(const char * suffix = NULL, FILE *where = stdout);
}; // End class cPoint2D

//-----------------------------------------------------------------------------
// Name   : cMatrix4x4
// Usage  : 4x4 matrix
//-----------------------------------------------------------------------------
// class cMatrix3x3;
class cMatrix4x4
{
public:
    float m[4][4];

	cMatrix4x4() { zero(); };
//     cMatrix4x4( const cMatrix3x3& m3 );
    ~cMatrix4x4() {};

	void zero(void);
    int  read( FILE * h );
    int  write( FILE * h );
    void show( const uint tab = 0, FILE *where = stdout );
}; // End class cMatrix4x4

//-----------------------------------------------------------------------------
// Name   : cMatrix3x3
// Usage  : 3x3 matrix
//-----------------------------------------------------------------------------
class cMatrix3x3
{
public:
    float m[3][3];

	cMatrix3x3() { zero(); };
    ~cMatrix3x3() {};

	void zero( void );
    int  read( FILE * h );
    int  write( FILE * h );
    void show( const uint tab = 0, FILE *where = stdout );
}; // End class cMatrix3x3

//-----------------------------------------------------------------------------
// Name   : cMaterialList / cMaterial
// Usage  : Material class definition
//-----------------------------------------------------------------------------
class cMaterial
{
public:
    uint           dwID;         // Id
    string         sName;        // Name
    uint           dwAlpha;      // Uses alpha channel flag
    uint           dwUnk1;       // unknown
    uint           dwNtex;       // Number of textures in this material
    uint           bUnknown[26]; // unknown
    vector<string> sTexture;     // Name of textures (3 of them/last not used)

#if !defined(NO_BLENDER)
    Image *_img[3];		// Blender image object
	Material *_mat;		// Blender material
#endif // !NO_BLENDER

    uint _index;       // Index of material in list
    uint _filePos;     // Offset of material in file

    cMaterial();
    ~cMaterial() {};

    int read( FILE *h, const uint idx = 0 );
    int  write( FILE * h );
#if !defined(NO_BLENDER)
    int toBlender( const char* lightDir, const char* texDir,
                   const bool carMaterial = false );
	int fromBlender( Material *ma, const bool carMaterial = false );
#endif //NO_BLENDER
    void show( const uint tab = 0, FILE *where = stdout );
}; // End class cMaterial


class cMaterialList
{
public:
    uint               dwNmaterials;  // Number of materials in list
    vector <cMaterial> materials;     // Material list

    uint _filePos;     // Offset of material list in file

    cMaterialList()  { dwNmaterials = 0; };
    ~cMaterialList() { };

    int  read( FILE * h );
    int  write( FILE * h );
	void clear( void ) { materials.clear(); dwNmaterials = 0; }

#if !defined(NO_BLENDER)
    int toBlender( const char* lightDir, const char* texDir,
                   const bool carMaterial = false );
	int fromBlender( const bool carMaterial = false );
	int indexOf( Material *mat );
#endif //NO_BLENDER
    void show( const uint tab = 0, FILE *where = stdout );
}; // End class cMaterialList


//-----------------------------------------------------------------------------
// Name   : cStream / cStreamList
// Usage  : Generic stream class definition
//-----------------------------------------------------------------------------
class cStream
{
public:
    uint dwType;
    uint dwZero;
    uint dwNum;
    uint dwStride;
    uint dwFormat;

    uint _filePos;
    uint _index;

    cStream()  { dwType = dwZero = dwNum = dwStride = dwFormat = 0;
                 _filePos = _index = 0; };
//     ~cStream() {};
    virtual ~cStream();

    int  read( FILE * h );
    int  write( FILE * h );
    virtual void show(const uint n, const uint tab = 0,
		FILE *where = stdout) = 0;
}; // End class cStream

class cStreamList
{
public:
    uint            dwNstreams;  // Number of streams in list
    vector<cStream*> streams;    // Stream list

    uint _filePos;			// Offset of stream list in file

	int _vIdx;				// Index of last vertex stream used
	int _iIdx;				// Index of last index stream used

    cStreamList()  { dwNstreams = 0; _vIdx = _iIdx = -1; };
	~cStreamList() { clear(NULL,false); };

	void clear( cTrack *track, const bool modelsOnly );
    int  read( FILE * h );
    int  write( FILE * h );
    void show( const uint tab = 0, FILE *where = stdout );
#if !defined(NO_BLENDER)
	int organize( cTrack& track );
#endif // NO_BLENDER
}; // End class cStreamList

//-----------------------------------------------------------------------------
// Name   : cVertex / cVertexList
// Usage  : Vertex stream class definition
//-----------------------------------------------------------------------------
class cVertex
{
public:
    cPoint3D pos;
    cPoint3D normal;
    float    fBlend;
    cPoint2D uv[2];

    uint _index;         // Index of vertex in list
    uint _offsetStream;

    cVertex() { fBlend = 0.0; _offsetStream = 0; }
    ~cVertex() {};

    int  read( FILE * h, const uint format,
               const uint basePos, const uint idx = 0 );
    int  write( FILE * h, const uint format,
               const uint basePos );
    void show( const uint format, const uint tab = 0,
               FILE *where = stdout );
}; // End class cVertex

class cVertexStream : public cStream
{
public:
    vector<cVertex> vertices;

    cVertexStream(const uint format);
    ~cVertexStream() {}

    int  read( FILE *h );
    int  write( FILE * h );
    void show( const uint n, const uint tab = 0, FILE *where = stdout );
}; // End class cVertexStream

//-----------------------------------------------------------------------------
// Name   : cIndex / cIndexStream
// Usage  : Index stream class definition
//-----------------------------------------------------------------------------
// class cIndex
// {
// public:
//     cPoint3D pos;
//     cPoint3D normal;
//     float    fBlend;
//     cPoint2D uv;
//     cPoint2D uv2;

//     uint _index;         // Index of vertex in list
//     uint _offsetStream;

//     cVertex() { fBlend = 0.0; _offsetStream = 0; }
//     ~cVertex() {};

//     int  read( FILE * h, const uint format,
//                const uint basePos, const uint idx = 0 );
//     void show( const uint format, const uint tab = 0,
//                FILE *where = stdout );
// }; // End class cIndex

class cIndexStream : public cStream
{
public:
    vector<ushort> indexes;

    cIndexStream() : cStream() { dwType = STREAM_TYPE_INDEX; }
    ~cIndexStream() { }

    int  read( FILE *h );
    int  write( FILE * h );
    void show( const uint n, const uint tab = 0, FILE *where = stdout );
}; // End class cIndexStream

//-----------------------------------------------------------------------------
// Name   : cUnknown / cUnknownList
// Usage  : Unknown stream class definition
//-----------------------------------------------------------------------------
class cUnknown
{
public:
    cPoint3D pos;
    cPoint3D normal;
    uint     dwGroup;

    uint _index;         // Index of vertex in list
    uint _offsetStream;

    cUnknown() { dwGroup = 0; _offsetStream = 0; }
    ~cUnknown() {}

    int  read( FILE * h, const uint format,
               const uint basePos, const uint idx = 0 );
    int  write( FILE * h, const uint format,
                const uint basePos );
    void show( const uint format, const uint tab = 0,
               FILE *where = stdout );
}; // End class cUnknown

class cUnknownStream : public cStream
{
public:
    vector<cUnknown> unknowns;

    cUnknownStream() : cStream() { dwType = STREAM_TYPE_UNKNOWN; }
    ~cUnknownStream() { }

    int  read( FILE *h );
    int  write( FILE * h );
    void show( const uint n, const uint tab = 0, FILE *where = stdout );
}; // End class cUnknownStream

//-----------------------------------------------------------------------------
// Name   : cSurface / cSurfaceList
// Usage  : Surface stream class definition
//-----------------------------------------------------------------------------
class cSurface
{
public:
    uint dwIsNotMesh;      // Surface is a mesh? 0=Yes 1=No (does not use Index stream then) -1=Free surface
    uint dwMatIdx;         // Index of material to use
    uint dwVertNum;        // Number of vertices in surface
    uint dwFormat;         // Surface format
    uint dwPolyNum;        // Number of polygons (from index tables)
    uint dwPolyMode;       // Polygon mode: 4-triindx or 5-tristrip
    uint dwPolyNumIndex;   // Number of indexes for polygon
    uint dwStreamUse;      // Number of streams used (1 or 2: Vstream/Istream)
    uint dwVStreamIdx;     // Index of vertex stream to use
    uint dwVStreamOffset;  // Offset to 1st vertex in vertex stream
    uint dwIStreamIdx;     // Index of index stream to use
    uint dwIStreamOffset;  // Offset to 1st index in index stream

    uint _index;           // Index of surface in list
    uint _filePos;

	uint _VStreamOffset;	// Index of 1st vertex in vertex stream
	uint _IStreamOffset;	// Index of 1st index in index stream

#if !defined(NO_BLENDER)
    Object   *_obj;			// Blender object
	Material *_mat;			// Blender material
#endif // !NO_BLENDER

    cSurface();
    ~cSurface() {};

    int  read( FILE * h, cStreamList &sList, const uint idx = 0 );
    int  write( FILE * h );
#if !defined(NO_BLENDER)
	int toBlender( cTrack& track );
	int fromBlender( Object *ob, cTrack& track );
	int linkMaterials( cTrack& track );
	int fixMaterial( cTrack& track );
#endif //NO_BLENDER
    void show( const uint tab = 0, FILE *where = stdout );
}; // End class cSurface

class cSurfaceList
{
public:
    uint			 dwNSurface;
    vector<cSurface> surfaces;

	int _curLayer;	// Index of layer being processed

    cSurfaceList()  { dwNSurface = 0; _curLayer = -1; }
    ~cSurfaceList() { }

    int  read( FILE *h, cStreamList &sList );
    int  write( FILE * h );
	void clear( const bool modelsOnly );
#if !defined(NO_BLENDER)
	int toBlender( cTrack& track );
	int fromBlender( cTrack& track, const bool modelsOnly = true );
	int linkMaterials( cTrack& track );
	//int addSurface( Object *ob, uint layer, cTrack& track );
#endif //NO_BLENDER
    void show( const uint tab = 0, FILE *where = stdout );
}; // End class cSurfaceList

//-----------------------------------------------------------------------------
// Name   : cSurfCenter / cSurfCenterList
// Usage  : Surface center class definition
//-----------------------------------------------------------------------------
class cSurfCenter
{
public:
    uint  dwIdx1;		// Index of surface center in list
    uint  dwIdx2;		// Index of surface center in list
    uint  dwSurfIdx;	// Index of surface in surface list
    cPoint3D posWrld;	// Position of surface center in world
    cPoint3D posBBox;	// Position of surface center in surface bounding box

    uint _index;           // Index in list
    uint _filePos;

    cSurfCenter();
    ~cSurfCenter() {};

    int  read( FILE * h, const uint idx = 0 );
    int  write( FILE * h );
    void show( const uint tab = 0, FILE *where = stdout );
}; // End class cSurfCenter

class cSurfCenterList
{
public:
    uint                dwNSurfCenter;
    vector<cSurfCenter> surfcenters;

    cSurfCenterList()  { dwNSurfCenter = 0; }
    ~cSurfCenterList() {  }

    int  read( FILE *h );
    int  write( FILE * h );
    void show( const uint tab = 0, FILE *where = stdout );
	void clear( void ) { surfcenters.clear(); dwNSurfCenter = 0; }
}; // End class cSurfCenterList

//-----------------------------------------------------------------------------
// Name   : cUnknown2List
// Usage  : Unknown2 stream class definition
//-----------------------------------------------------------------------------
class cUnknown2List
{
public:
    uint         dwNUnknown2;
    vector<uint> unknown2s;

    cUnknown2List()  { dwNUnknown2 = 0; }
    ~cUnknown2List() { }

    int  read( FILE *h );
    int  write( FILE * h );
    void show( const uint tab = 0, FILE *where = stdout );
}; // End class cUnknown2List

//-----------------------------------------------------------------------------
// Name   : cUnknown3 / cUnknown3List
// Usage  : Unknown3 stream class definition
//-----------------------------------------------------------------------------
class cUnknown3
{
public:
    float fUnk1;
    float fUnk2;
    float fUnk3;
    float fUnk4;
    float fUnk5;
    uint  dwUnk1;
    uint  dwUnk2;

    uint _index;           // Index in list
    uint _filePos;

    cUnknown3();
    ~cUnknown3() {};

    int  read( FILE * h, const uint idx = 0 );
    int  write( FILE * h );
    void show( const uint tab = 0, FILE *where = stdout );
}; // End class cUnknown3

class cUnknown3List
{
public:
    uint              dwNUnknown3;
    vector<cUnknown3> unknown3s;

    cUnknown3List()  { dwNUnknown3 = 0; }
    ~cUnknown3List() { }

    int  read( FILE *h );
    int  write( FILE * h );
    void show( const uint tab = 0, FILE *where = stdout );
}; // End class cUnknown3List

//-----------------------------------------------------------------------------
// Name   : cUnknown4 / cUnknown4List
// Usage  : Unknown4 stream class definition
//-----------------------------------------------------------------------------
class cUnknown4
{
	// It seems this is used to manage tree LOD
public:
    int  dwUnk1;
    int  dwUnk2;
	int  dwSurfIdx1;	// Surface index (from unknown stream)
    int  dwUnk3;
    float fUnk1[19];
	int  dwSurfIdx2;	// Surface index
	int  dwSurfIdx3;	// Surface index
	int  dwSurfIdx4;	// Surface index (from unknown stream)
    int  dwUnk4;
    int  dwUnk5;
	int dwMatIdx;		// Material index (LOD texture, of dwSurfIdx4?)

    uint _index;		// Index in list
    uint _filePos;

    cUnknown4();
    ~cUnknown4() {};

    int  read( FILE * h, const uint idx = 0 );
    int  write( FILE * h );
    void show( const uint tab = 0, FILE *where = stdout );
}; // End class cUnknown4

class cUnknown4List
{
public:
    uint              dwNUnknown4;
    vector<cUnknown4> unknown4s;

    cUnknown4List()  { dwNUnknown4 = 0; }
    ~cUnknown4List() { }

    int  read( FILE *h );
    int  write( FILE * h );
    void show( const uint tab = 0, FILE *where = stdout );
}; // End class cUnknown4List

//-----------------------------------------------------------------------------
// Name   : cUnknown5List
// Usage  : Unknown5 stream class definition
//-----------------------------------------------------------------------------
class cUnknown5List
{
public:
    uint          dwNUnknown5;
    vector<float> unknown5s;

    cUnknown5List()  { dwNUnknown5 = 0; }
    ~cUnknown5List() { }

    int  read( FILE *h );
    int  write( FILE * h );
    void show( const uint tab = 0, FILE *where = stdout );
}; // End class cUnknown5List

//-----------------------------------------------------------------------------
// Name   : cModel / cModelList
// Usage  : Model stream class definition
//-----------------------------------------------------------------------------
class cModel
{
public:
    uint     dwID;
    uint     dwUnk1;			// ??? Set to 4 on export... (except for ragdoll / car ?)
    char    *sName;				// Name of 1st mesh /wo "@nnn" extension
    cPoint3D bBoxCenterOffset;	// Offset from object origin to center of bounding box
    cPoint3D bBox;				//  Bounding box ( = Blender bounding box / 2 )
    float    bSphereR;			//  Bounding sphere radius ( = | bBox | )
    uint     dwNSurfIdx;		//  Number of surface indexes
    vector<uint> dwSurfIdx;		//  Surface indexes

    uint _index;           // Index in list
    uint _filePos;

#if !defined(NO_BLENDER)
	Object *_obj;
#endif //NO_BLENDER

    cModel();
    ~cModel();
    cModel( const cModel& m );   // copy constructor

    int  read( FILE * h, const uint idx = 0 );
    int  write( FILE * h );
#if !defined(NO_BLENDER)
	int toBlender( cTrack& track );
	int fromBlender( Object *ob );
#endif //NO_BLENDER
    void show( const uint tab = 0, FILE *where = stdout );
}; // End class cModel

class cModelList
{
public:
    uint           dwNModel;
    vector<cModel> models;

    cModelList()  { dwNModel = 0; }
    ~cModelList() { }

    int  read( FILE *h );
    int  write( FILE * h );
	void clear( void ) { models.clear(); dwNModel = 0; }
#if !defined(NO_BLENDER)
	int toBlender( cTrack& track );
	int fromBlender( cTrack& track );
	int organize( cTrack& track );
#endif //NO_BLENDER
	void show( const uint tab = 0, FILE *where = stdout );
}; // End class cModelList

//-----------------------------------------------------------------------------
// Name   : cObject / cObjectList
// Usage  : Object stream class definition
//-----------------------------------------------------------------------------
class cObject
{
public:
    uint  dwID;
    char  *sName;
    char  *sUnk1;
    uint  dwFlag;
    cMatrix4x4 matrix;

    uint _index;           // Index in list
    uint _filePos;

#if !defined(NO_BLENDER)
    Object *_obj;          // Pointer to Blender object
#endif // !NO_BLENDER

    cObject();
    cObject( const cObject& o );   // copy constructor
    ~cObject() { if( sName ) free( sName ); if( sUnk1 ) free( sUnk1 ); };

    int  read( FILE * h, const uint idx = 0 );
    int  write( FILE * h );
#if !defined(NO_BLENDER)
#ifdef FO2_USE_ARMATURE
    EditBone *toBlender( const int layer );
#else
    // int toBlender( Group *gr ); // For cars?
	int toBlender( const uint index, cTrack& track );
	int fromBlender( Object *ob );
#endif // FO2_USE_ARMATURE
//     int  fromBlender( void );
#endif //NO_BLENDER
    void show( const uint tab = 0, FILE *where = stdout );
    void raz( void ) { sName = sUnk1 = NULL; };
}; // End class cObject

class cObjectList
{
public:
    uint             dwNObject;
    vector <cObject> objects;

    cObjectList()  { dwNObject = 0; }
    ~cObjectList() { }

    int  read( FILE *h );
    int  write( FILE * h );
#if !defined(NO_BLENDER)
    //int toBlender( Group *gr = NULL ); // For cars?
    int toBlender( cTrack& track );
	int fromBlender( cTrack& track );
#endif //NO_BLENDER
    void show( const uint tab = 0, FILE *where = stdout );
}; // End class cObjectList

//-----------------------------------------------------------------------------
// Name   : cBoundingBox / cBoundingBoxList
// Usage  : BoundingBox stream class definition
//-----------------------------------------------------------------------------
class cBoundingBox
{
public:
    uint  dwNbIdx;				// Number of model indexes
    uint  dwModelIdx[16];		// Model index !! dwNbIdx values used !!
    cPoint3D bBoxCenterOffset;	// Offset from object origin to center of bounding box
    cPoint3D bBox;				// Bounding box ( = Blender bounding box / 2 )

    uint _index;				// Index in list
    uint _filePos;

#if !defined(NO_BLENDER)
	Object *_obj;
#endif //NO_BLENDER

	cBoundingBox();
    ~cBoundingBox() {};

    int  read( FILE * h, const uint idx = 0 );
    int  write( FILE * h );
#if !defined(NO_BLENDER)
	int  fromBlender( Object *ob, cModel *mo );
#endif //NO_BLENDER
    void show( const uint tab = 0, FILE *where = stdout );
}; // End class cBoundingBox

class cBoundingBoxList
{
public:
    uint                 dwNBoundingBox;
    vector<cBoundingBox> boundingBoxes;

    cBoundingBoxList()  { dwNBoundingBox = 0; }
    ~cBoundingBoxList() { }

    int  read( FILE *h );
    int  write( FILE * h );
    void show( const uint tab = 0, FILE *where = stdout );
	void clear( void ) { boundingBoxes.clear(); dwNBoundingBox = 0; }
}; // End class cBoundingBoxList

//-----------------------------------------------------------------------------
// Name   : cMeshToBBox / cMeshToBBoxList
// Usage  : MeshToBBox stream class definition
//-----------------------------------------------------------------------------
class cMeshToBBox
{
public:
    char  *sMeshPrefix;    // Mesh name prefix
    int    dwBBIdx[2];     // Index of boundig box item in bounding box list
                           // (first is object, second damage)
    uint _index;           // Index in list
    uint _filePos;

    cMeshToBBox() { sMeshPrefix = NULL; dwBBIdx[0] = dwBBIdx[1] = -1; _index = _filePos = 0; };
    ~cMeshToBBox() { if( sMeshPrefix ) free( sMeshPrefix ); };
    cMeshToBBox( const cMeshToBBox& m );   // copy constructor

#if !defined(NO_BLENDER)
	int  fromBlender( Object *ob );
#endif //NO_BLENDER
    int  read( FILE * h, const uint idx = 0 );
    int  write( FILE * h );
    void show( const uint tab = 0, FILE *where = stdout );
}; // End class cMeshToBBox

class cMeshToBBoxList
{
public:
    uint                dwNMeshToBBox;
    vector<cMeshToBBox> meshToBBoxes;

    cMeshToBBoxList()  { dwNMeshToBBox = 0; }
    ~cMeshToBBoxList() { }

    int  read( FILE *h );
    int  write( FILE * h );
    void show( const uint tab = 0, FILE *where = stdout );
	void clear( void ) { meshToBBoxes.clear(); dwNMeshToBBox = 0; }
}; // End class cMeshToBBoxList

//-----------------------------------------------------------------------------
// Name   : cMesh / cMeshList
// Usage  : Mesh stream class definition
//-----------------------------------------------------------------------------
class cMesh
{
public:
    uint  dwID;
    char  *sName;
    char  *sMadeOf;
    uint  dwFlag;
    int  dwGroup;          // Group number
    cMatrix4x4 matrix;
    uint  dwUnk3;
    uint  dwM2BBIdx;       // Index of meshToBBox item

    uint _index;           // Index in list
    uint _filePos;
#if !defined(NO_BLENDER)
	Object *_obj;
#endif //NO_BLENDER

    cMesh();
    ~cMesh();
    cMesh( const cMesh& m );   // copy constructor

    int  read( FILE * h, const uint idx = 0 );
    int  write( FILE * h );
#if !defined(NO_BLENDER)
    int toBlender( cTrack& track );
	int fromBlender( cTrack& track, Object *ob );
#endif //NO_BLENDER
    void show( const uint tab = 0, FILE *where = stdout );
}; // End class cMesh

class cMeshList
{
public:
    uint          dwNMesh;
    uint          dwNGroup;
    vector<cMesh> meshes;

    cMeshList()  { dwNMesh = dwNGroup = 0; }
    ~cMeshList() { }

    int  read( FILE *h );
    int  write( FILE * h );
	void clear( void );
#if !defined(NO_BLENDER)
    int toBlender( cTrack& track );
	int fromBlender( cTrack& track );
	Object *findMeshObject( const char* name );
#endif //NO_BLENDER
    void show( const uint tab = 0, FILE *where = stdout );
}; // End class cMeshList

//=============================================================================

//-----------------------------------------------------------------------------
// Name   : cTrackHeader
// Usage  : TrackHeader class definition
//-----------------------------------------------------------------------------
class cTrackHeader
{
public:
    uint dwType;
    uint dwUnknown;

    cTrackHeader()  { dwType = dwUnknown = 0; };
    ~cTrackHeader() {};

    int  read( FILE * h );
    int  write( FILE * h );
    void show( const uint tab = 0, FILE *where = stdout );
}; // End class cTrackHeader


//-----------------------------------------------------------------------------
// Name   : cTrack
// Usage  : Track class definition
//-----------------------------------------------------------------------------
class cTrack
{
public:
    char            *trackName;
    cTrackHeader     header;
    cMaterialList    materialList;
    cStreamList      streamList;
    cSurfaceList     surfaceList;
    cSurfCenterList  surfcenterList;
    cUnknown2List    unknown2List;
    cUnknown3List    unknown3List;
    cUnknown4List    unknown4List;
    cUnknown5List    unknown5List;
    cModelList       modelList;
    cObjectList      objectList;
    cBoundingBoxList boundingBoxList;
    cMeshToBBoxList  meshToBBoxList;
    cMeshList        meshList;

	uint *layers;	// For mesh placement

    uint _filePos;
    char *_logFile;

    cTrack()                          { trackName = NULL;
                                        layers    = NULL;
                                        _logFile  = NULL;
                                        _filePos  = 0; };
    cTrack( const char * const name ) { trackName = strdup( name );
                                        _filePos = 0; };
    ~cTrack()                         { if( trackName ) free( trackName );
                                        if( _logFile )  free( _logFile );
                                        if( layers )    free( layers ); };

    int  read( const char * const fileName,
               const int printOnLoad = 0,
               const char * const name = NULL,
               const char * const logFile = NULL );
    int  write( const char * const fileName,
                const int printOnWrite = 0 );
    void show( const uint tab = 0, FILE *where = stdout );

	int calcLayers( void );
	int getDynObjModelLayer( void );
	int sortStreams( void );
}; // End class cTrack

#endif // __cplusplus
// #endif // BUILD_DLL

//
//=============================================================================
//                            INTERNAL ROUTINES
//=============================================================================
//
//
//=============================================================================
//                                ROUTINES
//=============================================================================
//

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus
#if !defined(NO_BLENDER)
    Object *findObject( const char *name );
    Object *findObjectMesh( const char *name, const char *mesh );
    Group *findGroup( const char *name );
    Curve *findCurve( const char *name, Object **object );
    int makeParent( Object *parent, Object *child );
	int loadPhysics( char *path, char *dir, int verbose );
#endif // NO_BLENDER
#ifdef __cplusplus
}
#endif // __cplusplus

#if !defined(NO_BLENDER)
#ifdef __cplusplus
    int moveRotateObj( Object           *obj,
                       const cMatrix4x4 *matrix,
                       char             *name = NULL );
#endif // __cplusplus
#endif // NO_BLENDER

int buildPath( const char* name, FO2_Path_t *pPath, const int forReading );
int printMsg( int flag, const char *fmt, ... );

#define MSG_CONSOLE (1<<0)
#define MSG_DBG     (1<<1)
#define MSG_ALL     (MSG_CONSOLE|MSG_DBG)

#endif // FO2_TRACK_DATA_H

//-- End of file : FO2trackData.h  --
