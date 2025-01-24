//--------------------------------------------------------
//   PluginMain.cpp v9.5
//   Setup for Geometry Cache Node.

//   Created by Hiroyuki Okubo on 3/10/2006.
//   Copyright 2006 Hiroyuki Okubo, All rights reserved.
//--------------------------------------------------------

#include "lmBakeOut.h"
#include <maya/MFnPlugin.h>

// required to link the libraries
#ifdef WIN32
	#pragma comment(lib,"Foundation.lib")
	#pragma comment(lib,"OpenMaya.lib")
	#pragma comment(lib,"OpenMayaFx.lib")
	#pragma comment(lib,"OpenMayaUi.lib")
	#pragma comment(lib,"Image.lib")
	#pragma comment(lib,"OpenMayaAnim.lib")
#endif

// under WIN32 we have to 'export' these functions so that they are visible
// inside the dll. If maya can't see the functions, it can't load the plugin!
#ifdef WIN32
	#define EXPORT __declspec(dllexport)
#else
	#define EXPORT
#endif


/// \brief	This function is called once when our plugin is loaded. We can use
///			it to register any custom nodes, mel functions etc.
/// \param	obj	-	a handle to the loaded plugin
/// \return	an MStatus error code
///
EXPORT MStatus initializePlugin( MObject obj ) {
    MFnPlugin plugin( obj, "Alias", "1.0", "Any" );
    plugin.registerCommand( "lmBakeOut",
lmBakeOut::creator,lmBakeOut::newSyntax);
    return MS::kSuccess;
}

EXPORT MStatus uninitializePlugin( MObject obj ) {
    MFnPlugin plugin( obj );
    plugin.deregisterCommand( "lmBakeOut" );

    return MS::kSuccess;
}


