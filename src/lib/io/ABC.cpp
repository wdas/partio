
#include "../Partio.h"
#include "../core/ParticleHeaders.h"
#include "PartioEndian.h"

#include <Alembic/AbcCoreFactory/IFactory.h>
#include <Alembic/Abc/All.h>
#include <Alembic/AbcGeom/IXform.h>
#include <Alembic/AbcGeom/IPoints.h>
#include <Alembic/AbcGeom/GeometryScope.h>

namespace Partio
{

namespace AbcA = Alembic::AbcCoreAbstract;
namespace Abc = Alembic::Abc;
namespace AbcGeom = Alembic::AbcGeom;

void findPoints(Abc::IObject & iObj, double iTime, int & oNumPts,
                std::map<std::string, ParticleAttribute> & oAttrMap,
                ParticlesDataMutable * ioData)
{
    if (AbcGeom::IPoints::matches(iObj.getMetaData()))
    {

        AbcGeom::IPoints ptsObj(iObj.getParent(), iObj.getName());

        // check native properties starting with all important position
        Abc::IP3fArrayProperty ptsProp =
            ptsObj.getSchema().getPositionsProperty();

        if (ptsProp.getNumSamples() > 0)
        {
            Alembic::Util::Dimensions dims;
            ptsProp.getDimensions(dims, Alembic::Abc::ISampleSelector(iTime));
            oNumPts += dims.numPoints();
            if (oAttrMap.find("P") == oAttrMap.end())
            {
                oAttrMap["P"] = ioData->addAttribute("position",VECTOR,3);
            }
        }

        if (ptsObj.getSchema().getVelocitiesProperty().valid() &&
            oAttrMap.find(".velocities") == oAttrMap.end())
        {
            oAttrMap[".velocities"] = ioData->addAttribute("velocity", VECTOR, 3);
        }

        if (ptsObj.getSchema().getWidthsParam().valid() &&
            oAttrMap.find(".widths") == oAttrMap.end())
        {
            oAttrMap[".widths"] = ioData->addAttribute("width", FLOAT, 1);
        }

        if (ptsObj.getSchema().getWidthsParam().valid() &&
            oAttrMap.find(".pointIds") == oAttrMap.end())
        {
            oAttrMap[".pointIds"] = ioData->addAttribute("id", INT, 1);
        }

        Abc::ICompoundProperty arbGeom =
            ptsObj.getSchema().getArbGeomParams();

        std::size_t numArb = 0;
        if (arbGeom.valid())
        {
            numArb = arbGeom.getNumProperties();
        }

        for (std::size_t i = 0; i < numArb; ++i)
        {
            const AbcA::PropertyHeader * prop = &arbGeom.getPropertyHeader(i);
            AbcGeom::GeometryScope scope =
                AbcGeom::GetGeometryScope( prop->getMetaData() );

            std::string propName = prop->getName();
            if (prop->isCompound() && AbcGeom::IsGeomParam(*prop))
            {
                Abc::ICompoundProperty cp(arbGeom, propName);
                prop = cp.getPropertyHeader(".vals");
            }

            if (prop->isArray() && (scope == AbcGeom::kVaryingScope ||
                scope == AbcGeom::kVertexScope ||
                scope == AbcGeom::kFacevaryingScope))
            {
                std::string keyName = ".arbGeomParams/" + propName;
                int extent = prop->getDataType().getExtent();
                switch(prop->getDataType().getPod())
                {
                    case Alembic::Util::kBooleanPOD:
                    case Alembic::Util::kInt8POD:
                    case Alembic::Util::kInt16POD:
                    case Alembic::Util::kInt32POD:
                    case Alembic::Util::kInt64POD:
                    case Alembic::Util::kUint8POD:
                    case Alembic::Util::kUint16POD:
                    case Alembic::Util::kUint32POD:
                    case Alembic::Util::kUint64POD:
                    {
                        if (oAttrMap.find(keyName) == oAttrMap.end())
                        {
                            oAttrMap[keyName] = ioData->addAttribute(
                                propName.c_str(), INT, extent);
                        }
                    }

                    case Alembic::Util::kFloat16POD:
                    case Alembic::Util::kFloat32POD:
                    case Alembic::Util::kFloat64POD:
                    {
                        if (oAttrMap.find(keyName) == oAttrMap.end())
                        {
                            ParticleAttributeType ptype = FLOAT;
                            std::string interp =
                                prop->getMetaData().get("interpretation");
                            if (interp == "vector" || interp == "normal" ||
                                interp == "point")
                            {
                                ptype = VECTOR;
                            }
                            oAttrMap[keyName] = ioData->addAttribute(
                                propName.c_str(), ptype, extent);
                        }
                    }

                    case Alembic::Util::kStringPOD:
                    {
                        if (oAttrMap.find(keyName) == oAttrMap.end())
                        {
                            oAttrMap[keyName] = ioData->addAttribute(
                                propName.c_str(), INDEXEDSTR, extent);
                        }
                    }

                    default:
                    {
                        continue;
                    }

                }
            }
        }
    }

    for (std::size_t i = 0; i < iObj.getNumChildren(); ++i)
    {
        Abc::IObject child = iObj.getChild(i);
        findPoints(child, iTime, oNumPts, oAttrMap, ioData);
    }
}

void writePoints(Abc::IObject & iObj, double iTime,
                const std::map<std::string, ParticleAttribute> & iAttrMap,
                ParticlesDataMutable * iData,
                uint64_t & oIndex,
                std::vector<Imath::M44d> & oMatStack)
{

    bool shouldPop = false;

    if (AbcGeom::IXform::matches(iObj.getMetaData()))
    {
        AbcGeom::IXform xformObj(iObj.getParent(), iObj.getName());
        AbcGeom::IXformSchema schema = xformObj.getSchema();
        AbcGeom::XformSample samp;

        // should we need to linearly interpolate?
        schema.get(samp, Alembic::Abc::ISampleSelector(iTime));
        Abc::M44d mat = samp.getMatrix();

        // doesnt inherit? just push this matrix on the stack
        if (!samp.getInheritsXforms())
        {
            shouldPop = true;
            oMatStack.push_back(mat);
        }
        else if (mat != Abc::M44d())
        {
            shouldPop = true;
            oMatStack.push_back(mat * oMatStack[oMatStack.size() - 1]);
        }
    }
    else if (AbcGeom::IPoints::matches(iObj.getMetaData()))
    {
        Abc::ISampleSelector sel(iTime);
        Abc::ICompoundProperty geomProp(iObj.getProperties(), ".geom");
        Abc::ICompoundProperty arbGeomProp;
        if (geomProp.getPropertyHeader(".arbGeomParams") != NULL)
        {
            arbGeomProp = Abc::ICompoundProperty(geomProp, ".arbGeomParams");
        }

        std::size_t numPts = 0;

        Abc::IArrayProperty pProp(geomProp, "P");
        if (pProp.getNumSamples() > 0)
        {
            Alembic::Util::Dimensions dims;
            pProp.getDimensions(dims, sel);
            numPts = dims.numPoints();
        }

        std::map<std::string, ParticleAttribute>::const_iterator it;
        for (it = iAttrMap.begin(); it != iAttrMap.end(); ++it)
        {
            Abc::IArrayProperty dataProp;
            Abc::IArrayProperty indexProp;

            if (it->first.find(".arbGeomParams/") == 0 && arbGeomProp.valid())
            {
                std::string propName = it->first.substr(15);
                const AbcA::PropertyHeader * header = arbGeomProp.getPropertyHeader(propName);
                if (header && header->isCompound())
                {
                    Abc::ICompoundProperty prop(arbGeomProp, propName);
                    dataProp = Abc::IArrayProperty(prop, ".vals");
                    indexProp = Abc::IArrayProperty(prop, ".indices");
                }
                else if (header)
                {
                    dataProp = Abc::IArrayProperty(arbGeomProp, propName);
                }
            }
            else
            {
                dataProp = Abc::IArrayProperty(geomProp, it->first);
            }

            Alembic::Util::Dimensions dims;
            dataProp.getDimensions(dims, sel);
            std::size_t dataPts = dims.numPoints();
            std::size_t extent = dataProp.getHeader().getDataType().getExtent();
            if (it->second.type == INT)
            {
                std::vector<int> dataVec(dataPts * extent);
                dataProp.getAs(dataVec.data(), Alembic::Util::kInt32POD, sel);
                for (std::size_t i=0; i < numPts; ++i)
                {
                    int * ptr = iData->dataWrite<int>(it->second, i);

                    std::size_t idx = i * extent;
                    // more points than we have data, so copy over the last
                    // extent worth of data
                    if (i >= dataPts)
                    {
                        idx = dataVec.size() - extent;
                    }

                    memcpy(ptr, &(dataVec[idx]), extent * sizeof(int));
                }
            }
            else if (it->second.type == INDEXEDSTR)
            {
                std::vector<int> indexVec;
                if (indexProp.valid())
                {
                    Alembic::Util::Dimensions indexDims;
                    indexProp.getDimensions(indexDims, sel);
                    indexVec.resize(indexDims.numPoints());
                    indexProp.getAs(indexVec.data(), Alembic::Util::kInt32POD, sel);
                }

                std::vector<std::string> dataVec(dataPts * extent);
                dataProp.getAs(dataVec.data(), Alembic::Util::kStringPOD, sel);

                // not indexed, we need to add them manually
                if (indexVec.empty())
                {

                    for (std::size_t i=0; i < numPts; ++i)
                    {
                        int * ptr = iData->dataWrite<int>(it->second, i);

                        std::size_t idx = i;

                        // more points than we have data, so copy over the last
                        // extent worth of data
                        if (i >= dataPts)
                        {
                            idx = dataVec.size() - 1;
                        }
                        *ptr = iData->registerIndexedStr(it->second, dataVec[idx].c_str());
                    }
                }
                else
                {
                    // first register our strings
                    std::vector<int> indexMap(dataVec.size());
                    for (std::size_t i=0; i < dataVec.size(); ++i)
                    {
                        indexMap[i] = iData->registerIndexedStr(it->second, dataVec[i].c_str());
                    }

                    // now remap our indices according to what was registered
                    for (std::size_t i=0; i < numPts; ++i)
                    {
                        int * ptr = iData->dataWrite<int>(it->second, i);

                        std::size_t idx = i;

                        // more points than we have data, so copy over the last
                        // extent worth of data
                        if (i >= indexVec.size())
                        {
                            idx = indexVec.size() - 1;
                        }
                        *ptr = indexMap[indexVec[idx]];
                    }
                }
            }
            else
            {
                std::vector<float> dataVec(dataPts * extent);
                dataProp.getAs(dataVec.data(), Alembic::Util::kFloat32POD, sel);

                // do we need to multiply the matrix by our points?
                if (it->first == "P" &&
                    oMatStack[oMatStack.size() - 1] != Imath::M44d())
                {
                    Imath::M44d mat = oMatStack[oMatStack.size() - 1];
                    for (std::size_t i = 0; i < dataPts; ++i)
                    {
                        Imath::V3f v(dataVec[i*3], dataVec[i*3+1], dataVec[i*3+2]);
                        v = v * mat;
                        dataVec[i*3] = v.x;
                        dataVec[i*3+1] = v.y;
                        dataVec[i*3+2] = v.z;
                    }
                }

                for (std::size_t i=0; i < numPts; ++i)
                {
                    float * ptr = iData->dataWrite<float>(it->second, i);

                    std::size_t idx = i * extent;
                    // more points than we have data, so copy over the last
                    // extent worth of data
                    if (i >= dataPts)
                    {
                        idx = dataVec.size() - extent;
                    }

                    memcpy(ptr, &(dataVec[idx]), extent * sizeof(float));
                }
            }
        }
    }

    for (std::size_t i = 0; i < iObj.getNumChildren(); ++i)
    {
        Abc::IObject child = iObj.getChild(i);
        writePoints(child, iTime, iAttrMap, iData, oIndex, oMatStack);
    }

    if (shouldPop)
    {
        oMatStack.pop_back();
    }
}

ParticlesDataMutable* readABC(const char* filename,
                              const bool headersOnly,
                              std::ostream* errorStream)
{
    // partio looks for the extension .abc to decide to call readABC
    // and we want to be able to accept a time to read (right now in seconds)
    // so we will pass in /tmp/foo@42.5.abc
    std::string name = filename;
    std::size_t lastAt = name.rfind('@');
    double timeVal = -DBL_MAX;
    if (lastAt != std::string::npos)
    {
        try
        {
            // we want to get everything FROM @ to but not including .abc
            timeVal = stod(name.substr(lastAt + 1, name.size() - lastAt - 4));
            name = name.substr(0, lastAt) + ".abc";
        }
        catch(...)
        {}
    }

    Alembic::AbcCoreFactory::IFactory factory;
    Abc::IArchive archive = factory.getArchive(name);

    if (!archive.valid())
    {
        if(errorStream)
        {
            *errorStream << "Partio: Unable to open file Alembic: " << name << std::endl;
            *errorStream << "(" << filename << ")" << std::endl;
        }
        return 0;
    }
    // look for attr names

    // Allocate a simple particle with the appropriate number of points
    ParticlesDataMutable* simple = 0;
    if(headersOnly)
    {
        simple = new ParticleHeaders;
    }
    else
    {
        simple = create();
    }

    int nPoints = 0;
    std::map<std::string, ParticleAttribute> attrMap;

    Abc::IObject top = archive.getTop();
    findPoints(top, timeVal, nPoints, attrMap, simple);
    simple->addParticles(nPoints);

    if (headersOnly || nPoints == 0)
    {
        return simple;
    }

    std::vector<Imath::M44d> matStack;
    matStack.push_back(Imath::M44d());
    uint64_t index = 0;
    writePoints(top, timeVal, attrMap, simple, index, matStack);

    return simple;
}

}