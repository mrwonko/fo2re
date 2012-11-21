From GulBroz' FO2trackData.cpp:

Line 7682, in function cTrack::read:

	// Read header
	header.read( h );
	
	// Read materials
	materialList.read( h );
	
	// Read streams
	streamList.read( h );
	
	// Read surfaces
	surfaceList.read( h, streamList );
	
	// Read surface centers
	surfcenterList.read( h );
	
	// Read unknown2s
	unknown2List.read( h );
	
	// Read unknown3s
	unknown3List.read( h );
	
	// Read unknown4s
	unknown4List.read( h );
	
	// Read unknown5s
	unknown5List.read( h );
	
	// Read models
	modelList.read( h );
	
	// Read objects
	objectList.read( h );
	
	// Read boundingBoxes
	boundingBoxList.read( h );
	
	// Read meshToBBoxs
	meshToBBoxList.read( h );
	
	// Read meshes
	meshList.read( h );