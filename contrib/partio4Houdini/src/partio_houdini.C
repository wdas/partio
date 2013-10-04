/* partio4Houdini  5/01/2013, Miles Green

 PARTIO Import/Export
 Copyright 2013 (c)  All rights reserved

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are
 met:

 * Redistributions of source code must retain the above copyright
 notice, this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright
 notice, this list of conditions and the following disclaimer in
 the documentation and/or other materials provided with the
 distribution.

 Disclaimer: THIS SOFTWARE IS PROVIDED BY  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
 BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 FOR A PARTICULAR PURPOSE, NONINFRINGEMENT AND TITLE ARE DISCLAIMED.
 IN NO EVENT SHALL  THE COPYRIGHT HOLDER OR
 CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND BASED ON ANY
 THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
 */


// Notes: This is the main body of code that converts to and from partio to houdini's internal formats,
// the nodes (ROP/SOP) and command line converter all use this code


#include <vector>
#include <string>
#include <fstream>

#include <GU/GU_PrimPart.h>
#include <GEO/GEO_AttributeHandle.h>
#include <UT/UT_Vector3.h>
#include <UT/UT_Vector4.h>
#include <GU/GU_Detail.h>
#include <GEO/GEO_Point.h>
#include <GEO/GEO_Primitive.h>

#include <Partio.h>

#include "partio_houdini.h"


//helper function to check read file exists
bool fexists(const char *filename)
{
    std::ifstream ifile(filename);
    return ifile;
}

bool partioLoad(char *fileName, GU_Detail *gdp, int verbosity)
{

    if (verbosity>0)
    {
        cerr << "partio: loading " << fileName << " verbosity: "<< verbosity <<  endl;
    }

    if (!fexists(fileName))
    {
        // cerr << " Error: File does not exist: " << fileName <<  endl;
        return false;
    }

    // read file into partio
    Partio::ParticlesDataMutable* particleData=Partio::read(fileName);

    if (verbosity>0)
    {
        std::cout<<"Number of particles: "<<particleData->numParticles()<<std::endl;
        std::cout<<"Number of Attributes: "<<particleData->numAttributes()<<std::endl;
    }

    // build particles
    GU_PrimParticle *part = GU_PrimParticle::build(gdp, particleData->numParticles());


    // if houdini particle primitive created
    if (part)
    {
        for (int i=0;i<particleData->numAttributes();i++)
        {
            Partio::ParticleAttribute attrHandle;
            particleData->attributeInfo(i,attrHandle);

            Partio::ParticlesDataMutable::iterator pIOit=particleData->begin();
            Partio::ParticleAccessor pAttrAccesser(attrHandle);
            pIOit.addAccessor(pAttrAccesser);


            //Now we could additionally print out the names of all attributes in the particle file
            if (verbosity>1)
            {
                std::cout<<"attribute["<<i<<"] is: "<<attrHandle.name << "\n";
            }

            //handle = gdp->getPointAttribute(attrHandle.name.c_str());
            switch (attrHandle.type)
            {
            case Partio::VECTOR:
            {
                std::string houAttrName = attrHandle.name;
                //Partio::ParticleAttribute attrHandle;


                //if position request P, else create attribute
                if (attrHandle.name==std::string("position"))
                {
                    houAttrName = "P";
                }
                else
                {
                    gdp->addFloatTuple(GA_ATTRIB_POINT, houAttrName.c_str(), 3);
                }

                GA_RWPageHandleV3 v_ph(gdp, GEO_POINT_DICT, houAttrName.c_str());

                if (v_ph.isValid())
                {
                    GA_Offset       start, end;
                    for (GA_Iterator it(gdp->getPointRange()); it.blockAdvance(start, end); )
                    {
                        v_ph.setPage(start);
                        for (GA_Offset pt = start; pt < end; ++pt)
                        {
                            if (pIOit==particleData->end())
                            {
                                break;
                            }
                            //get partio data with partio itterator handle
                            const Partio::Data<float,3>& val = pAttrAccesser.data<Partio::Data<float,3> >(pIOit);

                            //set houdini data with houdini page/itterator handle
                            v_ph.value(pt) = UT_Vector3(val[0],val[1],val[2]);

                            ++pIOit;
                        }
                    }
                }

            }
            break;

            case Partio::FLOAT:
            {


                // add houdini attrand get handle to it  (fast interator handle)
                gdp->addFloatTuple(GA_ATTRIB_POINT, attrHandle.name.c_str(), 1);
                GA_RWPageHandleF   f_ph(gdp, GEO_POINT_DICT, attrHandle.name.c_str(),0);

                if (f_ph.isValid())
                {
                    GA_Offset       start, end;
                    for (GA_Iterator it(gdp->getPointRange()); it.blockAdvance(start, end); )
                    {
                        f_ph.setPage(start);
                        for (GA_Offset pt = start; pt < end; ++pt)
                        {
                            if (pIOit==particleData->end())
                            {
                                break;
                            }
                            //get partio data with partio itterator handle
                            const Partio::Data<float,1>& val = pAttrAccesser.data<Partio::Data<float,1> >(pIOit);

                            //set houdini data with houdini page/itterator handle
                            f_ph.value(pt) =val[0];

                            ++pIOit;
                        }
                    }
                }
            }

            break;

            case Partio::INT:
            {


                gdp->addIntTuple(GA_ATTRIB_POINT, attrHandle.name.c_str(), 1);
                GA_RWPageHandleI   i_ph(gdp, GEO_POINT_DICT, attrHandle.name.c_str(),0);

                if (i_ph.isValid())
                {
                    GA_Offset       start, end;
                    for (GA_Iterator it(gdp->getPointRange()); it.blockAdvance(start, end); )
                    {
                        i_ph.setPage(start);

                        for (GA_Offset pt = start; pt < end; ++pt)
                        {
                            if (pIOit==particleData->end())
                            {
                                break;
                            }

                            //get partio data with partio itterator handle
                            const Partio::Data<int,1>& val = pAttrAccesser.data<Partio::Data<int,1> >(pIOit);

                            //set houdini data with houdini page/itterator handle
                            i_ph.value(pt) = val[0];

                            ++pIOit;
                        }

                    }
                }
            }
            break;

            case Partio::NONE:
                break;

            case Partio::INDEXEDSTR:
                break;

            default:
                break;
            }

            // VARIBLES - add the varmapping so we can use $VELOCITY , $RADIUSPP  (but skip position)
            if (attrHandle.name.compare("position")!= 0)
            {

                UT_String attrVar(attrHandle.name.c_str());
                attrVar.toUpper();
                gdp->addVariableName(attrHandle.name.c_str(),attrVar);
                if (verbosity>1)
                {
                    std::cerr << " local var: "<< attrVar << "\n";
                }
            }


        }
    }

    particleData->release();
    return true;
}


bool partioSave(char *fileName, const GU_Detail *gdp, int verbosity)
{
    if (verbosity>0)
    {
        cerr << "partio: saving " << fileName <<" verbosity: "<< verbosity <<endl;
    }

    int nvars = 0,  numPoints;
    GEO_AttributeHandle  handle;
    GA_Attribute *attrib;
    UT_Vector3 tmpVect;
    UT_Vector4 tmpPoint;
    unsigned int a =0 ;

    // create partio data
    Partio::ParticlesDataMutable* particleData=Partio::createInterleave();

    // Set Total number of particles
    numPoints = gdp->points().entries();
    particleData->addParticles(numPoints);

    // get houdini num of point attrs (P/Position is not included)
    nvars = gdp->getAttributes().getDict(GA_ATTRIB_POINT).entries()-1; // minus 1 as we want to remove "_point_vertexRef"

    if (verbosity>0)
    {
        std::cout<<"Number of particles: "<<numPoints<<std::endl;
        std::cout<<"Number of Attributes: "<<nvars<<std::endl;

    }

    //create strings to hold types
    std::string attrNameStr[nvars];
    std::string attrTypeStr[nvars];
    int validAttr[nvars];
    std::fill_n(validAttr, nvars, 1);

    //Itterate over point attrs types first before dong anything to collect some attr info, report attrs we can't process
    for (GA_AttributeDict::iterator it=gdp->getAttributes().getDict(GA_ATTRIB_POINT).begin(); !it.atEnd(); ++it)
    {
        attrib = it.attrib();
        UT_String attrName = attrib->getName();

        //as long as we are not _point_vertex ref
        if (attrName!="_point_vertexRef")
        {
            if (verbosity>1)
            {
                std::cerr << "attribute["<< a <<"] is: "<<attrName <<" ";
            }

            if (attrName=="P")
            {
                attrName  = "position";
                if (verbosity>1)
                {
                    std::cerr << " (renamed to position) "<< " ";
                }
            }

            attrNameStr[a]=attrName;

            // Create the data storage for each attribute.
            switch ( attrib->getStorageClass() )
            {
            case GA_STORECLASS_FLOAT:
                if (attrib->getTupleSize() == 1)
                {
                    attrTypeStr[a]=std::string("float");
                    if (verbosity>1)
                    {
                        std::cerr << "Type: float:  ";
                    }
                }
                else if (attrib->getTupleSize() == 3)
                {
                    attrTypeStr[a]=std::string("vector");
                    if (verbosity>1)
                    {
                        std::cerr << "Type: vector: ";
                    }
                }
                else if (attrib->getTupleSize() == 4)
                {
                    attrTypeStr[a]=std::string("vector");
                    if (verbosity>1)
                    {
                        std::cerr << "Type: vector: ";
                    }
                }
                break;
            case GA_STORECLASS_INT:
                attrTypeStr[a]=std::string("int");
                if (verbosity>1)
                {
                    std::cerr << "Type: int:    ";
                }
                break;
            case GA_STORECLASS_STRING:
                std::cerr << "Warning: Can't write String attribute... unsupported type! Skipping...\n";
                validAttr[a] = 0;
                break;
            case GA_STORECLASS_OTHER:
                std::cerr << "Warning: Can't write attribute with unsupported type! Skipping...\n";
                validAttr[a] = 0;
                break;
            case GA_STORECLASS_INVALID:
                std::cerr << "Warning: Can't write attribute with unsupported type! Skipping...\n";
                validAttr[a] = 0;
                break;
            }
            a++;

            if (verbosity>0)
            {
                std::cerr <<"\n";
            }
        }
    }




    //now for each attr get particles and write to partio
    for (int k =0; k<nvars;k++)
    {
        if (validAttr[k] == 1)
        {
            if (verbosity>2)
            {
                std::cerr << "Attr:  "<<attrNameStr[k] << "\n";
            }



            // create attr handle
            Partio::ParticleAttribute attrHandle;

            if (attrTypeStr[k]=="vector")
            {
                attrHandle=particleData->addAttribute(attrNameStr[k].c_str(),Partio::VECTOR,3);
            }
            if (attrTypeStr[k]=="float")
            {
                attrHandle=particleData->addAttribute(attrNameStr[k].c_str(),Partio::FLOAT,1);
            }
            if (attrTypeStr[k]=="int")
            {
                attrHandle=particleData->addAttribute(attrNameStr[k].c_str(),Partio::INT,1);
            }


            //create iterator
            Partio::ParticlesDataMutable::iterator pIOit=particleData->begin();
            Partio::ParticleAccessor pAttrAccesser(attrHandle);
            pIOit.addAccessor(pAttrAccesser);

            switch (attrHandle.type)
            {
            case Partio::VECTOR:
            {
                std::string houAttrName = attrHandle.name;
                //Partio::ParticleAttribute attrHandle;


                //if position request P, else create attribute
                if (attrHandle.name==std::string("position"))
                {
                    houAttrName = "P";
                }


                GA_ROPageHandleV3 v_ph(gdp, GEO_POINT_DICT, houAttrName.c_str());

                if (v_ph.isValid())
                {
                    GA_Offset       start, end;
                    for (GA_Iterator it(gdp->getPointRange()); it.blockAdvance(start, end); )
                    {
                        v_ph.setPage(start);
                        for (GA_Offset pt = start; pt < end; ++pt)
                        {
                            if (pIOit==particleData->end())
                            {
                                break;
                            }
                            //get partio data with partio itterator handle
                            Partio::Data<float,3>& val = pAttrAccesser.data<Partio::Data<float,3> >(pIOit);

                            //set houdini data with houdini page/itterator handle
                            UT_Vector3 tmpVect  = v_ph.get(pt);
                            val[0]=tmpVect.x();
                            val[1]=tmpVect.y();
                            val[2]=tmpVect.z();
                            ++pIOit;
                        }
                    }
                }

            }
            break;

            case Partio::FLOAT:
            {


                // add houdini attrand get handle to it  (fast interator handle)

                GA_ROPageHandleF   f_ph(gdp, GEO_POINT_DICT, attrHandle.name.c_str(),0);

                if (f_ph.isValid())
                {
                    GA_Offset       start, end;
                    for (GA_Iterator it(gdp->getPointRange()); it.blockAdvance(start, end); )
                    {
                        f_ph.setPage(start);
                        for (GA_Offset pt = start; pt < end; ++pt)
                        {
                            if (pIOit==particleData->end())
                            {
                                break;
                            }
                            //get partio data with partio itterator handle
                            Partio::Data<float,1>& val = pAttrAccesser.data<Partio::Data<float,1> >(pIOit);

                            //set houdini data with houdini page/itterator handle
                            val[0] = f_ph.get(pt);

                            ++pIOit;
                        }
                    }
                }
            }

            break;

            case Partio::INT:
            {

                GA_ROPageHandleI   i_ph(gdp, GEO_POINT_DICT, attrHandle.name.c_str(),0);

                if (i_ph.isValid())
                {
                    GA_Offset       start, end;
                    for (GA_Iterator it(gdp->getPointRange()); it.blockAdvance(start, end); )
                    {
                        i_ph.setPage(start);

                        for (GA_Offset pt = start; pt < end; ++pt)
                        {
                            if (pIOit==particleData->end())
                            {
                                break;
                            }

                            //get partio data with partio itterator handle
                            Partio::Data<int,1>& val = pAttrAccesser.data<Partio::Data<int,1> >(pIOit);

                            //get houdini data with houdini page/itterator handle and set partio data
                            val[0] =i_ph.get(pt);

                            ++pIOit;
                        }

                    }
                }
            }
            break;

            case Partio::NONE:
                break;

            case Partio::INDEXEDSTR:
                break;

            default:
                break;
            }
        }
    }

    Partio::write(fileName,*particleData);
    particleData->release();

    return true;
}
