//--------------------------------------------------------
//   lmBakeOut.cpp v11.7
//   Geometry Cache Write Node for Maya
//   Created by Hiroyuki Okubo on 3/10/2006.
//   Copyright 2006 Hiroyuki Okubo, All rights reserved.
//

//---------------------------------------------------------

#ifdef WIN32
	#define NT_PLUGIN
#endif

// include any maya headers we need.
// include any maya headers we need.

#include <maya/MGlobal.h>
#include <maya/MMatrix.h>
#include <maya/MFnMesh.h>
#include <maya/MFnTransform.h>
#include <maya/MItDependencyNodes.h>
#include <maya/MPointArray.h>
#include <maya/MComputation.h>
#include <maya/MItSelectionList.h>
#include <maya/MSelectionList.h>
#include <maya/MDagPath.h>
#include <maya/MObject.h>
#include <maya/MSyntax.h>
#include <maya/MArgDatabase.h>
#include <stdio.h>
#include "lmBakeOut.h"


#define DEBUG 1
#define startFlag "-s"
#define startLongFlag "-start"
#define endFlag "-e"
#define endLongFlag "-end"
#define filepathFlag "-fp"
#define filepathLongFlag "-filepath"

//---------------------------------------------------------------------------------
// Use a helper macro to register a command with Maya.  This creates and
// registers a command that does not support undo or redo.  The
// created class derives off of MPxCommand.
//
//DeclareSimpleCommand( lmBakeOut, "luma geometry chash out command", "1.0");

void* lmBakeOut::creator() {
    return new lmBakeOut;
}

MSyntax lmBakeOut::newSyntax(){
        MSyntax syntax;
        syntax.addFlag(startFlag,startLongFlag,MSyntax::kLong);
        syntax.addFlag(endFlag,endLongFlag,MSyntax::kLong);
        syntax.addFlag(filepathFlag,filepathLongFlag,MSyntax::kString);
        return syntax;
}





MStatus lmBakeOut::doIt( const MArgList& args )

{
        MStatus stat;

        unsigned int startframe=0,endframe=10;
        MString filepath;
        // create an MComputation class for heavy calculation type things
        MComputation Interupter;

        MArgDatabase argData(syntax(),args);

        //Set Cash FilePath
        if(argData.isFlagSet(filepathFlag)){
                argData.getFlagArgument(filepathFlag,0,filepath);
        }
        else{
                //FILENAME = MFileIO::currentFile();
        }

        //Set StartFrame
        if(argData.isFlagSet(startFlag)){
                argData.getFlagArgument(startFlag,0,startframe);
        }
        //Set EndFrame
        if(argData.isFlagSet(endFlag)){
                argData.getFlagArgument(endFlag,0,endframe);
        }

        // set the start of the heavy computation
        Interupter.beginComputation();
        //printf("computing\n");
#ifdef DEBUG
       unsigned int vnum=0;
#endif
        char filename[256];
        MStringArray lis;
        MObjectArray selobj;
        MSelectionList listarray;

        //MGlobal::selectCommand(sl,MGlobal::kReplaceList);
        stat = MGlobal::getActiveSelectionList(listarray);
        MItSelectionList    iter(listarray);

        if(MS::kSuccess != stat) {
                MGlobal::displayError("Could not get select list");
                return stat;
        }

        unsigned int selnum=listarray.length();
        selobj.setLength(selnum);

        if(MS::kSuccess != stat) {
                MGlobal::displayError("Could not get depend node from selection list");
                return stat;
        }

        MString cominfpath;
        MString cashefile;
        cominfpath=filepath+("/commandinfo.txt");
        cashefile=filepath+("cashefile.%4d.dat");
        //initialfile="/private/Network/Servers/sv-amul/Volumes/sv-amulUsers/Users/hiro/temp/commandinfo.txt";

        //open the ascii file
        //save command info
        FILE* initfp = fopen(cominfpath.asChar(),"w");

        //filepath
        fprintf(initfp,"%s\n",filepath.asChar());
        //startframe
        fprintf(initfp,"%d\n",startframe);
        //endframe
        fprintf(initfp,"%d\n",endframe);
        //number of object to bake
        fprintf(initfp,"%d\n",selnum);
        for(unsigned int i=0;i<selnum;i++){

                stat = listarray.getDependNode(i,selobj[i]);
                MFnMesh fnmesh(selobj[i]);
                fprintf(initfp,"%s\n",fnmesh.name().asChar());
                //MGlobal::displayInfo(fnmesh.name());
        }
        fclose(initfp);

        const char* chcashefile=cashefile.asChar();
        for(unsigned int nu=startframe;nu<=endframe;++nu) {

                        /// generate a filename with frame padding
                        sprintf(filename,chcashefile,nu);

                        // replace spaces with zeros
                        for(unsigned int i=0;filename[i];++i) {
                                if(filename[i] == ' ') {
                                        filename[i] = '0';
                                }
                        }
                        /// open the binary file
#ifdef DEBUG
						FILE* fp = fopen(filename,"w");
#else
                        FILE* fp = fopen(filename,"wb");
#endif
                        // loop through all mesh nodes
                        //MItDependencyNodes it(MFn::kMesh);
                        //while( !it.isDone() )
                        //{
                                // attach a function set to the mesh
                                //MFnMesh fnMesh( it.item() );

                        // sets the current frame
                        stat = MGlobal::getActiveSelectionList(listarray);
                        MItSelectionList    iter(listarray);

                        MGlobal::viewFrame(nu);

                        for ( ; !iter.isDone(); iter.next() )
                        //for(int i=0;i<listarray.length();i++);
                        {

                                MObject component;
                                MDagPath dpath;
                                iter.getDependNode(component);
                                //listarray.getDependNode(i,component);
                                //listarray.getDagPath(i,dpath,component);
                                iter.getDagPath(dpath,component);

                                //MFnMesh fnMesh(component);
                                MFnMesh fnMesh(dpath);
                                //fnMesh.setObject(dpath);

#ifdef DEBUG
                                vnum=fnMesh.numVertices(&stat);
                                printf("%i\n",vnum);
#else

#endif
                                // retrieve the mesh points (local space)
                                MPointArray points;
								fnMesh.getPoints(points,MSpace::kWorld);
                                MString objname=fnMesh.name();
#ifdef DEBUG
                                MGlobal::displayInfo(objname);
                                printf("%d\n",fnMesh.parentCount());
#endif
                                //printf(objname.asChar());
                                //printf("\n");

                                        for(unsigned int k=0;k!=points.length() ; ++k)
                                        {
                                                // transform the points from local space to world space
                                                //
                                                //MPoint OutPoint = points[k] * parent_matrix;
                                                MPoint OutPoint=points[k];

                                                // write 3D vector to file
                                                float data[3];
                                                data[0] = OutPoint.x;
                                                data[1] = OutPoint.y;
                                                data[2] = OutPoint.z;
#ifdef DEBUG
                                                //fprintf(fp,"%i %f %f %f\n",k,data[0],data[1],data[2]);
												fprintf(fp,"%f %f %f\n",data[0],data[1],data[2]);
#else
                                                fwrite(data,sizeof(float),3,fp);
#endif
                                        }

                        }
                        // close the file
                        fclose(fp);
                }

        //finished our computation
        Interupter.endComputation();

        return MS::kSuccess;
}


