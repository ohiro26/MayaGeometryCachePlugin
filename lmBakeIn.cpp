//--------------------------------------------------------
//   lmBakeIn.cpp v11.7
//   Geometry Cache Read Node for Maya
//   Created by Hiroyuki Okubo on 3/10/2006.
//   Copyright 2006 Hiroyuki Okubo, All rights reserved.
//

//---------------------------------------------------------

#include "lmBakeIn.h"
#include "CreatelmBakeIn.h"
#include <math.h>
#include <stdio.h>
#include <maya/MSelectionList.h>
#include <maya/MItSelectionList.h>
#include <maya/MDagPath.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MArrayDataHandle.h>
#include <maya/MFnCompoundAttribute.h>
#include <maya/MFnMeshData.h>
#include <maya/MItGeometry.h>
#include <maya/MPlugArray.h>
#include <maya/MSceneMessage.h>
#include <maya/MMessage.h>
#include <maya/MEventMessage.h>
#include <string>
using namespace std;

#define
MCHECK_ERROR(stat)              if(stat==MStatus::kSuccess)MGlobal::displayInfo("Success");\
                                        else if(stat==MStatus::kFailure)MGlobal::displayInfo("Failure");\
                                        else MGlobal::displayInfo("Unknown");

#define MDISP(num)                      temp.set(num);\
                                        MGlobal::displayInfo(temp);

MTypeId lmBakeIn::m_TypeId( 0x00333 );
MString lmBakeIn::m_TypeName( "lmBakeIn" );

/// an connection made to the input time value
MObject lmBakeIn::m_aTime;

MObject lmBakeIn::m_aInMeshes;
MObject lmBakeIn::m_aStartf;
//MObject lmBakeIn::m_aEndf;
MObject lmBakeIn::m_aFilePath;
MObject lmBakeIn::m_aVnum;
MObject lmBakeIn::m_aOutMeshes;
MObject lmBakeIn::m_aByframe;
MObject lmBakeIn::m_aOffset;
MObject lmBakeIn::m_aInitflag;
MIntArray lmBakeIn::callbackIDList;
MObject lmBakeIn::m_aSkipFlag;
MObject lmBakeIn::m_aObjoder;
MObject lmBakeIn::m_aIternalFlag;
MObject lmBakeIn::m_aMeshes;
MObject lmBakeIn::m_aNewVnum;
MObject lmBakeIn::m_aObjNum;
MObject lmBakeIn::m_aFindReplace;
MObjectArray lmBakeIn::thisNodeObj;


int fread_be(float* aa, int size, int nb, FILE* fich) {

     assert(size == 4) ;
     // Determines whether the default storage is big endian
     //  or large endians
     int itest = 1 ;
     bool little_endian = ( *( (char*) &itest ) == 1) ;

     if (little_endian) {
        //MGlobal::displayInfo("little_endian");
         int size_tot = 4 * nb;
         char* bytes_big = new char[size_tot];
         int nr = fread(bytes_big, 1, size_tot, fich);
         char* pbig =  bytes_big;
         char* plit = (char*) aa;
         for (int j=0; j< nb; j++) {
             for (int i=0; i<4; i++) {
                 plit[i] = pbig[3-i] ;
             }
             plit +=4 ;     // next item
             pbig +=4 ;
         }
         delete [] bytes_big ;
         return nr /4 ;
     }
    else {  // Big endian case: nothing to do:
        return fread(aa, size, nb, fich) ;
    }
}

void lmBakeIn::nodeAdded(MObject& node,void* np){

        MStatus stat;
        MObject thisNode=node;

        thisNodeObj.append(node);
        //MObject thisNode=*((MObject*)np);

        MGlobal::displayInfo("callbackfunc");

        unsigned int
cID=MSceneMessage::addCallback(MSceneMessage::kBeforeSave,lmBakeIn::sceneSaved,&(thisNodeObj[thisNodeObj.length()-1]),NULL);
        if (!callbackIDList.append( cID ))
        {
         MStatus().perror("Cannot add ID to list");
         }
        cID=MEventMessage::addEventCallback("SceneOpened",lmBakeIn::sceneOpened,&(thisNodeObj[thisNodeObj.length()-1]),NULL);
        if (!callbackIDList.append( cID ))
        {
         MStatus().perror("Cannot add ID to list");
         }
        MPlug initplug;
        MFnDependencyNode dp(thisNode);
        MString fpname("initializeflag");
        initplug=dp.findPlug((MString )"initializeflag",&stat);
        MGlobal::displayInfo("nodeadd="+dp.name());

}

void lmBakeIn::sceneOpened(void* np){

        int startf,shapenum;
        MGlobal::displayInfo("SceneOpened");
        MStatus stat;
        MObject thisNode=*((MObject*)np);
        MFnDependencyNode dp(thisNode);
        MGlobal::displayInfo(dp.name());
        MString lpath;

        MSelectionList sl;
        MPlug outMeshesplug(thisNode,m_aOutMeshes);
        MPlug inMeshesplug(thisNode,m_aInMeshes);
        MPlug vnumplug(thisNode,m_aVnum);
        MPlug initplug(thisNode,m_aInitflag);
        MPlug objorderplug(thisNode,m_aObjoder);
        MPlug meshesplug(thisNode,m_aMeshes);
        MPlug newvnumplug(thisNode,m_aNewVnum);
        MPlug skipflagplug(thisNode,m_aSkipFlag);
        MPlug objnumplug(thisNode,m_aObjNum);
        MPlug findreplaceplug(thisNode,m_aFindReplace);

        MPlug filepathplug(thisNode,m_aFilePath);
        filepathplug.getValue(lpath);

        MIntArray connflag;
        MPlugArray conArray;

        MStringArray transobjs;
        MIntArray objorder;
        MIntArray newVertNums;
        MIntArray skipflags;
        lpath+="/commandinfo.txt";
        //MGlobal::displayInfo(fpath);

        //Get the BakeIn information from commandinfo.txt
        const char *cominfofile=lpath.asChar();
        FILE* infofp = fopen(cominfofile,"rb");
        char tempfp[256];
        fscanf(infofp,"%s",tempfp);
        fscanf(infofp,"%i",&startf);
        //Get the shape number which is going to be cashed
        fscanf(infofp,"%i",&shapenum);

        skipflags.setLength(shapenum);
        newVertNums.setLength(shapenum);
        objorder.setLength(shapenum);


        MString transobj,findname,replacename;
        MPlug findreplaceplugElem=findreplaceplug.elementByLogicalIndex(0);
        findreplaceplugElem.getValue(findname);

        findreplaceplugElem=findreplaceplug.elementByLogicalIndex(1);
        findreplaceplugElem.getValue(replacename);

        int tempv=0;
        MGlobal::displayInfo("startinit");
        MString shapestr;
        shapestr.set(shapenum);
        MGlobal::displayInfo(shapestr);
        MGlobal::displayInfo("endinit");
        objnumplug.setValue(shapenum);
        for(int i=0;i<shapenum;i++){

                char trans[256];
                fscanf(infofp,"%s",trans);
                string buf1,buf2;
                string temptrans=trans;
                MString mtrans(trans),transname;
                if(!findname.length()||!replacename.length()){
                        buf1=findname.asChar();
                        buf2=replacename.asChar();
                        unsigned int pos=temptrans.find(buf1,0);
                        unsigned int len=buf1.length();
                        temptrans.replace(pos,len,buf2);
                        transname=temptrans.c_str();
                }
                else transname=trans;
                fscanf(infofp,"%u",&tempv);
                MString tempvstr;
                tempvstr.set(tempv);
                MGlobal::displayInfo("VtxNum="+tempvstr);
                stat=MGlobal::getSelectionListByName(transname,sl);
                MGlobal::displayInfo("select Object "+transname);
                if(MS::kSuccess == stat){
                        MObject transMObj,shapeMObj, newdFnTransformObj;
                        sl.getDependNode(0, transMObj);

                        sl.clear();
                        MFnDagNode transdag(transMObj);
                        MDagPath dagp=MDagPath::getAPathTo(transMObj);
                        dagp.extendToShape();
                        MFnDagNode shapedag(dagp);
                        shapeMObj=dagp.node();
                        //MString shape=dagp.partialPathName();
                        //shapeArray.set(shape,i);

                        transobj.set(trans);
                        MGlobal::displayInfo("objname="+transobj);
                        transobjs.append(transobj);

                        MPlug skipflagplugElem=skipflagplug.elementByLogicalIndex(i);
                        skipflagplugElem.getValue(skipflags[i]);
                        MDagModifier dgModifier;
                        if(skipflags[i]){
                                MPlug
vnumplugElem=vnumplug.elementByLogicalIndex(outMeshesplug.numElements());
                                vnumplugElem.setValue(tempv);

                                MPlug
meshesplugElem=meshesplug.elementByLogicalIndex(outMeshesplug.numElements());
                                meshesplugElem.setValue(transobj);

                                MFnDependencyNode dFn,dpdFn;
                                dFn.setObject(shapeMObj);

                                MFnDagNode dFndg;
                                // Duplicate Mesh for input
                                dFndg.setObject(shapeMObj);
                                dFndg.setObject(dFndg.parent(0));
                                newdFnTransformObj=dFndg.duplicate(false,false);

                                MFnDagNode newdFndg;

                                //MGlobal::displayInfo(dFn.name());
                                newdFndg.setObject(newdFnTransformObj);

                                MDagPath dagp=MDagPath::getAPathTo(newdFnTransformObj);
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

                                MPlug worldMeshPlug=shapedag.findPlug("worldMesh");
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

                                //Get Output Mesh Plug
                                MPlug
plugElement1=outMeshesplug.elementByLogicalIndex(outMeshesplug.numElements());
                                //Get Input Mesh Plug
                                MPlug
plugElement2=inMeshesplug.elementByLogicalIndex(outMeshesplug.numElements());

                                dgModifier.connect(plugElement1,inMeshPlug);
                                dgModifier.connect(worldMeshPlugElement,plugElement2);
                                stat=dgModifier.doIt();
                                initplug.setValue(0);
                                MDagModifier delModifier;
                                delModifier.deleteNode(newdFnTransformObj);
                                stat=delModifier.doIt();
                        }
                }
        }
        fclose(infofp);




        /*for(unsigned int i=0;i<vnumplug.numElements();i++){
                //MPlug plugElement=vnumplug.elementByLogicalIndex(i);
                //plugElement.getValue(cpvexnum[i]);
                MStatus stat;
                MPlug outmeshesplugElem=outMeshesplug.elementByLogicalIndex(i);
                bool conf=outmeshesplugElem.connectedTo(conArray,0,1);
                MObject destobj=conArray[0].node();
                MFnDagNode destdagNode(destobj);
                MObject destransf=destdagNode.parent(0);
                MFnDependencyNode destransfDep(destransf);
                //MGlobal::displayInfo(destransfDep.name());
                //MPlug visPlug=destransfDep.findPlug("visibility");
        }*/
}

void lmBakeIn::sceneSaved(void* np){
        MGlobal::displayInfo("beforesave");
        MStatus stat;
        MPlug initplug;
        MObject thisNode=*((MObject*)np);
        MFnDependencyNode dp(thisNode);
        MGlobal::displayInfo(dp.name());
        MString fpname("initializeflag");
        initplug=dp.findPlug((MString )"initializeflag",&stat);
        stat=initplug.setValue(0);
}

lmBakeIn::lmBakeIn (void)
{
        //MStatus stat;
        //MObject thisNode=thisMObject();
        //unsigned int
cID=MSceneMessage::addCallback(MSceneMessage::kBeforeSave,lmBakeIn::nodeAdded,&thisNode,NULL);
        //if (!callbackIDList.append( cID ))
        //{
        //      MStatus().perror("Cannot add ID to list");
         //}
        //MGlobal::displayInfo("constructor");
}

lmBakeIn::~lmBakeIn (void)
{
        if(!MSceneMessage::removeCallbacks(lmBakeIn::callbackIDList))
         {
         MStatus().perror("cant deregister callback ...");
         }
        if(!MDGMessage::removeCallbacks(lmBakeIn::callbackIDList))
         {
         MStatus().perror("cant deregister callback ...");
         }
        if(!MEventMessage::removeCallbacks(lmBakeIn::callbackIDList))
         {
         MStatus().perror("cant deregister callback ...");
         }
}

void lmBakeIn::postConstructor(){


        unsigned int
cID=MDGMessage::addNodeAddedCallback(lmBakeIn::nodeAdded,"lmBakeIn",NULL,NULL);
        if (!callbackIDList.append( cID ))
        {
         MStatus().perror("Cannot add ID to list");
         }

}
//-------------------------------------------------------------------------------------------------
//
void* lmBakeIn::creator()
{

        // return a new instance of the node for maya to use
    return new lmBakeIn();
}


float lmBakeIn::splinef(float y0, float y1, float y2, float y3 ,float x){

        float h0,h1,h2,x1,x2;
        h0=h1=h2=1;
        x1=1;
        x2=2;

        float A=(y2-y1)/h1-(y1-y0)/h0;
        float B=(y3-y2)/h2-(y2-y1)/h1;

        float z1=(6*B*h1-12*A*(h1+h2))/(pow(h1,2)-4*(h0+h1)*(h1+h2));
        float z2=(6*A)/h1-(2*z1*(h0+h1))/h1;

        float
s=(z2*pow((x-x1),3)+z1*pow((x2-x),3))/(6*h1)+(y2/h1-h1*z2/6)*(x-x1)+(y1/h1-h1*z1/6)*(x2-x);

        return s;
}
//-------------------------------------------------------------------------------------------------
//
MStatus lmBakeIn::initialize()
{

        MFnTypedAttribute tAttr,tAttr1,tAttr2,tAttr3,tAttr4;

        m_aInMeshes=tAttr.create("inmeshes","im",MFnData::kMesh);

        tAttr.setArray(true);
        //tAttr.setStorable(false);
        //tAttr.setKeyable(false);
        //tAttr.setReadable(true);
        //tAttr.setWritable(true);
        //tAttr.setHidden(false);
        //tAttr.setCached(false);

        m_aFilePath=tAttr1.create("filepath","fp",MFnData::kString);

        tAttr1.setStorable(true);
        tAttr1.setKeyable(false);
        tAttr1.setReadable(true);
        tAttr1.setWritable(true);
        tAttr1.setHidden(false);
        tAttr1.setCached(false);

        m_aOutMeshes=tAttr2.create("outmeshes","om",MFnData::kMesh);

        tAttr2.setArray(true);
        tAttr2.setStorable(false);
        //tAttr2.setKeyable(false);
        tAttr2.setReadable(true);
        tAttr2.setWritable(true);
        //tAttr2.setHidden(false);
        //tAttr2.setCached(false);

        m_aMeshes=tAttr3.create("meshes","ms",MFnData::kString);

        tAttr3.setArray(true);
        tAttr3.setStorable(true);
        tAttr3.setKeyable(false);
        tAttr3.setReadable(true);
        tAttr3.setWritable(true);
        tAttr3.setHidden(true);
        tAttr3.setCached(false);

        m_aFindReplace=tAttr4.create("findreplace","fr",MFnData::kString);

        tAttr4.setArray(true);
        tAttr4.setStorable(true);
        tAttr4.setKeyable(false);
        tAttr4.setReadable(true);
        tAttr4.setWritable(true);
        tAttr4.setHidden(true);
        tAttr4.setCached(false);

        MFnUnitAttribute uAttr;
        // create the input time attribute
        m_aTime =  uAttr.create( "time", "t", MFnUnitAttribute::kTime );

        uAttr.setStorable(false);
        uAttr.setKeyable(false);

        //      The MFnNumericAttribute function set allows us to add numeric types such
as
        //
        //      bool, int, float, double, point, colour etc.
        //
        MFnNumericAttribute
nAttr,nAttr1,nAttr3,nAttr4,nAttr5,nAttr6,nAttr7,nAttr8,nAttr9,nAttr10;

        // create the output attribute
        m_aStartf=nAttr1.create("startf","sf",MFnNumericData::kInt);

        nAttr1.setReadable(true);
        nAttr1.setKeyable(false);
        nAttr1.setWritable(true);
        nAttr1.setHidden(true);

        //m_aEndf=nAttr2.create("endf","ef",MFnNumericData::kInt);

        // make this attribute a read only output value

        //nAttr2.setReadable(true);
        //nAttr2.setKeyable(true);
        //nAttr2.setWritable(true);
        //nAttr2.setHidden(false);

        m_aVnum=nAttr3.create("numvert","nv",MFnNumericData::kInt);

        nAttr3.setArray(true);
        nAttr3.setReadable(true);
        nAttr3.setKeyable(true);
        nAttr3.setWritable(true);
        nAttr3.setHidden(true);

        m_aNewVnum=nAttr10.create("newnumvert","nnv",MFnNumericData::kInt);

        nAttr10.setArray(true);
        nAttr10.setReadable(true);
        nAttr10.setKeyable(true);
        nAttr10.setWritable(true);
        nAttr10.setHidden(true);

        m_aByframe=nAttr4.create("byframe","bf",MFnNumericData::kDouble);

        nAttr4.setArray(false);
        nAttr4.setReadable(true);
        nAttr4.setKeyable(true);
        nAttr4.setWritable(true);
        nAttr4.setHidden(false);

        m_aOffset=nAttr5.create("offset","of",MFnNumericData::kDouble);

        nAttr5.setArray(false);
        nAttr5.setReadable(true);
        nAttr5.setKeyable(true);
        nAttr5.setWritable(true);
        nAttr5.setHidden(false);

        m_aInitflag=nAttr6.create("initializeflag","if",MFnNumericData::kInt);

        nAttr6.setReadable(true);
        nAttr6.setKeyable(true);
        nAttr6.setWritable(true);
        nAttr6.setHidden(true);

        m_aSkipFlag=nAttr7.create("skipflag","sk",MFnNumericData::kInt);
        nAttr7.setArray(true);
        nAttr7.setReadable(true);
        nAttr7.setKeyable(true);
        nAttr7.setWritable(true);
        nAttr7.setHidden(true);

        m_aObjoder=nAttr8.create("objoder","od",MFnNumericData::kInt);
        nAttr8.setArray(true);
        nAttr8.setReadable(true);
        nAttr8.setKeyable(true);
        nAttr8.setWritable(true);
        nAttr8.setHidden(true);

        m_aObjNum=nAttr10.create("objNum","on",MFnNumericData::kInt);
        nAttr10.setArray(false);
        nAttr10.setReadable(true);
        nAttr10.setKeyable(true);
        nAttr10.setWritable(true);
        nAttr10.setHidden(true);

        // add the attributes to the node
        addAttribute(m_aFindReplace);
        addAttribute(m_aObjNum);
        addAttribute(m_aNewVnum);
        addAttribute(m_aMeshes);
        addAttribute(m_aObjoder);
        addAttribute(m_aSkipFlag);
        addAttribute(m_aInitflag);
        addAttribute(m_aOffset);
        addAttribute(m_aByframe);
        addAttribute(m_aStartf);
        //addAttribute(m_aEndf);
        addAttribute(m_aFilePath);
        addAttribute(m_aVnum);
        addAttribute(m_aTime);
        addAttribute(m_aInMeshes);
        addAttribute(m_aOutMeshes);

        // set the attribute dependency. the output attribute will now be evaluated
whenever
        // the time input changes.
        //
        attributeAffects(m_aTime,  m_aOutMeshes);
        MGlobal::displayInfo("lmBakeIn v11.7");

        return MS::kSuccess;
}


//-------------------------------------------------------------------------------------------------
//
//
MStatus lmBakeIn::compute (const MPlug& plug, MDataBlock& data)
{
        //MGlobal::displayInfo("testA");
        MStatus stat;
        MString temp;
        //int skcnt=0;

        if( plug ==m_aOutMeshes ){
                int lstartf;//,lendf;
                double byframenum,offset;
                 // get the input time handle
         MDataHandle timeData = data.inputValue(m_aTime);
         // get the output handle
                MArrayDataHandle inmeshes=data.inputArrayValue(m_aInMeshes);
                MArrayDataHandle outmeshes=data.outputArrayValue(m_aOutMeshes);

                MString infopath,lpath,lpath1;

                lpath.clear();
                lpath1.clear();

                MObject thisNode=thisMObject();

                MPlug offsetplug(thisNode,m_aOffset);
                MPlug byframeplug(thisNode,m_aByframe);
                MPlug startfplug(thisNode,m_aStartf);
                //MPlug endfplug(thisNode,m_aEndf);
                MPlug filepathplug(thisNode,m_aFilePath);

                filepathplug.getValue(lpath);
                infopath=lpath;
                lpath+="/cashefile.%4d.dat";
                lpath1=lpath;
                byframeplug.getValue(byframenum);
                offsetplug.getValue(offset);
                startfplug.getValue(lstartf);
                //endfplug.getValue(lendf);

         // get the input value as a time value
         MTime Time = timeData.asTime();
                //int d=int(Time.as(MTime::kFilm));
                double d=double(Time.as(MTime::kFilm));
                double m_d=byframenum*(d-lstartf)+lstartf-byframenum*offset;
                //d=byframenum*(d-lstartf)+lstartf;
                double dt=fmod(m_d,1);

                MIntArray cpvexnum;
                MIntArray connflag;
                MPlugArray conArray;

                MPlug outmeshesplug(thisNode,m_aOutMeshes);
                MPlug inMeshesplug(thisNode,m_aInMeshes);
                MPlug vnumplug(thisNode,m_aVnum);
                MPlug initplug(thisNode,m_aInitflag);
                MPlug objorderplug(thisNode,m_aObjoder);
                MPlug meshesplug(thisNode,m_aMeshes);
                MPlug newvnumplug(thisNode,m_aNewVnum);
                MPlug skipflagplug(thisNode,m_aSkipFlag);
                MPlug objnumplug(thisNode,m_aObjNum);

                conArray.setLength(1);
                connflag.setLength(vnumplug.numElements());
                cpvexnum.setLength(vnumplug.numElements());

                //MDISP("start");
                for(unsigned int i=0;i<vnumplug.numElements();i++){
                        MPlug plugElement=vnumplug.elementByLogicalIndex(i);
                        plugElement.getValue(cpvexnum[i]);
                        MStatus stat;
                        MPlug outmeshesplugElem=outmeshesplug.elementByLogicalIndex(i);
                        bool conf=outmeshesplugElem.connectedTo(conArray,0,1);

                        MObject destobj=conArray[0].node();
                        MFnDagNode destdagNode(destobj);
                        MObject destransf=destdagNode.parent(0);
                        MFnDependencyNode destransfDep(destransf);

                        MPlug visPlug=destransfDep.findPlug("visibility");
                        bool visib=1;
                        visPlug.getValue(visib);

                        if(!visib||!conf){
                                connflag[i]=0;
                        }
                        else{
                                connflag[i]=1;
                        }
                }

                char filename[128];

                sprintf(filename,lpath.asChar(),(int)m_d);

                // replace spaces with zeros
                for(unsigned int i=0;filename[i];++i) {
                        if(filename[i] == ' ') {
                                filename[i] = '0';
                        }
                }

                FILE* fp = fopen(filename,"rb");
                if(fp==NULL){
                        data.setClean( plug );
                        outmeshes.setAllClean();
                        inmeshes.setAllClean();
                        return MS::kSuccess;
                }

                char filename0[128];
                char filename1[128];
                char filename2[128];

                FILE* fp0=NULL;
                FILE* fp1=NULL;
                FILE* fp2=NULL;
                if(dt!=0){
                        int pframe=int(m_d)+1;
                        sprintf(filename1,lpath1.asChar(),pframe);
                        // replace spaces with zeros
                        for(unsigned int i=0;filename1[i];++i) {
                                if(filename1[i] == ' ') {
                                        filename1[i] = '0';
                                }
                        }

                        fp1 = fopen(filename1,"rb");
                        if(fp1==NULL){
                                fp1=fopen(filename,"rb");
                        }
                        pframe=int(m_d)-1;
                        sprintf(filename0,lpath1.asChar(),pframe);
                        // replace spaces with zeros
                        for(unsigned int i=0;filename0[i];++i) {
                                if(filename0[i] == ' ') {
                                        filename0[i] = '0';
                                }
                        }

                        fp0 = fopen(filename0,"rb");
                        if(fp0==NULL){
                                fp0=fopen(filename,"rb");
                        }
                        pframe=int(m_d)+2;
                        sprintf(filename2,lpath1.asChar(),pframe);
                        // replace spaces with zeros
                        for(unsigned int i=0;filename2[i];++i) {
                                if(filename2[i] == ' ') {
                                        filename2[i] = '0';
                                }
                        }

                        fp2 = fopen(filename2,"rb");
                        if(fp2==NULL){
                                fp2=fopen(filename,"rb");
                        }
                }

                MIntArray debgorder;
                MStringArray transobjs;
                MIntArray objorder;
                MIntArray newVertNums;

                int startf,shapenum;
                transobjs.setLength(0);
                int initflag=0;
                initplug.getValue(initflag);
                if(initflag==0){
                        infopath+="/commandinfo.txt";
                        //MGlobal::displayInfo(fpath);

                        //Get the BakeIn information from commandinfo.txt
                        const char *cominfofile=infopath.asChar();
                        FILE* infofp = fopen(cominfofile,"rb");
                        char tempfp[256];
                        fscanf(infofp,"%s",tempfp);
                        fscanf(infofp,"%i",&startf);
                        //Get the shape number which is going to be cashed
                        fscanf(infofp,"%i",&shapenum);
                        newVertNums.setLength(shapenum);
                        objorder.setLength(shapenum);
                        debgorder.setLength(shapenum);
                        char trans[256];
                        MString transobj;
                        int tempv=0;
                        MGlobal::displayInfo("startinit");
                        MString shapestr;
                        shapestr.set(shapenum);
                        MGlobal::displayInfo(shapestr);
                        MGlobal::displayInfo("endinit");
                        objnumplug.setValue(shapenum);
                        for(int i=0;i<shapenum;i++){
                                fscanf(infofp,"%s",trans);
                                transobj.set(trans);
                                transobjs.append(transobj);
                                fscanf(infofp,"%u",&tempv);
                                MPlug objoderplugElem=objorderplug.elementByLogicalIndex(i);
                                objoderplugElem.setValue(i);
                                MPlug newvnumplugElem=newvnumplug.elementByLogicalIndex(i);
                                newvnumplugElem.setValue(tempv);
                                newVertNums[i]=tempv;
                                MPlug skipflagplugElem=skipflagplug.elementByLogicalIndex(i);
                                skipflagplugElem.setValue(1);
                        }
                        fclose(infofp);

                        for(int i=0;i<outmeshesplug.numElements();i++){
                                MPlug objoderplugElem=objorderplug.elementByLogicalIndex(i);

                                stat=outmeshes.jumpToElement(i);
                                stat=inmeshes.jumpToElement(i);

                                MDataHandle inmesh=inmeshes.inputValue();
                                MDataHandle outmesh=outmeshes.outputValue();

                                outmesh.copy(inmesh);
                                MItGeometry iter(outmesh,false);

                                initplug.getValue(initflag);

                                MPlug objoderplugElem1;
                                MPlug skipflagplugElem;

                                if( initflag==0 && i<outmeshesplug.numElements() ){//Re-order Cycle
                                        MPlug meshesplugElem=meshesplug.elementByLogicalIndex(i);
                                        MString meshName;
                                        meshesplugElem.getValue(meshName);
                                        int matchflag=0;

                                        for(int cnt=0;cnt<transobjs.length();cnt++){
                                                if(meshName==transobjs[cnt]){
                                                        MGlobal::displayInfo("matched");
                                                        MString tmpcnt;
                                                        tmpcnt.set(cnt);
                                                        MGlobal::displayInfo(tmpcnt);
                                                        MGlobal::displayInfo(meshName);
                                                        MGlobal::displayInfo(transobjs[cnt]);
                                                        MString tempvnum;
                                                        tempvnum.set(cpvexnum[i]);
                                                        MGlobal::displayInfo("cpvexnum="+tempvnum);
                                                        tempvnum.set(newVertNums[cnt]);
                                                        MGlobal::displayInfo("newVertNum="+tempvnum);
                                                        objorder[i]=cnt;
                                                        objoderplugElem1=objorderplug.elementByLogicalIndex(cnt);
                                                        objoderplugElem1.setValue(i);
                                                        skipflagplugElem=skipflagplug.elementByLogicalIndex(cnt);
                                                        //matchflag=1;
                                                        //if(cpvexnum[i]==newVertNums[cnt])
                                                        skipflagplugElem.setValue(0);
                                                }
                                        }
                                        /*if(!matchflag){
                                                MGlobal::displayInfo("did not match");
                                                MString tmpcnt;
                                                tmpcnt.set(i);
                                                MGlobal::displayInfo(tmpcnt);
                                                MGlobal::displayInfo(meshName);
                                                objoderplugElem1=objorderplug.elementByLogicalIndex(i);
                                                objoderplugElem1.setValue(i);
                                                skipflagplugElem=skipflagplug.elementByLogicalIndex(i);
                                                skipflagplugElem.setValue(1);
                                        }*/
                                        if(i==outmeshesplug.numElements()-1){
                                                initplug.setValue(1);
                                                MGlobal::displayInfo("init==1");
                                                data.setClean( plug );
                                                outmeshes.setAllClean();
                                                inmeshes.setAllClean();
                                                return MS::kSuccess;
                                        }
                                }
                        }
                }//if(initflag==0)

                initplug.getValue(initflag);
                if(initflag==1){
                        int num=0;
                        int objnum;
                        objnumplug.getValue(objnum);
                        MPlug objorderplug1(thisNode,m_aObjoder);
                        if(!inmeshes.elementCount())return MS::kUnknownParameter;
                        for(int i=0;i<objnum;i++){
                                int tempindex;

                                MPlug objoderplugElem2=objorderplug1.elementByLogicalIndex(i);
                                objoderplugElem2.getValue(tempindex);
                                MPlug skipflagplugElem=skipflagplug.elementByLogicalIndex(i);
                                MPlug newvnumplugElem=newvnumplug.elementByLogicalIndex(i);

                                int skipf=0;
                                skipflagplugElem.getValue(skipf);
                                        //MString tempi;
                                        //tempi.set(tempindex);
                                        //MGlobal::displayInfo("tempindex="+tempi);
                                        //tempi.set(skipf);
                                        //MGlobal::displayInfo("skipf="+tempi);
                                        //MString tv;
                                        //tv.set(cpvexnum[tempindex]);
                                        //MGlobal::displayInfo("vertex="+tv);

                                if(skipf){
                                        int newVnum;
                                        newvnumplugElem.getValue(newVnum);
                                        fseek(fp,newVnum*3*sizeof(float),1);

                                        if(dt>0){
                                                fseek(fp0,newVnum*3*sizeof(float),1);
                                                fseek(fp1,newVnum*3*sizeof(float),1);
                                                fseek(fp2,newVnum*3*sizeof(float),1);
                                        }
                                        //      MGlobal::displayInfo("skipped");

                                }
                                else{
                                        stat=outmeshes.jumpToElement(tempindex);
                                        stat=inmeshes.jumpToElement(tempindex);

                                        MDataHandle inmesh=inmeshes.inputValue();
                                        MDataHandle outmesh=outmeshes.outputValue();

                                        outmesh.copy(inmesh);
                                        MItGeometry iter(outmesh,false);

                                        //MGlobal::displayInfo("phaseA");
                                        initplug.getValue(initflag);
                                        float* bn;
                                        bn=new float[(cpvexnum[tempindex])*3];
                                        if(!connflag[tempindex]){
                                                fseek(fp,cpvexnum[tempindex]*3*sizeof(float),1);
                                                if(dt>0){
                                                        fseek(fp0,cpvexnum[tempindex]*3*sizeof(float),1);
                                                        fseek(fp1,cpvexnum[tempindex]*3*sizeof(float),1);
                                                        fseek(fp2,cpvexnum[tempindex]*3*sizeof(float),1);
                                                }
                                        }
                                        else{
                                                MPointArray points;
                                                points.setLength(cpvexnum[tempindex]);
                                        //      MGlobal::displayInfo("movie point");
                                                if(dt==0){
                                                        fread_be(bn,sizeof(float),cpvexnum[tempindex]*3,fp);
                                                        for(int vertexnum=0;vertexnum<cpvexnum[tempindex]*3;vertexnum+=3){
                                                                points.set(num,bn[vertexnum],bn[vertexnum+1],bn[vertexnum+2]);
                                                                iter.setPosition(points[num],MSpace::kWorld);
                                                                iter.next();
                                                                num++;
                                                        }
                                                }
                                                else{//Cubic Spline interpolation
                                                        float* bn0;
                                                        float* bn1;
                                                        float* bn2;
                                                        bn0=new float[(cpvexnum[tempindex])*3];
                                                        bn1=new float[(cpvexnum[tempindex])*3];
                                                        bn2=new float[(cpvexnum[tempindex])*3];
                                                        fread_be(bn,sizeof(float),cpvexnum[tempindex]*3,fp);
                                                        fread_be(bn0,sizeof(float),cpvexnum[tempindex]*3,fp0);
                                                        fread_be(bn1,sizeof(float),cpvexnum[tempindex]*3,fp1);
                                                        fread_be(bn2,sizeof(float),cpvexnum[tempindex]*3,fp2);
                                                        for(int vertexnum=0;vertexnum<cpvexnum[tempindex]*3;vertexnum+=3){
                                                                points.set(num,splinef(bn0[vertexnum],bn[vertexnum],bn1[vertexnum],bn2[vertexnum],1+dt),\
                                                                splinef(bn0[vertexnum+1],bn[vertexnum+1],bn1[vertexnum+1],bn2[vertexnum+1],1+dt),\
                                                                splinef(bn0[vertexnum+2],bn[vertexnum+2],bn1[vertexnum+2],bn2[vertexnum+2],1+dt));
                                                                iter.setPosition(points[num],MSpace::kWorld);
                                                                iter.next();
                                                                num++;
                                                        }
                                                        delete [] bn0;
                                                        delete [] bn1;
                                                        delete [] bn2;
                                                }
                                        }
                                        num=0;
                                        delete [] bn;
                                }//if(skipf)
                        }//for(int i=0;i<objnum;i++){
                }//if(initflag==1){
                fclose(fp);
                if(dt>0){
                        fclose(fp0);
                        fclose(fp1);
                        fclose(fp2);
                }
                // clean the output plug, ie unset it from dirty so that maya does
notre-evaluate it
                data.setClean( plug );
                outmeshes.setAllClean();
                inmeshes.setAllClean();
                return MS::kSuccess;
        }
        else{
                return MS::kUnknownParameter;
        }
}
