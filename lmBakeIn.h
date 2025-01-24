//--------------------------------------------------------
//   lmBakeIn.h v9.5
//
//   Created by Hiroyuki Okubo on 3/10/2006.
//   Copyright 2006 Hiroyuki Okubo All rights reserved.
//
//---------------------------------------------------------


#ifndef __SIMPLE_NODE__H__
#define __SIMPLE_NODE__H__

        #ifdef WIN32
                #define NT_PLUGIN
                #pragma once
                #define WIN32_LEAN_AND_MEAN
                #include <windows.h>
        #endif

        #include <maya/MString.h>
        #include <maya/MGlobal.h>
        #include <maya/MFnDependencyNode.h>
        #include <maya/MTime.h>
        #include <maya/MPlug.h>
        #include <maya/MDagModifier.h>
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
        #include <maya/MObjectArray.h>
        #include <maya/MDGMessage.h>

        class lmBakeIn
                : public MPxNode
        {
        public:
                virtual MStatus compute( const MPlug&, MDataBlock& );
                lmBakeIn(void);
                virtual ~lmBakeIn(void);

        public:

                static  void *  creator();
                virtual void postConstructor();

                static  MStatus initialize();
                static  float splinef(float y0,float y1,float y2,float y3,float x);
                static void nodeAdded(MObject& node,void* np);

        public:

                //       ------------ The Type Info ------------

                /// The specific ID for this node type
                //const static  MTypeId m_TypeId;
                static  MTypeId m_TypeId;

                /// The type name for this new node type
                //const static  MString m_TypeName;
                static  MString m_TypeName;
                static unsigned int objnum;
                static MIntArray callbackIDList;
                static MObject m_aSkipFlag;
                static MObject m_aObjoder;
                static MObject m_aIternalFlag;
                static MObject m_aMeshes;

        private:

                //       ------------ The attributes ------------

                /// an connection made to the input time value
                static MObject m_aTime;

                static MObject m_aInMeshes;
                static MObject m_aOutMeshes;
                static MObject m_aStartf;
                //static MObject m_aEndf;
                static MObject m_aFilePath;
                static MObject m_aVnum;
                static MObject m_aByframe;
                static MObject m_aOffset;
                static MObject m_aInitflag;


        };

#endif