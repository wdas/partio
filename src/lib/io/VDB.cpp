
#include "../Partio.h"
#include "../core/ParticleHeaders.h"
#include "PartioEndian.h"

#include <openvdb/openvdb.h>
#include <openvdb/points/PointConversion.h>
#include <openvdb/points/PointCount.h>

#include <cfloat>
#include <unordered_map>
#include <tbb/task_scheduler_init.h>
namespace Partio
{

static void initLibrary() {
    static struct OpenVDBLib {
        OpenVDBLib()  { openvdb::initialize(); }
        ~OpenVDBLib() { openvdb::uninitialize(); }
    } vdblib_init;
}

template <typename PartioType, typename VDBType, typename IndexType> void
convertScalar(const openvdb::points::AttributeHandle<VDBType>& vdbAttr,
              const IndexType&                                 vdbIndex,
              ParticlesDataMutable*                            particles,
              ParticleAttribute&                               partioAttr,
              size_t                                           partioIndex)
{
    auto* attrStorage = particles->dataWrite<PartioType>(partioAttr, partioIndex);
    for (int i = 0; i < partioAttr.count; ++i)
        attrStorage[i] = vdbAttr.get(vdbIndex, i);
}

template <typename PartioType, typename VDBType, typename IndexType> PartioType*
convertVector(const openvdb::points::AttributeHandle<VDBType>& vdbAttr,
              const IndexType&                                 vdbIndex,
              ParticlesDataMutable*                            particles,
              ParticleAttribute&                               partioAttr,
              size_t                                           partioIndex)
{
    openvdb::Vec3d vdbData = vdbAttr.get(vdbIndex);
    PartioType* partioData = particles->dataWrite<PartioType>(partioAttr, partioIndex);

    for (unsigned i = 0; i < 3; ++i)
        partioData[i] = vdbData[i];

    return partioData;
}

enum {
    Precision32,
    Precision64
};

ParticlesDataMutable* readVDB(const char* filename,
                              const bool headersOnly,
                              std::ostream* errorStream)
{

    std::size_t numThreads = 0;
    char *cueThreads = getenv("CUE_THREADS");
    if (cueThreads)
    {
        numThreads = atoi(cueThreads);
    }
    tbb::task_scheduler_init schedulerInit((numThreads == 0) ? tbb::task_scheduler_init::automatic : numThreads);

    initLibrary();

    openvdb::io::File file(filename);
    file.open();
    if (!file.isOpen()) {
        if (errorStream)
            *errorStream << "Partio.VDB: Unable to open VDB file: '" << filename << "'\n";
        return nullptr;
    }

    // Since we can be merging multiple VDB-Points objects:
    // 1:
    //   Walk the file collecting all the VDB-Point objects
    //   Save the point attributes into an AttributeCache to ensure all attribute types match
    //   Cache the index to the attribute (and it's precision)
    // 2:
    //   Re-walk all the found grids, copying data (using the cached attribute index and precision)
    //
    using AttributeIndex = std::tuple<size_t, ParticleAttributeType, bool, bool>;
    struct AttributeCache {
        std::vector<AttributeIndex> indices;
        ParticleAttribute           partio;    // partio.type contains the type that partio will store / allocate
        unsigned                    tupleSize;
        AttributeCache(ParticleAttributeType type,
                       unsigned              tplSz) : tupleSize(tplSz) { partio.type = type; }
    };

    size_t totalPoints = 0;
    std::unordered_map<openvdb::Name, AttributeCache> vdbAttributes;
    std::vector<std::pair<openvdb::points::PointDataGrid::Ptr, size_t>> pointGrids;

    for (openvdb::io::File::NameIterator nameIter = file.beginName(), nameEnd = file.endName(); nameIter != nameEnd; ++nameIter) {
        const std::string& name = *nameIter;
        openvdb::GridBase::Ptr grid = file.readGrid(name);
        if (!grid) {
            if (errorStream)
                *errorStream << "Partio.VDB: Could not open grid: '" << name << "'\n";
            continue;
        }

        // This seems a silly way to detect points or not, so just use casting below
        //if (grid->getGridClass() != openvdb::GridClass::GRID_UNKNOWN)

        openvdb::points::PointDataGrid::Ptr ptGrid = openvdb::gridPtrCast<openvdb::points::PointDataGrid>(grid);
        if (!ptGrid) {
            if (errorStream) {
                *errorStream << "Partio.VDB: grid[" << name << "] is not a PointDataGrid, but a "
                             << openvdb::GridBase::gridClassToString(grid->getGridClass()) << "\n";
            }
            continue;
        }
        // From VRAY_OPenVDB_Points.cc:
        // //
        // // enable streaming mode to auto-collapse attributes
        // // on read for improved memory efficiency
        // points::setStreamingMode(ptGrid->tree(), /*on=*/true);

        const auto& leafIter = ptGrid->tree().cbeginLeaf();
        if (!leafIter)
            continue;

        const auto& attrs = leafIter->attributeSet();
        for (auto&& attrNameIndx : attrs.descriptor().map()) {
            const auto& vdbAttr = attrs.get(attrNameIndx.second);
            const auto& vdbType = vdbAttr->type();
            // VDB-6 ABI vdbAttr->valueTypeSize() vdbAttr->storageTypeSize()
            bool spatial = false;
            bool precision = Precision32;
            ParticleAttributeType vdbRepr = NONE;
            ParticleAttributeType partioType = NONE;
            unsigned tupleSize = vdbAttr->dataSize() / vdbAttr->size();

            if (vdbType.first == "vec3s") {
                tupleSize = 3; // Partio wants VECTOR types to have a tupleSize / count of 3
                std::tie(partioType, vdbRepr) = std::make_tuple(VECTOR, VECTOR);
                spatial = vdbType.second.find(openvdb::points::PositionRange::name()) == 0;
            }
            else if (vdbType.first == "vec3d") {
                tupleSize = 3; // Partio wants VECTOR types to have a tupleSize / count of 3
                std::tie(partioType, vdbRepr, precision) = std::make_tuple(VECTOR, VECTOR, Precision64);
                spatial = vdbType.second.find(openvdb::points::PositionRange::name()) == 0;
            }
            else if (vdbType.first == "float")
                std::tie(partioType, vdbRepr) = std::make_tuple(tupleSize == 3 ? VECTOR : FLOAT, FLOAT);
            else if (vdbType.first == "double")
                std::tie(partioType, vdbRepr, precision) = std::make_tuple(tupleSize == 3 ? VECTOR : FLOAT, FLOAT, Precision64);
            else if (vdbType.first == "int32")
                std::tie(partioType, vdbRepr) = std::make_tuple(INT, INT);
            else if (vdbType.first == "int64")
                std::tie(partioType, vdbRepr, precision) = std::make_tuple(INT, INT, Precision64);
            else if (vdbAttr->isType<openvdb::points::StringAttributeArray>())
                std::tie(partioType, vdbRepr) = std::make_tuple(INDEXEDSTR, INDEXEDSTR);
            else {
                // WARN ?
                continue;
            }

            // If this attribute already existed, make sure it's the same type and tuple size
            // The VDB representation can vary as that is going to be pulled directly from the
            // VDB attribute stream.
            //
            auto inserted = vdbAttributes.emplace(std::piecewise_construct,
                                                  std::forward_as_tuple(attrNameIndx.first),
                                                  std::forward_as_tuple(partioType, tupleSize));
            if (!inserted.second) {
                const AttributeCache& cache = inserted.first->second;
                if (cache.partio.type != partioType || cache.tupleSize != tupleSize)
                    continue;
            }
            inserted.first->second.indices.emplace_back(attrNameIndx.second, vdbRepr, precision, spatial);
        }

        pointGrids.emplace_back(std::move(ptGrid), totalPoints);
        totalPoints += openvdb::points::pointCount(pointGrids.back().first->tree());
    }

    const std::unordered_map<std::string, std::string> kHoudiniMapping = {
        {"P",      "position"},
        {"pscale", "width"},
        {"v",      "velocity"},
    };

    // Allocate a simple particle with the appropriate number of points
    ParticlesDataMutable* particles = headersOnly ? new ParticleHeaders : create();
    particles->addParticles(totalPoints);

    const size_t nGrids = pointGrids.size();
    for (auto& attrIndex : vdbAttributes) {
        AttributeCache& cache = attrIndex.second;
        const ParticleAttributeType partioType = cache.partio.type;

        // Check the attribute isn't ignored / disabled
        //
        if (cache.indices.size() != nGrids || partioType == NONE) {
            cache.indices.clear();
            cache.partio.type = NONE;
            continue;
        }
        assert(partioType != NONE && "Invalid Partio type");

        // Create the PartIO attribute
        //
        const std::string& vdbName = attrIndex.first;
        const char*        name    = vdbName.c_str();

        // Remap to Partio / Katana names if they don't already exist
        //
        const auto itr = kHoudiniMapping.find(vdbName);
        if (itr != kHoudiniMapping.end() && vdbAttributes.count(itr->second) == 0)
            name = itr->second.c_str();

        cache.partio = particles->addAttribute(name, partioType, cache.tupleSize);
        if (headersOnly)
            continue;

        // Now walk the grids and copy the leaf attribute data
        // If this loop needs to be threaded, pointGrids[i].second contains the starting offset
        // (the particleIdx for partio) of the grid in pointGrids[i].first
        //
        for (size_t i = 0; i < nGrids; ++i) {
            const size_t attrIdx   = std::get<0>(cache.indices[i]);
            const auto   vdbType   = std::get<1>(cache.indices[i]);
            const auto   precision = std::get<2>(cache.indices[i]);
            const bool   spatial   = std::get<3>(cache.indices[i]);
            assert(vdbType != NONE && "Invalid VDB representation");

            openvdb::points::PointDataGrid *ptGrid = pointGrids[i].first.get();
            const auto &ptTree = ptGrid->constTree();
            const auto &transform = ptGrid->constTransform();
            std::vector<openvdb::Index64> offsets;
            std::ignore = openvdb::points::pointOffsets(offsets, ptTree);

            if (vdbType == VECTOR)
            {
                openvdb::tree::LeafManager<const openvdb::points::PointDataTree> leafManager(ptTree);
                leafManager.foreach (
                    [&](const openvdb::points::PointDataTree::LeafNodeType &leafIter, size_t idx) {
                        assert(idx < offsets.size());
                        openvdb::Index64 offset = pointGrids[i].second;
                        if (idx > 0)
                            offset += offsets[idx - 1];

                        const auto &attrs = leafIter.attributeSet();
                        const auto *vdbAttr = attrs.get(attrIdx);

                        openvdb::points::AttributeHandle<openvdb::Vec3f>::Ptr vec32;
                        openvdb::points::AttributeHandle<openvdb::Vec3d>::Ptr vec64;
                        if (precision == Precision32)
                            vec32.reset(new openvdb::points::AttributeHandle<openvdb::Vec3f>(*vdbAttr));
                        else
                            vec64.reset(new openvdb::points::AttributeHandle<openvdb::Vec3d>(*vdbAttr));
                        for (auto indexIter = leafIter.beginIndexOn(); indexIter; ++indexIter)
                        {
                            float *partioData;
                            if (vec32)
                                partioData = convertVector<float>(*vec32, *indexIter, particles, cache.partio, static_cast<size_t>(offset++));
                            else
                                partioData = convertVector<float>(*vec64, *indexIter, particles, cache.partio, static_cast<size_t>(offset++));

                            // Reinterpret the voxel-data to world-space co-ordinates
                            if (spatial)
                            {
                                // Extract the voxel-space position of the point.
                                openvdb::Vec3f *voxelPosition = reinterpret_cast<openvdb::Vec3f *>(partioData);
                                // Compute the world-space position of the point.
                                *voxelPosition = transform.indexToWorld(*voxelPosition + indexIter.getCoord().asVec3d());
                            }
                        }
                    },
                    /*threaded*/ true);
            }
            else if (vdbType == FLOAT)
            {
                openvdb::tree::LeafManager<const openvdb::points::PointDataTree> leafManager(ptTree);
                leafManager.foreach (
                    [&](const openvdb::points::PointDataTree::LeafNodeType &leafIter, size_t idx) {
                        assert(idx < offsets.size());
                        openvdb::Index64 offset = pointGrids[i].second;
                        if (idx > 0)
                            offset += offsets[idx - 1];

                        const auto &attrs = leafIter.attributeSet();
                        const auto *vdbAttr = attrs.get(attrIdx);

                        openvdb::points::AttributeHandle<float>::Ptr float32;
                        openvdb::points::AttributeHandle<double>::Ptr float64;
                        if (precision == Precision32)
                            float32.reset(new openvdb::points::AttributeHandle<float>(*vdbAttr));
                        else
                            float64.reset(new openvdb::points::AttributeHandle<double>(*vdbAttr));

                        for (auto indexIter = leafIter.beginIndexOn(); indexIter; ++indexIter)
                        {
                            if (float32)
                                convertScalar<float>(*float32, *indexIter, particles, cache.partio, static_cast<size_t>(offset++));
                            else
                                convertScalar<float>(*float64, *indexIter, particles, cache.partio, static_cast<size_t>(offset++));
                        }
                    },
                    /*threaded*/ true);
            }
            else if (vdbType == INT)
            {
                openvdb::tree::LeafManager<const openvdb::points::PointDataTree> leafManager(ptTree);
                leafManager.foreach (
                    [&](const openvdb::points::PointDataTree::LeafNodeType &leafIter, size_t idx) {
                        assert(idx < offsets.size());
                        openvdb::Index64 offset = pointGrids[i].second;
                        if (idx > 0)
                            offset += offsets[idx - 1];

                        const auto &attrs = leafIter.attributeSet();
                        const auto *vdbAttr = attrs.get(attrIdx);

                        openvdb::points::AttributeHandle<int32_t>::Ptr int32;
                        openvdb::points::AttributeHandle<int64_t>::Ptr int64;
                        if (precision == Precision32)
                            int32.reset(new openvdb::points::AttributeHandle<openvdb::Int32>(*vdbAttr));
                        else
                            int64.reset(new openvdb::points::AttributeHandle<openvdb::Int64>(*vdbAttr));

                        for (auto indexIter = leafIter.beginIndexOn(); indexIter; ++indexIter)
                        {
                            if (int32)
                                convertScalar<int>(*int32, *indexIter, particles, cache.partio, static_cast<size_t>(offset++));
                            else
                                convertScalar<int>(*int64, *indexIter, particles, cache.partio, static_cast<size_t>(offset++));
                        }
                    },
                    /*threaded*/ true);
            }
            else if (vdbType == INDEXEDSTR)
            {
                // handle string type none-thread version.
                openvdb::tree::LeafManager<const openvdb::points::PointDataTree> leafManager(ptTree);
                leafManager.foreach (
                    [&](const openvdb::points::PointDataTree::LeafNodeType &leafIter, size_t idx) {
                        const auto &attrs = leafIter.attributeSet();
                        const auto *vdbAttr = attrs.get(attrIdx);
                        openvdb::points::StringAttributeHandle strs(*vdbAttr, attrs.descriptor().getMetadata());
                        
                        int id(0);
                        for (auto indexIter = leafIter.beginIndexOn(); indexIter; ++indexIter, ++id)
                        {
                            int index = particles->registerIndexedStr(cache.partio, strs.get(*indexIter).c_str());
                            int *idd = particles->dataWrite<int>(cache.partio, id);
                            *idd = index;
                        }
                    },
                    /*threaded*/ false);
            }
            else
            {
                assert(0 && "Unsupported attribute type");
            }
        }
    }

    file.close();
    return particles;
}

} // namespace Partio
