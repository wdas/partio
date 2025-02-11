// Utilize this library
// https://github.com/LaikaStudios/bgeo_reader


#include "../core/ParticleHeaders.h"

#ifdef MODERN_BGEO
    #include <bgeo/Bgeo.h>
    #include <bgeo/BgeoHeader.h>
    #include <bgeo/parser/ReadError.h>
#endif

#include <unordered_map>

using namespace ika::bgeo;

namespace Partio
{
    std::string getMapping(const char* name)
    {
        const std::unordered_map<std::string, std::string> mapping = 
        {
            {"P", "position"},
        };
        auto it = mapping.find(name);
        if (it != mapping.end())
            return it->second;
        else
            return name;
    }

    ParticlesDataMutable* readBJSON(const char* filename, const bool headersOnly, std::ostream* errorStream)
    {
        try
        {
            BgeoHeader bgeoHeader(filename);
        }
        catch(const ika::bgeo::parser::ReadError& e)
        {
            return nullptr;
        }
        
        Bgeo bgeo(filename, false);

        // Allocate a simple particle with the appropriate number of points
        ParticlesDataMutable* particles = headersOnly ? new ParticleHeaders : create();
        particles->addParticles(bgeo.getPointCount());
        
        int attribCount = bgeo.getPointAttributeCount();
        for (int k = 0; k < attribCount; k++)
        {
            Bgeo::AttributePtr currentAttrib = bgeo.getPointAttribute(k);

            if (strcmp(currentAttrib->getType(), "string") == 0)
            {
                std::vector<int> values;
                std::vector<std::string> strings;
                currentAttrib->getData(values);
                currentAttrib->getStrings(strings);

                ParticleAttribute attrib = particles->addAttribute( getMapping(currentAttrib->getName()).c_str(), INDEXEDSTR, 1);
                for(size_t i = 0; i < strings.size(); i++)
                {
                    particles->registerIndexedStr(attrib, strings[i].c_str());
                }

                for (size_t i = 0; i < bgeo.getPointCount(); i++)
                {
                    int* data = particles->dataWrite<int>(attrib, i);
                    *data = values[i];
                }
                continue;
            }

            int tupleSize = currentAttrib->getTupleSize();
            parser::storage::Storage fundamentalType = currentAttrib->getFundamentalType();// 1: Int32, 2: Fpreal32
            ParticleAttributeType myType = NONE;

            if (fundamentalType == parser::storage::Int32)
                myType = INT;
            else if(fundamentalType == parser::storage::Fpreal32)
            {
                myType = FLOAT;
                const char *subType = currentAttrib->getSubType();

                if ((tupleSize == 3) &&
                    ((strcmp(subType, "point") == 0) ||
                    (strcmp(subType, "vector") == 0) ||
                    (strcmp(subType, "normal") == 0)))
                    myType = VECTOR;
            }
            else
                continue;
            
            ParticleAttribute attrib = particles->addAttribute( getMapping(currentAttrib->getName()).c_str(), myType, tupleSize);
            
            if (fundamentalType == parser::storage::Int32)
            {
                std::vector<int> values;
                currentAttrib->getData(values);
                size_t idx = 0;

                for (size_t i = 0; i < bgeo.getPointCount(); i++)
                {
                    int* data = particles->dataWrite<int>(attrib, i);

                    for (int c = 0; c < tupleSize; c++)
                    {
                        data[c] = values[idx+c];
                    }
                    idx += tupleSize;
                }
            }
            else if(fundamentalType == parser::storage::Fpreal32)
            {
                std::vector<float> values;
                currentAttrib->getData(values);
                size_t idx = 0;

                for (size_t i = 0; i < bgeo.getPointCount(); i++)
                {
                    float* data = particles->dataWrite<float>(attrib, i);
                    for (int c = 0; c < tupleSize; c++)
                    {
                        data[c] = values[idx+c];
                    }
                    idx += tupleSize;
                }
            }
            else
                continue;
        }

        return particles;
    }
}
