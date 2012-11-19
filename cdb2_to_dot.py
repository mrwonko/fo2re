import struct, sys

# Interpreting Flatout 2's track_cdb2.gen files and writing the tree that's stored in there as a Graphviz-Dot compatible graph definition file.


#   Parse Arguments

if len( sys.argv ) not in {2, 3, 4}:
    print( "Usage: {} <path/to/track_cdb2.gen> [<recursion limit>] [<bitmask>]\n  <recursion limit> defaults to -1\n  <bitmask> defaults to 0xFF\n  writes to <filepath>.dot".format( sys.argv[0] ) )
    sys.exit()

filename = sys.argv[1]

maxRecursion = -1
if len( sys.argv ) >= 3:
    maxRecursion = int( sys.argv[2] )

bitmask = ( 1 << 6 ) - 1
if len( sys.argv ) >= 4:
    bitmask = int( sys.argv[3] )


#   Open the files

inFile = open( sys.argv[1], "rb" )
outFile = open( sys.argv[1] + ".dot", "w" )

headerSize = 0x40

#   Read through input file recursively.

def WriteDotData( file, relOfs, recursionLimit, parents = None ):
    # catch loops
    if parents == None:
        parents = { relOfs }
    elif relOfs in parents:
        sys.stderr.write( "Loop in hierarchy!" )
        # no need to write anything, such a node must already exist
        return
    parents.add( relOfs )
        
    
    # offset is relative to data, i.e. 0 is just past the header
    absOfs = relOfs + headerSize

    # read data
    inFile.seek( absOfs )
    data = inFile.read( 8 )
    
    # invalid offset?
    if len( data ) < 8:
        # node is still required
        outFile.write( '\t"{1}" [label="invalid offset: {1}"]\n'.format( relOfs ) )
        return

    # interpret data
    packed, val1, val2 = struct.unpack( "I2h", data )
    flags = packed & 0xFF
    child1Offset = packed >> 8
    child2Offset = child1Offset + 8
    
    # write node to dot file
    outFile.write( '\t"{}" [label="flags: {:0>8b} val1: {:6} val2: {:6}"]\n'.format( relOfs, flags, val1, val2 ) )
    
    # is this a leaf?
    if ( flags & 3 ) == 3:
        return
    # is this relevant?
    if not ( flags >> 2 ) & bitmask:
        return
    # is recursion depth limited?
    if recursionLimit == 0:
        return
    
    # Nope, we've got children. Write those.
    WriteDotData( file, child1Offset, recursionLimit - 1, parents )
    WriteDotData( file, child2Offset, recursionLimit - 1, parents )
    
    # Write edges to children
    outFile.write( '\t"{self}" -> "{child1}" [label="child 1"]\n\t"{self}" -> "{child2}" [label="child 2"]\n'.format( self = relOfs, child1 = child1Offset, child2 = child2Offset ) )

outFile.write( "digraph\n{\n" )
WriteDotData( outFile, 0, maxRecursion )
outFile.write( "}\n" )
outFile.close()
inFile.close()
