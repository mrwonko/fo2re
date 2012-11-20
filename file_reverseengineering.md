Next
====

See what happens when the desired node is found in the recursive cdb2 walk.

Run a Trace between the load function calls - see where they're all from.



General stuff, file handling
============================


data\tracks\forest\forest1\a\geometry\track_bvh.gen

OpenFile() seem to return a pointer to an Object. (A big one! There might be a 0x4000 byte (= 16kb) array at offset 0x4, with its size at +0x4004 (default -1))
The first element seems to be a pointer to a vtable.

It is used on data/database/flatout2.db, data/language/languages.dat, savegame/options.cfg

When anything from that vtable is called, 
It would appear that the pointer to the object of that class is in ECX

	struct FileObject
	{
		void* vtable;
		char buffer[0x4000]; // 0x4
		int unknown;           // 0x4004 - is -1 on newly opened track_spvs.gen
		char* filePos;         // 0x4008 - "virtual" file pos - reads in 16kb blocks internally, but allows for arbitrary reads
		char* realFilePos;     // 0x400C - real file's filePos, gets increased by bytesRead
		void* filesize;        // 0x4010 - returned by BvhFile_Func8 unless it's -1 - *0x4008 otherwise
		char* fileHandle;      // 0x4014
		void* unknown;         // 0x4018 - some bitmask?
		char* filenameRelated; // 0x401C
		char* filenameRelated; // 0x4020
		// presumably 6 more
	};

File opening via KERNEL32.CreateFileA, in BvhFile_Open and others
File reading via KERNEL32.ReadFile, in BvhFile_FillBuffer, which fills FileObject::buffer
File closing via KERNEL32.CloseHandle, BvhFile_Func5




File processing order (game start)
==================================

Open, Read, Close data/database/flatout2.db
Open, Read, Close data/language/languages.dat
Open, Read, Close data/language/version.ini
Open, Read, Close savegame/device.cfg
Open, Read, Close data/language/language2.dat
[load shaders (data/shader/)]
[load definition files]
data/settings/game.ini
[savegame]
[fonts, hud]




File processing order (track start)
===================================

data/global/particles/hud_particles_2d.bed
data/menu/loading_bg_<levelname>.tga
hud
sounds
scripts
data/tracks/<track>/lighting/sh_w2.ini
data/tracks/<track>/data/effectmap.4b
global filters/environment stuff (sun, skybox)
data/tracks/<track>/lighting/shadowmap_w2.dat
data/tracks/<track>/geometry/track_bvh.dat (and turn 2nd array's entries' offsets into pointers)
data/tracks/<track>/geometry/track_spvs.dat
data/global/dynamics/dynamic_objects.bed
data/settings/lods.ini
data/tracks/<track>/lighting/vertexcolors_w2.w32
data/tracks/<track>/geometry/track_geom.w32
data/tracks/<track>/geometry/track_cdb2.gen
data/tracks/<track>/geometry/plant_vdb.gen (loading of this is pretty weird...)
data/tracks/<track>/geometry/plant_geom.w32
data/tracks/<track>/data/resetmap.4b
data/tracks/<track>/data/trackai.bin
cars (player's?)
drivers (player's?)
[ cdb tree traversial starts here ]
data/tracks/<track>/data/camera.ini
data/scripts/cameratemplates.bed
menu stuff
flares and other graphics, map, particles, skybox
data/tracks/<track>/lighting/lightmap1_w2.dds
textures (in alphabetical order)
cars
drivers
hud
sounds




BVH File Parsing
================

BvhLoadingRelated1
Return bvh filename in EDI (char*) and 0x109 in ESI (=?)

Store 3rd int from track_bvh.gen in EBP+8
Call 6035CE (bvhParseRelated1) with ((3rd int) << 5) parameter
	""" safe_malloc, it appears. """
	the first thing that does is call another function 6035A2 (bvhParseRelated2) with its its as one parameter (and *(0x6A30BC) as the other))
	The memory is 0 in my case, the parameter 0x13D60.
		""" Heap allocation with alternative in case of failure """
		Since 0x13D60 is not above -0x20 (?!?), another function 603526 (bvhParseRelated3) is called with the (3rd int) << 5 parameter.
			""" Safe heap allocation (using SE Handler) of size = Argument, result in EAX (or 0), init as 0xBAADF00D """
			(
			this pushes the arguments 0x0C and 0x658E78 and calls 606108 (bvhParseRelated4)
				at this point I've had enough and continue
			Looking at the stack, some SE (Structured Exception) Handler got setup and the level string got pushed, amongst others
			round ESI to next hexnumber ending in 0 (i.e. multiple of 4)
			Then space on the heap is allocated via ntdll.RtlAllocateHeap. Size = ESI = 0x13D60 (4-byte-aligned)
			Some other function 606143 (bvhParseRelated5) is called.
				It apparently undoes the SE Handler installing from bvhParseRelated4
			)
That functions



CDB2 File Parsing
=================

Get Filesize
read header stuff (64 bytes) into existing structs
read rest into one big buffer


CDB2 tree traversal
===================

SomeTreeFunc:
	node = *ecx
	edx = ARG.2889 << 2 // at 0056AF40 - result may be 0x20
	if( node.flags & edx ) // "Arg-Flag-Test" below
		if( ( node.flags & 3 ) == 3 ) // "flag & 3 test"
			...
		else
			// 0, 1 & 2 pass.
			// comparison is done with the 0nth/1st/2nd int after esp+90
			// -> x/y/z?
			highBytes = node.flags >> 8
			ARG.6 = highBytes
			ARG.24 = shortToInt( node.link1 )
			EDI = shortToInt( node.link2 )
			[ there's something huuuge on the stack that's used here ]
			[ something in it is compared with node.link1 ]
			[ if node.link1 is <= that thing (+90) ]
				ecx = cdb2_buf + highBytes + 8 // probably +8 because node0 == root, thus never child
				[ if node.link2 is >= another thing (+A0) ]
					...
				if ecx
					goto SomeTreeFunc
			else
				...
	else
		...

Visited nodes (forest1/a)
	+-------+---------------+---------------+------------+------------+------------------+
	| Index | Arg-Flag-Test | flag & 3 test | link1 test | link2 test | next node source |
	+-------+---------------+---------------+------------+------------+------------------+
	| 0     | 1             | 0 (?)         | 1          | 0          | highBytes + 8    |
	| 2     | 1             | 0             | 1          | 0          | highBytes + 8    |
	| 45C1  | 1             | 0             | 1          | 0          | highBytes + 8    |
	| 45C3  | 1             | 0             | 0          | 1          | highBytes + 8    |
	| 45C6  | 1             | 0             | 1          | 0          | highBytes + 8    |
	| 4B1D  | 1             | 0             | 1          | 0          | highBytes + 8    |
	| 4B1F  | 1             | 0             | 1          | 0          | highBytes + 8    |
	| 4B21  | 0             | -             | -          | -          | end (b/c edi==0) | might've looked into node.link2 if there had been any?
	+-------+---------------+---------------+------------+------------+------------------+
	| 0     | 1             | 0             | 0          | 1          | highBytes + 8    |
	| 2     | 1             | 0             | 1          | 0          | highBytes + 8    |
	| 45C1  | 1             | 0             | 1          | 0          | "                |
	| 45C3  | 1             | 0             | 0          | 1          | "                |
	| 45C6  | 1             | 0             | 1          | 0          | "                |
	| ...   |               |               |            |            |                  |
	+-------+---------------+---------------+------------+------------+------------------+
	| 0     | 1             | 0             | 0?         | 1?         | highBytes + 8    | +90: -1
	| 2     | 1             | 0             | 1          | 0          |                  | +A0: 1
	| 45C1  | 1             | 0             | 1          | 0          |                  |
	| 45C3  | 1             | 0             | 0          | 1          |                  |
	| 45C6  | 1             | 0             | 1          | 0          |                  |
	| 4B1D  | 1             | 0             | 1          | 0          |                  |
	| 4B1F  | 1             | 0             | 1          | 0          |                  |
	| 4B21  | 0             |               |            |            |                  |
	+-------+---------------+---------------+------------+------------+------------------+
	| 0     | 1             | 0             |            |            |                  | +90: -1
	| same  |               |               |            |            |                  | +A0: 1
	+-------+---------------+---------------+------------+------------+------------------+
	| 0     | 1             | 0             |            |            |                  |
	|       |               |               |            |            |                  |
	|       |               |               |            |            |                  |
	|       |               |               |            |            |                  |
	|       |               |               |            |            |                  |
	+-------+---------------+---------------+------------+------------+------------------+



CDB2 usage breakpoint
---------------------

During loading, some tree traversial already starts. A memory breakpoint excluding those needs the condition:

	( eip == 56AF9E ) && ( eip != 56AFA6 ) && ( eip != 56AF9E )




BVH Files
=========

start with 0xDEADC0DE (ID)
followed by 0x1 (version)
Then some objectCount1 - forest/forest1/a/ has 2539
then objectCount1 objects of 32 bytes each

	{
		float mins[3]; // 0x0, 0x4, 0x8 - Y Up
		
		float maxs[3]; // 0xC, 0x10, 0x14
		
		unsigned int unknown;   // 0x18 always < objectCount1 and >= 0. There appear to be no gaps, for arena1 it's 0-633 (there are dublicates), so very likely some index.
		
		unsigned int index; // 0x1C first object: 0, second: 1 etc.
		// this is never actually used, I think. Instead, it is replaced by pointers, probably to the referenced structure. Which I think is in track_geom.w32
		// this replacing is first done in 0x005988D0 (SomeGeomRead2), but that doesn't replace everything, just some. Not the first N, either, but with gaps.
		// The same ones are replaced again in SomeGeomRead3, and to the same values. ?!?
		//
		// So this indexes into something in the geometry file... Probably the collision meshes, right? Something's in CDB2 though...
	}

Then some objectCount2
then objectCount2 objects of 32 bytes each

	{
		float mins[3];
		float maxs[3];
		unsigned int offset;
		unsigned int unknown; // if this is 0, the offset is into the first object array, otherwise into the second.
	}

The End.




CDB2 Files
==========


My progress
-----------


	{
	// 64 byte header:
		int unknown, unknown; // 8 bytes - ignored. second one = 0?
		// these are probably offsets, since they're kept
		// 12 bytes - starts at EBP + 18
			// e.g. (-32767, -735, -32767) -> mins?
		// 12 bytes - starts at EBP + 24
			// e.g. (32557, 32767, 31184) -> maxs?
		// 12 bytes - starts at EBP + 0C
			// e.g. (1014855377, 1001598014, 1015315347) = (0.015470222570002079, 0.0054679205641150475, 0.01617220602929592)
		// 12 bytes - starts at EBP 
			// e.g. (1115768791, 1127670407, 1115117187) = (64.64031219482422, 182.88487243652344, 61.8344841003418)
		
		// these two are apparently only needed during reading, not later
		// in fact, I don't think they are ever used?
		// 4 bytes - into ESP+38 (after ReadFromFile) - 0x28F52C Content: 0x5CEEE (forest1/a)
			// should be the size in bytes of the recurring following strcuture.
		// 4 bytes - into ESP+34 (after ReadFromFile) - 0x28F528 Content: 0x3CBF8
			// possibly some offset?
		
	// rest is data, saved in buffer

		// This is some recurring structure, I think. 
		{
			int unknown1/flags;  // or possibly Byte + Int24? typeID/flags + offset
			/*
				if ( ( ( something << 2 ) & unknown1 ) == 0 ), something is done different (0x56AF47) -> bitmask?
				else
					if( unknown1 == 3 )
						if( !( unknown2 & 0x7F ) )
							something at 0x56B08B
						else
							other stuff
				
				something is done with this >> 8 -> it would make sense for the higher 24 bytes to be a separate value!
			*/
			// the following are not unsigned!
			short unknown2/link1; // this
			short unknown3/link2; // and this are the links, I think.
		}
		
		
		
		// some reference to the BVH later on - indices in the first kind of object get changed
	}


once again, cleaner:
--------------------

	char header[64]; // to be looked into further
	
	// repeated until the file is over:
	{
		int flagsAndChildren; // low byte is flags, high 3 bytes is position of child1
		// value1 and value2 are likely mins and maxs
		short val1; // value1[i] is compared to this, where i is (flagsAndChildren & 3)
		short val2; // value2[i] is compared to this
	}
	// nodes with the same parent are adjacent, so child1pos + 8 = child2pos



user Diana from FOJ
-------------------


	dsTable01       record    dbTypeID      : Byte;       // Type ID or some flags
							  dwUnkOffset01 : Int24;      // Descr. later
							  dwUnkData01   : Word;       // Possibly ItemID of another table
							  dwUnkData02   : Word;       // Possibly ItemID of another table

	// dwUnkOffset01 - in most cases is an offset to element dsTable01, started 
	//                 from begining of daTable01 or about of it. So you have to divide by 8 to
	//                 get an index of daTable01 array;
	// But with some values of dbType it has a different data, possibly an ItemID of another table;

	// File started at position 0

	daHeader        : array [00h..2Fh] of byte;
	ddCount         : Dword;                                  // Number of bytes allocated by daTable01, so
															  // count = (ddCount/8)
	ddUnkOffset     : Dword;
	daTable01       : array [0..ddCount div 8 - 1] of dsTable01;




SPVS Files
==========

	{
		int unknown;   // 0x00 Version?
		void* offset1; // 0x04 relative to file start
		void* offset2; // 0x08 relative to file start
		void* offset3; // 0x0C relative to file start
		void* offset4; // 0x10 relative to file start
		void* offset5; // 0x14 relative to file start
		void* offset6; // 0x18 relative to file start
		void* offset7; // 0x1C relative to file start
		
		// there's some offset (?) at 0x68. Make sure that's 32-bit aligned (ofs += 7; ofs &= 0xFFFFFFF8;) before using it.
		// there's a safe_malloc( thatOffset >> 3 ) later. (?)
		// i.e. safe_malloc( 0x68 div 8 )
		// that's first filled with -1
	}




Geom Files
==========

{
	int unknown1; // read to local variable; is 0x00020001 for forest1; some version?
	int unknown2; // read to local variable; not read if unknown1 < [10210E8] (always 0x00020000?)
	
	something
	
	// Objects read via 0x005985C0 (ReadGeomCreateBuffers)
	unsigned int count1; // number of objects following
	// for each count1:
	{
		int type; // 1, 2, or 3
		// if type 1: vertex data
		{
			// to be determined
			// references vertexcolors_w2.w32 content
		}
		// if type 2: index data (triangles)
		{
			// to be determined
		}
		if type 3: other data (=?)
		{
			int unknown; // not used by game?
			unsigned int objSize;  // or count? likely size.
			unsigned int objCount; // or size? likely count. not remembered past loading into array (?). So maybe size after all?
			// objCount times:
			{
				// object of objSize size
			}
		}
	}
}




plant_geom.w32
==============

	{
		char unknown[0x08]; // ignored (?)
		char unknown[0x20];
		char unknown[0x20];
		char unknown[0x08];
		char unknown[0x08];
		char unknown[0x08];
		int unknownCount1; // space for this many 12-byte objects is reserved
		// to be determined
	}




trackai.bin
===========

	{
		int ignored;
		int something;
		// to be determined
	}