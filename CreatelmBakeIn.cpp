//--------------------------------------------------------
//   CreatelmBakeIn.cpp v9.5
//   Setup for Geometry Cache Node.

//   Created by Hiroyuki Okubo on 3/10/2006.
//   Copyright 2006 Hiroyuki Okubo, All rights reserved.
//
//      History
//  Aug.8.06:Remove start and end flag to read fram commandinfo.txt
//      Aug.3.06:Added the duplicate mesh line in CreatelmCommand
//---------------------------------------------------------


// include any maya headers we need.

#include "CreatelmBakeIn.h"
#include "lmBakeIn.h"
#include <maya/MSelectionList.h>
#include <maya/MArgList.h>
#include <maya/MArgDatabase.h>
#include <maya/MString.h>
#include <maya/MGlobal.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MTime.h>
#include <maya/MPlug.h>
#include <maya/MDagModifier.h>
#include <maya/MDagPath.h>
#include <maya/MDataBlock.h>
#include <maya/MDataHandle.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnUnitAttribute.h>
#include <maya/MFloatVector.h>
#include <maya/MPxNode.h>
#include <maya/MSelectionList.h>
#include <maya/MItDependencyNodes.h>
#include <maya/MPointArray.h>
#include <maya/MFnMesh.h>
#include <maya/MMatrix.h>
#include <maya/MFnTransform.h>
#include <maya/MFn.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MPlugArray.h>
#include <maya/MDagPathArray.h>
#include <stdio.h>
#include <string>
using namespace std;

#define filepathFlag "-fp"
#define filepathLongFlag "-filepath"
//#define startFlag "-s"
//#define startLongFlag "-start"
//#define endFlag "-e"
//#define endLongFlag "-end"
#define nameFlag "-n"
#define nameLongFlag "-name"
#define modeFlag "-m"
#define modeLongFlag "-mode"
#define findreplaceFlag "-fr"
#define findreplaceLongFlag "-findreplace"
#define byframeFlag "-bf"
#define byframeLongFlag "-byframe"
#define offsetFlag "-of"
#define offsetLongFlag "-offset"

/// the long and short help flags
const char* g_HelpFlag = "-h";
const char* g_HelpFlagLong = "-help";


/// the text printed when the -h/-help flag is specified
const char* g_HelpText = "This will create a simple node connected to the
time attribute";

MObjectArray CreatelmBakeIn::selobj;
MObjectArray CreatelmBakeIn::transobj;
MIntArray CreatelmBakeIn::vexnum;
unsigned int CreatelmBakeIn::startf;
//unsigned int CreatelmBakeIn::endf;
MString CreatelmBakeIn::path;


//---------------------------------------------------------------------------------
//
MSyntax CreatelmBakeIn::newSyntax()
{
        MSyntax syn;

        /// add a simple -h flag
        syn.addFlag( g_HelpFlag, g_HelpFlagLong );
        syn.addFlag(filepathFlag,filepathLongFlag,MSyntax::kString);
        //syn.addFlag(startFlag,startLongFlag,MSyntax::kUnsigned);
        //syn.addFlag(endFlag,endLongFlag,MSyntax::kUnsigned);
        syn.addFlag(nameFlag,nameLongFlag,MSyntax::kString);
        syn.addFlag(modeFlag,modeLongFlag,MSyntax::kBoolean);
        syn.addFlag(findreplaceFlag,findreplaceLongFlag,MSyntax::kString,MSyntax::kString);
        syn.addFlag(byframeFlag,byframeLongFlag,MSyntax::kDouble);
        syn.addFlag(offsetFlag,offsetLongFlag,MSyntax::kDouble);

        return syn;
}

//---------------------------------------------------------------------------------
///
//bool CreatelmBakeIn::isUndoable() const {
//      return true;
//}

//---------------------------------------------------------------------------------
//
void* CreatelmBakeIn::creator() {
        return new CreatelmBakeIn;
}


//---------------------------------------------------------------------------------
//
//MStatus CreatelmBakeIn::redoIt()
MStatus CreatelmBakeIn::doIt(const MArgList& args)
{
        // the return status
        MStatus stat = MS::kSuccess;
        MArgDatabase argData(syntax(),args);
        MSelectionList sl;

        bool mode=0;
        //set each vertex 0
        if(argData.isFlagSet(modeFlag))argData.getFlagArgument(modeFlag,0,mode);
        if(mode){
                MObjectArray selobject;
                MFnDagNode fndag;
                MPlug pntsPlug;
                MGlobal::getActiveSelectionList(sl);
                selobject.setLength(sl.length());
                for(unsigned int i=0;i<sl.length();i++){
                        sl.getDependNode(i,selobject[i]);
                        fndag.setObject(selobject[i]);
                        MString ntype(fndag.typeName());
                        MString transformtype("transform");
                        if(ntype==transformtype){
                                MDagPath transf=MDagPath::getAPathTo(selobject[i]);
                                transf.extendToShape();
                                MFnDagNode shapedag(transf);
                                pntsPlug=shapedag.findPlug("pnts");
                        }
                        else pntsPlug=fndag.findPlug("pnts");
                        for(unsigned int j=0;j<pntsPlug.numChildren();j++){
                                MPlug pntsPlugElem=pntsPlug.child(j);
                                pntsPlugElem.setValue(0);
                                //MGlobal::displayInfo("test");
                        }
                }
                return stat;
        }

        char temp[256];
        double byframe,offset;
        MString directlypath,nodename,findname,replacename;
        unsigned int shapenum=0;

        //Set byframe
        if(argData.isFlagSet(byframeFlag)){
                argData.getFlagArgument(byframeFlag,0,byframe);
        }
        else byframe=1;

        //Set offset
        if(argData.isFlagSet(offsetFlag)){
                argData.getFlagArgument(offsetFlag,0,offset);
        }
        else offset=0;

        //Set Replace Name
        if(argData.isFlagSet(findreplaceFlag)){
                argData.getFlagArgument(findreplaceFlag,0,findname);
                argData.getFlagArgument(findreplaceFlag,1,replacename);
        }

        //Set Node Name
        if(argData.isFlagSet(nameFlag)){
                argData.getFlagArgument(nameFlag,0,nodename);
        }

        //Set Cash FilePath
        if(argData.isFlagSet(filepathFlag)){
                argData.getFlagArgument(filepathFlag,0,directlypath);
        }
        else{
                //directlypath="/private/Network/Servers/sv-amul/Volumes/sv-amulUsers/Users/hiro/temp";
        }
        MString fpath(directlypath);
        if(!fpath.length()){
                MGlobal::displayError("Specify the Cashe Directoly");
                return MS::kFailure;
        }
        path=fpath;
        fpath+="/commandinfo.txt";
        //MGlobal::displayInfo(fpath);

        //Get the BakeIn information from commandinfo.txt
        const char *cominfofile=fpath.asChar();
        FILE* infofp = fopen(cominfofile,"rb");
        fscanf(infofp,"%s",temp);

        /*bool errorf=0;
        if(argData.isFlagSet(startFlag))argData.getFlagArgument(startFlag,0,startframe);
        else errorf=1;
        if(argData.isFlagSet(endFlag))argData.getFlagArgument(endFlag,0,endframe);
        else errorf=1;
        if(errorf){
                MGlobal::displayError("Set Frame Range");
                return MS::kFailure;
        }*/
        fscanf(infofp,"%i",&startf);
        //endf=endframe;
        //Get the shape number which is going to cashed
        fscanf(infofp,"%i",&shapenum);

        //Get shape info and set the vertex number for each shape
        MStringArray shapeArray,nameArray;
        MIntArray refnodeflag;

        shapeArray.setLength(shapenum);
        nameArray.setLength(shapenum);
        selobj.setLength(shapenum);
        transobj.setLength(shapenum);
        vexnum.setLength(shapenum);
        refnodeflag.setLength(shapenum);

        for(unsigned int i=0;i<shapenum;i++){
                refnodeflag[i]=0;
                char trans[256];
                fscanf(infofp,"%s",trans);
                string buf1,buf2;
                string temptrans=trans;
                MString mtrans(trans),transname;
                if(argData.isFlagSet(findreplaceFlag)){
                        buf1=findname.asChar();
                        buf2=replacename.asChar();
                        unsigned int pos=temptrans.find(buf1,0);
                        unsigned int len=buf1.length();
                        temptrans.replace(pos,len,buf2);
                        transname=temptrans.c_str();
                }
                else transname=trans;
                MGlobal::displayInfo(transname);
                stat=MGlobal::getSelectionListByName(transname,sl);
                nameArray.set(mtrans,i);

                MFnDependencyNode depNodeRef;
                if(MS::kSuccess != stat) {
                        //MGlobal::displayError("Could not get depend node from selection list");
                        //return stat;
                        //transobj[i]=depNodeRef.create(MFn::kMesh,&stat);
                        //selobj.set(transobj[i],i);
                        shapeArray.set("",i);
                        unsigned int tempv=0;
                        fscanf(infofp,"%u",&tempv);             //dummy read
                        vexnum[i]=tempv;
                        refnodeflag[i]=1;
                        //MString temp;
                        //MGlobal::displayInfo("error");
                        //temp.set(tempv);
                        //MGlobal::displayInfo(temp);
                }
                else {
                        //MGlobal::displayInfo("success");
                        unsigned nMatches = sl.length();
                        //MDagPath component;
                        /*for (unsigned int index=0;index<nMatches;index++) {
                                sl.getDependNode(index, selobj[i]);
                                break;
                        }*/
                        for (unsigned int index=0;index<nMatches;index++) {
                                sl.getDependNode(index, transobj[i]);
                                break;
                        }
                        sl.clear();
                        MFnDagNode transdag(transobj[i]);
                        MDagPath dagp=MDagPath::getAPathTo(transobj[i]);
                        dagp.extendToShape();
                        MFnDagNode shapedag(dagp);
                        selobj.set(dagp.node(),i);
                        MString shape=dagp.partialPathName();
                        shapeArray.set(shape,i);

                        //MFnMesh fnmesh(selobj[i]);
                        //vexnum[i]=fnmesh.numVertices(&stat);
                        unsigned int tempv=0;
                        fscanf(infofp,"%u",&tempv);
                        vexnum[i]=tempv;
                        //MString vn;
                        //vn.set(vexnum[i]);
                        //MGlobal::displayInfo(vn);
                }
        }
        fclose(infofp);

        MFnDependencyNode depNodeFn;

        //create BakeInNode here
        depNodeFn.create(lmBakeIn::m_TypeId,nodename);

        MString m_Name = depNodeFn.name();

        MDGModifier dgModifier;

        //Set up for the BakeIn Node
        MPlug offsetplug,byframeplug,worldMeshPlug,inMeshPlug,
inMeshesPlug,filepathplug,
        startfplug,numvertplug,initflagplug,OutMeshesPlug,meshesplug;

        offsetplug=depNodeFn.findPlug("offset");
        byframeplug=depNodeFn.findPlug("byframe");
        inMeshesPlug=depNodeFn.findPlug("inmeshes");
        OutMeshesPlug=depNodeFn.findPlug("outmeshes");
        filepathplug=depNodeFn.findPlug("filepath");
        startfplug=depNodeFn.findPlug("startf");
        //endfplug=depNodeFn.findPlug("endf");
        numvertplug=depNodeFn.findPlug("numvert");
        meshesplug=depNodeFn.findPlug("meshes");

        //Set start and end frame
        filepathplug.setValue(path);
        startfplug.setValue((int) startf);
        //endfplug.setValue((int) endf);
        byframeplug.setValue(byframe);
        offsetplug.setValue(offset);

        MObjectArray newdFnTransformObj;
        newdFnTransformObj.setLength(shapenum);
        for(unsigned int i=0;i<shapenum;i++){
                // Set Vertices Number
                MPlug plugElement=numvertplug.elementByLogicalIndex(i);
                MPlug meshesplugElem=meshesplug.elementByLogicalIndex(i);
                plugElement.setValue(vexnum[i]);
                meshesplugElem.setValue(nameArray[i]);
                if(refnodeflag[i]){
                        //skip the missing nodes....
                        //Get Output Mesh Plug
                        //Get Output Mesh Plug
                        MPlug plugElement1=OutMeshesPlug.elementByLogicalIndex(i);
                        //Get Input Mesh Plug
                        MPlug plugElement2=inMeshesPlug.elementByLogicalIndex(i);

                        dgModifier.connect(plugElement1,plugElement2);
                        dgModifier.disconnect(plugElement1,plugElement2);
                }
                else{
                        //Get Output Mesh Plug
                        MPlug plugElement1=OutMeshesPlug.elementByLogicalIndex(i);
                        //Get Input Mesh Plug
                        MPlug plugElement2=inMeshesPlug.elementByLogicalIndex(i);

                        MFnDependencyNode dFn,dpdFn;
                        dFn.setObject(selobj[i]);

                        MFnDagNode dFndg;
                        // Duplicate Mesh for input
                        dFndg.setObject(selobj[i]);
                        dFndg.setObject(dFndg.parent(0));
                        newdFnTransformObj[i]=dFndg.duplicate(false,false);

                        MFnDagNode newdFndg;
                        //MGlobal::displayInfo(newdFndg.setName(cptransfname));

                        newdFndg.setObject(newdFnTransformObj[i]);

                        MDagPath dagp=MDagPath::getAPathTo(newdFnTransformObj[i]);
                        dagp.extendToShape();
                        MFnDagNode shapedag(dagp);

                        //newdFndg.setObject(newdFndg.child(0));
                        //set each vertex 0
                        dFndg.setObject(dFndg.child(0));
                        MPlug pntsPlug=dFndg.findPlug("pnts");
                        for(unsigned int j=0;j<pntsPlug.numChildren();j++){
                                MPlug pntsPlugElem=pntsPlug.child(j);
                                pntsPlugElem.setValue(0);
                        }

                        worldMeshPlug=shapedag.findPlug("worldMesh");
                        MPlug visibPlug=shapedag.findPlug("visibility");
                        //MGlobal::displayInfo(newdFndg.name());
                        //shapeArray[i]=newdFndg.name();
                        visibPlug.setValue(0);
                        //World Mesh Plug
                        MPlug worldMeshPlugElement=worldMeshPlug.elementByLogicalIndex(0);

                        //MGlobal::displayInfo(dFn.name());

                        //Get inMesh Plug
                        MPlug inMeshPlug;
                        inMeshPlug = dFn.findPlug("inMesh");

                        if(inMeshPlug.isConnected()){
                                MPlugArray temp;
                                inMeshPlug.connectedTo(temp,true,false);
                                for(unsigned int
index=0;index<temp.length();index++)dgModifier.disconnect(temp[index],inMeshPlug);
                        }
                        dgModifier.connect(plugElement1,inMeshPlug);
                        dgModifier.connect(worldMeshPlugElement,plugElement2);
                }
        }

        // select the time node
        stat = MGlobal::selectByName( "time1", MGlobal::kReplaceList );

        if(MS::kSuccess != stat) {
                MGlobal::displayError("Could not select");
                return stat;
        }

        // get the selection list containing the time node
        MSelectionList list;
        stat = MGlobal::getActiveSelectionList(list);
        if(MS::kSuccess != stat) {
                MGlobal::displayError("Could not get select list");
                return stat;
        }

        // retrieve the MObject for the time1 node
        MObject oTime;
        stat = list.getDependNode(0,oTime);
        if(MS::kSuccess != stat) {
                MGlobal::displayError("Could not get depend node from selection list");
                return stat;
        }


        // attach a function set to time1
        MFnDependencyNode fnTime( oTime );

        // get a plug to it's outTime attribute
        MPlug outTime = fnTime.findPlug("outTime",&stat);
        if(stat != MS::kSuccess) {
                MGlobal::displayError("Could not get time1.outTime attr");
                return stat;
        }

        // get a plug to the simpleNode.time attribute
        MPlug inTime = depNodeFn.findPlug("time");
        if(stat != MS::kSuccess) {
                MGlobal::displayError("Could not get node.time attr");
                return stat;
        }

        dgModifier.connect( outTime, inTime );

        // perform the changes to the DAG

        stat=dgModifier.doIt();
        MDagModifier delModifier;
        for(unsigned int
i=0;i<shapenum;i++)delModifier.deleteNode(newdFnTransformObj[i]);
        stat=delModifier.doIt();

        // set this string as the result from the function
        shapeArray.append(m_Name);
        setResult(shapeArray);
        return stat;
}
