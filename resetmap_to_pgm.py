import sys
import struct

if len(sys.argv) < 2:
    print( "Usage: {} path/to/resetmap.4b [width]".format( sys.argv[0] ) )
    sys.exit()

filename = sys.argv[ 1 ]

nibbles = []
for byte in open( filename, "rb" ).read():
    nibbles.append( byte & 0xF )
    nibbles.append( byte >> 4 )

width = int( sys.argv[ 2 ] ) if len( sys.argv ) > 2 else 256
height = len( nibbles ) // width

with open( filename + ".pgm", "w" ) as f:
    f.write( "P2\n{w} {h}\n{max}\n".format( w = width, h = height, max = 0xF ) )
    for y in range( height ):
        for x in range( width ):
            f.write( "{} ".format( nibbles[ y * width + x ] ) )
        f.write( "\n" )
