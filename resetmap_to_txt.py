import sys, os

if len(sys.argv) < 2:
    print( "Usage: {} path/to/resetmap.4b [width]".format( sys.argv[0] ) )
    sys.exit()

def writeResetmap( file, data, width ):
    for i in range( len(data) // width ):
        file.write( data[ i*width : (i+1)*width ] )
        file.write( b'\n' )

filename = sys.argv[1]

writeResetmap( open( filename+  ".txt", "wb" ), open( filename, "rb" ).read(), int( sys.argv[2] ) if len( sys.argv ) > 2 else 128 )