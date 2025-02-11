
#include "../core/ParticleHeaders.h"
#include "readers.h"

namespace Partio
{
    ParticlesDataMutable* readGEO(const char* filename, const bool headersOnly, std::ostream* errorStream)
    {

#ifdef MODERN_BGEO
        ParticlesDataMutable *result = readBJSON(filename, headersOnly, errorStream);
        if (result != nullptr)
            return result;
#endif

        // Fallback to hclassic
        return readHCLASSIC(filename, headersOnly, errorStream);
    }

    bool writeGEO(const char* filename,const ParticlesData& p,const bool compressed,std::ostream* errorStream)
    {
        return writeHCLASSIC(filename, p, compressed, errorStream);
    }
}
