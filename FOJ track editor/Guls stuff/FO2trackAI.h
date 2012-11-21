//
//-=-=-=-=-=-=-=-=-=-=-=--=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// File        : $Id: FO2trackAI.h 346 2007-03-13 21:28:09Z GulBroz $
//   Revision  : $Revision: 346 $
//   Name      : $Name$
//
// Description : Class definitions to access FlatOut2 trackai.bin file
// Author      : 
// Date        : Wed Jan 10 22:21:52 2007
// Reader      : 
// Evolutions  : 
//
//  $Log$
//  Revision 1.1  2007/03/13 21:27:58  GulBroz
//    + Port to Blender 2.43
//
//  ----- Switch to Blender 2.43
//
//  Revision 1.4  2007/02/25 22:54:33  GulBroz
//    + Fix AltRoute index in .bed files
//    + Make raceline optional (compute for trackai.bin, skip in .bed files)
//
//  Revision 1.3  2007/02/20 19:48:52  GulBroz
//    + Add export and load to AI .bed files
//
//  Revision 1.2  2007/02/17 17:03:27  GulBroz
//    + AI stuff (almost final)
//
//  Revision 1.1  2007/01/22 22:15:46  GulBroz
//    + Add AI stuff
//
//-=-=-=-=-=-=-=-=-=-=-=--=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//

#ifndef FO2_TRACK_AI_H
#define FO2_TRACK_AI_H

//
//=============================================================================
//                              INCLUDE FILES
//=============================================================================
//
#include <stdio.h>
#ifdef __cplusplus
#include <vector>
#endif // __cplusplus

#include "FO2data.h"
#include "FO2trackData.h"

//
//=============================================================================
//                                CONSTANTS
//=============================================================================
//
#define AI_FILE_START      0x00270276
#define AI_FILE_END        0x00280276

#define AI_SPLINES_START   0x00290276
#define AI_SPLINES_END     0x00260276
#define AI_BORDERS_START   0x00290276
#define AI_BORDERS_END     0x00010376

#define AI_SPLINE_PT_START 0x00230276
#define AI_SPLINE_PT_END   0x00240276
#define AI_POINT_START     0x00280876
#define AI_POINT_END       0x00290876

#define AI_START_PT_START  0x00300876
#define AI_START_PT_END    0x00310876
#define AI_SPLIT_PT_START  0x00010976
#define AI_SPLIT_PT_END    0x00020976
#define AI_BORDER_PT_START 0x00020376
#define AI_BORDER_PT_END   0x00030376

#define AI_UNKNOWN_START   0x00040376
#define AI_UNKNOWN_END     0x00050376

#define AI_BORDER_LEFT     0  // Index of left spline
#define AI_BORDER_RIGHT    1  // Index of right spline
#define AI_RACE_LINE       4  // Index of race line
#define AI_RACE_LINE_LEFT  2  // Index of race line
#define AI_RACE_LINE_RIGHT 3  // Index of race line

//
//=============================================================================
//                      TYPE DEFINITIONS AND GLOBAL DATA
//=============================================================================
//

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

//-----------------------------------------------------------------------------
// Name   : cSplinePoints / cSplineList
// Usage  : Spline class definition
//-----------------------------------------------------------------------------
class cSplinePoints
{
public:
    uint       dwStart;
    uint       dwNextIdx;
    uint       dwZero1;
    uint       dwPrevIdx;
    uint       dwZero2;
    cMatrix3x3 m1;
    cPoint3D   p[5];
    cMatrix3x3 m2;
    // For dwUnk[] values:
    //   dwUnk[0](n)  = Avg of vector(P(n),P(n+1)) norm,
    //                  for curve(0) and curve(1) (right and left borders)
    //
    //   dwUnk[2](0)  = 0.0; dwUnk[2](n) = dwUnk[0](n-1) + dwUnk[2](n-1)
    //
    //
    //
    //
    //
    //   dwUnk[7](n)  = dwUnk[4](n)
    //
    //   dwUnk[9](n)  = n
    //   dwUnk[10](n) = 0
    //   dwUnk[11](n) = 0xffffffff
    //   dwUnk[12](n) = 0
    float      dwUnk1[9];
    uint       dwUnk2[4];
    uint       dwEnd;

    uint _index;    // TODO: REMOVEME
    uint _filePos;
    uint _raceLine; // Raceline from Blender flag

    cSplinePoints();
    ~cSplinePoints() { };

    int  read( FILE * h );
    int  write( FILE * h );
    void show( const uint tab = 0, FILE *where = stdout );
}; // End class cSplinePoints

class cSplineList
{
public:
    uint                   dwStart;
    uint                   dwNPoints;
    vector <cSplinePoints> splinePoints;
    uint                   dwUnknown[5];
    uint                   dwEnd;

    uint _index;    // TODO: REMOVEME
    uint _filePos;

    cSplineList( const uint idx = 0 );
    ~cSplineList() { }

    int  read( FILE *h );
    int  write( FILE * h );
    int  writeBEDfile( FILE * h, const uint idx );
#if !defined(NO_BLENDER)
    void toBlender( const uint idx = 0 );
    int  fromBlender( const uint idx = 0 );
#endif //NO_BLENDER
    void show( const uint tab = 0, FILE *where = stdout );
}; // End class cSplineList

//-----------------------------------------------------------------------------
// Name   : cBorder / cBorderList
// Usage  : Border class definition
//-----------------------------------------------------------------------------
class cBorder
{
public:
    uint     dwIndex;  // Point index in border list
    uint     dwPosIdx; // Point index in curve (upper byte is curve index)
    cPoint3D left;
    cPoint3D right;

    cBorder()  { dwIndex = dwPosIdx = 0; }
    ~cBorder() { };

    int  read( FILE *h );
    int  write( FILE * h );
    void show( const uint tab = 0, FILE *where = stdout );
}; // End class cBorder

class cBorderList
{
public:
    uint             dwNBorders;
    uint             dwStart;
    vector <cBorder> borders;
    uint             dwEnd;

    uint             _isBorder;

    cBorderList();
    ~cBorderList() { };

    int  read( FILE *h );
    int  write( FILE * h );
#if !defined(NO_BLENDER)
    void toBlender( void ) {};  // Not used in Blender
    int  fromBlender( const uint idx = 0 );
#endif //NO_BLENDER
    void show( const uint tab = 0, FILE *where = stdout );
}; // End class cBorderList

//-----------------------------------------------------------------------------
// Name   : cStartPt / cStartPtList
// Usage  : Start points class definition
//-----------------------------------------------------------------------------
class cStartPt
{
public:
    cPoint3D   pos;
    cMatrix3x3 m;

    cStartPt()  {}
    ~cStartPt() {}

    int  read( FILE *h );
    int  write( FILE * h );
#if !defined(NO_BLENDER)
    void toBlender( const uint idx = 0 );
#endif //NO_BLENDER
    void show( const uint tab = 0, FILE *where = stdout );
}; // End class cStartPt

class cStartPtList
{
public:
    uint              dwStart;
    uint              dwNStartPt;
    vector <cStartPt> startPt;
    uint              dwEnd;

    cStartPtList();
    ~cStartPtList() { }

    int  read( FILE *h );
    int  write( FILE * h );
    int  writeBEDfile( FILE * h );
#if !defined(NO_BLENDER)
    void toBlender(void);
    int  fromBlender( void );
#endif //NO_BLENDER
    void show( const uint tab = 0, FILE *where = stdout );
}; // End class cStartPtList

//-----------------------------------------------------------------------------
// Name   : cSplitPt / cSplitPtList
// Usage  : Split points class definition
//-----------------------------------------------------------------------------
class cSplitPt
{
public:
    cPoint3D pos;
    cPoint3D left;
    cPoint3D right;

    cSplitPt()  { }
    ~cSplitPt() { }

    int  read( FILE *h );
    int  write( FILE * h );
    void show( const uint tab = 0, FILE *where = stdout );
}; // End class cSplitPt

#define AI_NB_POINTS_IN_SPLITPT_CURVE 3  // Nb. of points in splitpoint curve

class cSplitPtList
{
public:
    uint              dwStart;
    uint              dwNSplitPt;
    vector <cSplitPt> splitPt;
    uint              dwEnd;

    cSplitPtList();
    ~cSplitPtList() { }

    int  read( FILE *h );
    int  write( FILE * h );
    int  writeBEDfile( FILE * h );
#if !defined(NO_BLENDER)
    void toBlender( void );
    int  fromBlender( void );
#endif //NO_BLENDER
    void show( const uint tab = 0, FILE *where = stdout );
}; // End class cSplitPtList

//-----------------------------------------------------------------------------
// Name   : cAIunknownList / cAIunknown
// Usage  : Unknown class definition
//-----------------------------------------------------------------------------
class cAIunknown
{
public:
    uint dwUnk1;
    uint dwUnk2;

    cAIunknown()  { }
    ~cAIunknown() { }

    int  read( FILE *h );
    int  write( FILE * h );
    void show( const uint tab = 0, FILE *where = stdout );
}; // End class cAIunknown

class cAIunknownList
{
public:
    uint                 dwNUnk;
    uint                 dwStart;
    vector <cAIunknown>  unknowns;
    uint                 dwEnd;

    cAIunknownList() ;
    ~cAIunknownList() { }

    int  read( FILE *h );
    int  write( FILE * h );
#if !defined(NO_BLENDER)
    void toBlender( void ) {};  // Not used in Blender
    int  fromBlender( const uint nb = 0 );
#endif //NO_BLENDER
    void show( const uint tab = 0, FILE *where = stdout );
}; // End class cAIunknownList

//-----------------------------------------------------------------------------
// Name   : cTrackAI
// Usage  : TrackAI class definition
//-----------------------------------------------------------------------------
class cTrackAI
{
public:
    uint           dwStart;
    uint           dwNSplineList;
    vector <cSplineList> splineList;
    uint           dwStartPt;
    cStartPtList   startPtList;
    cSplitPtList   splitPtList;
    uint           dwEndPt;
    uint           dwStartBd;
    cBorderList    borderList;
    cAIunknownList unknownList;
    uint           dwEndBd;
    uint           dwEnd;

    char *_baseDir;
    char *_logFile;

    cTrackAI();
//     cTrackAI( const char * const base,
//             const char * const log = NULL ) { _baseDir = strdup( base );
//                                               _logFile = ( log ?
//                                                            strdup( log ) :
//                                                            NULL );
//                                               dwStart = AI_FILE_START;
//                                               dwEnd   = AI_FILE_END; };
    ~cTrackAI()                       { if( _baseDir ) free( _baseDir );
                                        if( _logFile ) free( _logFile ); };

    int  read( const char * const baseDir,
               const int printOnLoad = 0,
               const char * const logFile = NULL );
    int  write( const char * const baseDir,
                const int printOnWrite = 0 );
#if !defined(NO_BLENDER)
    void toBlender( void );
    int  fromBlender( void );
#endif //NO_BLENDER
    void show( const uint tab = 0, FILE *where = stdout );
}; // End class cTrackAI


#if !defined(NO_BLENDER)
Object *addCurve( vector<BPoint> &points, const char *name,
                  const bool cyclic = false );
#endif //NO_BLENDER

#endif // FO2_TRACK_AI_H

//-- End of file : FO2trackAI.h  --
