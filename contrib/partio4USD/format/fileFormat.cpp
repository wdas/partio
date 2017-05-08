#include "fileFormat.h"

#include <pxr/usd/usd/usdaFileFormat.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdGeom/points.h>

#include <Partio.h>

#include <memory>
#include <exception>
#include <sstream>


PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PUBLIC_TOKENS(
   UsdPartIOFileFormatTokens,
   USDPARTIO_FILE_FORMAT_TOKENS);

TF_REGISTRY_FUNCTION(TfType)
{
    SDF_DEFINE_FILE_FORMAT(UsdPartIOFileFormat, SdfFileFormat);
}

UsdPartIOFileFormat::UsdPartIOFileFormat() :
    SdfFileFormat(
        UsdPartIOFileFormatTokens->Id,
        UsdPartIOFileFormatTokens->Version,
        UsdPartIOFileFormatTokens->Target,
        UsdPartIOFileFormatTokens->Id) {
}

namespace {
    using partio_t = std::unique_ptr<PARTIO::ParticlesData, std::function<void(PARTIO::ParticlesData*)>>;
    using attr_names_t = std::vector<std::string>;
    using attr_t = PARTIO::ParticleAttribute;

    template <PARTIO::ParticleAttributeType AT>
    bool _attributeIsType(const PARTIO::ParticleAttribute& attr) {
        return attr.type == AT;
    }

    // Vector attributes require special handling, they can be either the type of vector
    // or type of float with at least 3 elements.
    template <>
    bool _attributeIsType<PARTIO::VECTOR>(const attr_t& attr) {
        return attr.count > 2 && (attr.type == PARTIO::FLOAT || attr.type == PARTIO::VECTOR);
    }

    template <PARTIO::ParticleAttributeType AT>
    bool _getAttribute(partio_t& points, const attr_names_t& names, attr_t& attr, bool required = false) {
        for (const auto& name : names) {
            if (points->attributeInfo(name.c_str(), attr)) {
                if (_attributeIsType<AT>(attr)) {
                    return true;
                } else { break; }
            }
        }

        if (required) {
            std::stringstream ss;
            ss << "( ";
            for (const auto& name : names) { ss << name << " "; }
            ss << ")";
            TF_WARN("[partIO] Required attribute %s can't be found with type %i", ss.str().c_str(), AT);
            throw std::exception();
        }
        return false;
    }

    const attr_names_t _positionNames = {"position", "Position"};
    const attr_names_t _velocityNames = {"velocity", "Velocity"};
    const attr_names_t _radiusNames = {"radius", "radiusPP"};
    const attr_names_t _idNames = {"id", "particleId"};

    inline
    bool _isBuiltinAttribute(const std::string& attrName) {
        auto _findInVec = [&attrName] (const attr_names_t& names) -> bool {
            return std::find(_positionNames.begin(), _positionNames.end(), attrName) != _positionNames.end();
        };
        return _findInVec(_positionNames) ||
               _findInVec(_velocityNames) ||
               _findInVec(_radiusNames) ||
               _findInVec(_idNames);
    }

    template <typename to> inline
    void _appendValue(partio_t& points, const attr_t& attr, int id, VtArray<to>& a) {
        a.push_back(*points->data<to>(attr, id));
    };

    template <> inline
    void _appendValue<GfVec3f>(partio_t& points, const attr_t& attr, int id, VtArray<GfVec3f>& a) {
        const auto* v = points->data<float>(attr, id);
        a.push_back(GfVec3f(v[0], v[1], v[2]));
    }

    template <> inline
    void _appendValue<long>(partio_t& points, const attr_t& attr, int id, VtArray<long>& a) {
        a.push_back(*points->data<int>(attr, id));
    }

    template <typename to> inline
    VtArray<to> _convertAttribute(partio_t& points, int numParticles, const attr_t& attr) {
        VtArray<to> a;
        a.reserve(numParticles);
        for (auto i = decltype(numParticles){0}; i < numParticles; ++i) {
            _appendValue<to>(points, attr, i, a);
        }
        return a;
    };
}

UsdPartIOFileFormat::~UsdPartIOFileFormat() {
}

bool UsdPartIOFileFormat::CanRead(const std::string &file) const {
    return true;
}

bool UsdPartIOFileFormat::Read(const SdfLayerBasePtr& layerBase,
                               const std::string& resolvedPath,
                               bool metadataOnly) const {
    auto layer = SdfLayer::CreateAnonymous(".usda");
    auto stage = UsdStage::Open(layer);

    auto pointsSchema = UsdGeomPoints::Define(stage, SdfPath("/points"));
    partio_t points(
        PARTIO::read(resolvedPath.c_str()), [](PARTIO::ParticlesData* d) { d->release(); });
    if (points == nullptr) {
        TF_WARN("[partIO] Can't open file %s", resolvedPath.c_str());
        return false;
    }

    const auto pointCount = points->numParticles();
    if (pointCount < 1) {
        TF_WARN("[partIO] Particle count is zero in %s", resolvedPath.c_str());
        return false;
    }

    // Cleaner code structure is preferred. We will only see performance drops when something fails.
    try {
        PARTIO::ParticleAttribute attr;
        _getAttribute<PARTIO::VECTOR>(points, _positionNames, attr, true);
        pointsSchema.GetPointsAttr().Set(_convertAttribute<GfVec3f>(points, pointCount, attr));

        if (_getAttribute<PARTIO::VECTOR>(points, _velocityNames, attr)) {
            pointsSchema.GetVelocitiesAttr().Set(_convertAttribute<GfVec3f>(points, pointCount, attr));
        }

        if (_getAttribute<PARTIO::FLOAT>(points, _radiusNames, attr)) {
            auto radii = _convertAttribute<float>(points, pointCount, attr);
            for (auto& radius : radii) {
                radius = radius * 2.0f;
            }
            pointsSchema.GetWidthsAttr().Set(radii);
        }

        if (_getAttribute<PARTIO::INT>(points, _idNames, attr)) {
            pointsSchema.GetIdsAttr().Set(_convertAttribute<long>(points, pointCount, attr));
        }

        TfDynamic_cast<SdfLayerHandle>(layerBase)->TransferContent(layer);
    } catch (...) {
        return false;
    }

    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE
