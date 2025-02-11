
#include "../core/ParticleHeaders.h"
#include "readers.h"

namespace Partio
{
    ParticlesDataMutable* readBGEO(const char* filename, const bool headersOnly, std::ostream* errorStream)
    {

#ifdef MODERN_BGEO
        ParticlesDataMutable *result = readBJSON(filename, headersOnly, errorStream);
        if (result != nullptr)
            return result;
#endif

        // Fallback to bhclassic
        return readBHCLASSIC(filename, headersOnly, errorStream);
    }

    bool writeBGEO(const char* filename,const ParticlesData& p,const bool compressed,std::ostream* errorStream)
    {
        return writeBHCLASSIC(filename, p, compressed, errorStream);
    }
}
