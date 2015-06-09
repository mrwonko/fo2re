import sys
import struct
from PIL import Image

if len(sys.argv) < 2:
    print( "Usage: {} path/to/shadowmap.dat [width height]".format( sys.argv[0] ) )
    sys.exit()

filename = sys.argv[ 1 ]
width = int( sys.argv[ 2 ] ) if len( sys.argv ) > 2 else 256
height = int( sys.argv[ 3 ] ) if len( sys.argv ) > 3 else 256

# shadowmap_w2.dat = raw 32BPP RGBA non-interleaved 256x256
Image.frombytes( "RGBA", ( width, height ), open( filename, "rb" ).read() ).save( open( filename + ".png", "wb" ) )
