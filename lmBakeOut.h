//--------------------------------------------------------
//   lmBakeOut.h v9.5
//
//   Created by Hiroyuki Okubo on 3/10/2006.
//   Copyright 2006 Hiroyuki Okubo All rights reserved.
//
//---------------------------------------------------------


	#ifdef WIN32
		#define NT_PLUGIN
		#pragma once
	#endif

	// include any maya headers we need.
	#include <maya/MPxCommand.h>
	#include <maya/MGlobal.h>
	#include <maya/MFnMesh.h>
	#include <maya/MFnTransform.h>
	#include <maya/MFnPointLight.h>
	#include <maya/MItDependencyNodes.h>
	#include <maya/MPointArray.h>
#include<maya/MArgList.h>

class lmBakeOut : public MPxCommand
{

public:

    MStatus        doIt( const MArgList& args );
    static void*   creator();
        static MSyntax newSyntax();

};


