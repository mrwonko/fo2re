# reading Flatout 2's track_bvh.gen file, interpreting it
import sys, struct

# Print error message + exit
def error( message ):
    sys.stderr.write( message)
    sys.exit( 1 )

# the structures in the track_bvh.gen file
class NodeA:
    nodeFormat = '\tnodeA_{index} [label = "Node A{index}\\nvalue1 = {value1}\\nvalue2 = {value2}" color=blue]\n'
    
    def __init__( self, file, index ):
        self.mins = struct.unpack( "3f", file.read( 12 ) )
        self.maxs = struct.unpack( "3f", file.read( 12 ) )
        self.value1, self.value2 = struct.unpack( "2I", file.read( 8 ) )
        self.index = index

    def ToDot( self ):
        return self.nodeFormat.format( index = self.index, value1 = self.value1, value2 = self.value2 )

class NodeB:
    nodeFormat = '\tnodeB_{index} [label = "Node B{index}\\noffset = {offset}\\nvalue2 = {value2}"]\n'
    edgeFormat = '\tnodeB_{index} -> node{childtype}_{childindex}\n'
    
    def __init__( self, file, index ):
        self.mins = struct.unpack( "3f", file.read( 12 ) )
        self.maxs = struct.unpack( "3f", file.read( 12 ) )
        self.offset, self.value2 = struct.unpack( "2I", file.read( 8 ) )
        self.index = index

    def ToDot( self ):
        childIndex = self.offset // 32
        childType = 'A' if self.value2 != 0 else 'B'
        return (self.nodeFormat + self.edgeFormat).format( index = self.index, offset = self.offset, value2 = self.value2, childindex = childIndex, childtype = childType )

# command line parsing
usage = """Usage: {} <path/to/track_bvh.gen>
Opens the supplied Flatout 2 track_bvh.gen file and creates a graphviz .dot file with the hierarchy from it.""".format( sys.argv[0] )

if len( sys.argv ) != 2:
    error( "Wrong number of arguments! Usage:\n{}".format( usage ) )
    
if sys.argv[1] in { "-h", "--help", "-?" }:
    sys.stdout.write( usage )
    sys.exit( 0 )

# open file
filename = sys.argv[1]
inFile = open( filename, "rb" )

# read header
magicnumber, version, numNodesA = struct.unpack( "3I", inFile.read( 12 ) )

if magicnumber != 0xDEADC0DE:
    error( "Invalid file, does not start with 0xDEADC0DE!" )
if version != 1:
    error( "Invalid file, version is not 1!" )

# read first list
listA = [ NodeA( inFile, i ) for i in range( numNodesA ) ]

# read length of second list
numNodesB, = struct.unpack( "I", inFile.read( 4 ) )

# read second list
listB = [ NodeB( inFile, i ) for i in range( numNodesB ) ]

# should be at eof
if inFile.read() != b'':
    error( "Invalid file, longer than expected!" )

inFile.close()

# write results to filename.dot
outFile = open( filename + ".dot", "w" )
outFile.write( "digraph\n{\n" )
for nodeA in listA:
    outFile.write( nodeA.ToDot() )
for nodeB in listB:
    outFile.write( nodeB.ToDot() )
outFile.write ( "}\n" )
outFile.close()

sys.stdout.write( "Success!" )
