#include "fileFormat.h"

#include <pxr/usd/usd/usdaFileFormat.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdGeom/points.h>

#include <Partio.h>

#include <boost/regex.hpp>
#include <functional>

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
        return attr.count == 3 && (attr.type == PARTIO::FLOAT || attr.type == PARTIO::VECTOR);
    }

    template <>
    bool _attributeIsType<PARTIO::FLOAT>(const attr_t& attr) {
        return attr.count == 1 && attr.type == PARTIO::FLOAT;
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
        }
        return false;
    }

    const attr_names_t _positionNames = {"position", "Position", "P"};
    const attr_names_t _velocityNames = {"velocity", "Velocity", "v", "vel"};
    const attr_names_t _radiusNames = {"radius", "radiusPP", "pscale"};
    const attr_names_t _scaleNames = {"scale", "scalePP"};
    const attr_names_t _idNames = {"id", "particleId", "ID", "Id"};
    const SdfPath _pointsPath("/points");

    inline
    bool _isBuiltinAttribute(const std::string& attrName) {
        auto _findInVec = [&attrName] (const attr_names_t& names) -> bool {
            return std::find(names.begin(), names.end(), attrName) != names.end();
        };
        return _findInVec(_positionNames) ||
               _findInVec(_velocityNames) ||
               _findInVec(_radiusNames) ||
               _findInVec(_scaleNames) ||
               _findInVec(_idNames);
    }

    template <typename T> inline
    void _appendValue(partio_t& points, const attr_t& attr, int id, VtArray<T>& a) {
        a.push_back(*points->data<T>(attr, id));
    };

    template <> inline
    void _appendValue<GfVec3f>(partio_t& points, const attr_t& attr, int id, VtArray<GfVec3f>& a) {
        const auto* v = points->data<float>(attr, id);
        a.push_back(GfVec3f(v[0], v[1], v[2]));
    }

    template <> inline
    void _appendValue<GfVec4f>(partio_t& points, const attr_t& attr, int id, VtArray<GfVec4f>& a) {
        const auto* v = points->data<float>(attr, id);
        a.push_back(GfVec4f(v[0], v[1], v[2], v[3]));
    }

    template <> inline
    void _appendValue<long>(partio_t& points, const attr_t& attr, int id, VtArray<long>& a) {
        a.push_back(*points->data<int>(attr, id));
    }

    template <typename T> inline
    VtArray<T> _convertAttribute(partio_t& points, int numParticles, const attr_t& attr) {
        VtArray<T> a;
        a.reserve(numParticles);
        for (auto i = decltype(numParticles){0}; i < numParticles; ++i) {
            _appendValue<T>(points, attr, i, a);
        }
        return a;
    };

    template <typename T> inline
    void _addAttr(UsdGeomPoints& points, const std::string& name, const SdfValueTypeName& typeName,
                  const VtArray<T>& a, const UsdTimeCode& usdTime) {
        points.GetPrim().CreateAttribute(TfToken(name), typeName, false, SdfVariabilityVarying).Set(a, usdTime);
    }

    struct ScopeExit {
        ScopeExit(std::function<void()> _f) : f(_f) { }
        ~ScopeExit() { f(); }
        std::function<void()> f;
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

    auto pointsSchema = UsdGeomPoints::Define(stage, _pointsPath);
    stage->SetDefaultPrim(pointsSchema.GetPrim());
    partio_t points(
        PARTIO::read(resolvedPath.c_str()), [](PARTIO::ParticlesData* d) { d->release(); });
    if (points == nullptr) {
        TF_WARN("[partIO] Can't open file %s", resolvedPath.c_str());
        return false;
    }

    ScopeExit scopeExit([&layerBase, &layer]() { TfDynamic_cast<SdfLayerHandle>(layerBase)->TransferContent(layer); });
    const auto pointCount = points->numParticles();
    if (pointCount < 1) {
        TF_WARN("[partIO] Particle count is zero in %s", resolvedPath.c_str());
        return true;
    }

    // We are trying to guess the frame of the cache here.
    // Also, regex in gcc 4.8.x is unreliable! So we have to use boost here.
    std::stringstream ss_re;
    ss_re << "\\.([0-9]+)\\.(";
    const auto supportedFormats = PARTIO::supportedReadFormats();
    assert(supportedFormats.size() != 0);
    auto formatIt = supportedFormats.begin();
    ss_re << *formatIt;
    for (; formatIt != supportedFormats.end(); ++formatIt) {
        ss_re << "|" << *formatIt;
    }
    ss_re << ")$";
    boost::regex re(ss_re.str().c_str());
    UsdTimeCode outTime = UsdTimeCode::Default();
    boost::cmatch match;
    if (boost::regex_search(resolvedPath.c_str(), match, re)) {
        outTime = std::atoi(match[1].first);
    }

    PARTIO::ParticleAttribute attr;
    if (!_getAttribute<PARTIO::VECTOR>(points, _positionNames, attr, true)) { return false; }
    pointsSchema.GetPointsAttr().Set(_convertAttribute<GfVec3f>(points, pointCount, attr), outTime);

    if (_getAttribute<PARTIO::VECTOR>(points, _velocityNames, attr)) {
        pointsSchema.GetVelocitiesAttr().Set(_convertAttribute<GfVec3f>(points, pointCount, attr), outTime);
    }

    if (_getAttribute<PARTIO::FLOAT>(points, _radiusNames, attr)) {
        auto radii = _convertAttribute<float>(points, pointCount, attr);
        for (auto& radius : radii) {
            radius = radius * 2.0f;
        }
        pointsSchema.GetWidthsAttr().Set(radii, outTime);
    } else if (_getAttribute<PARTIO::VECTOR>(points, _scaleNames, attr)) {
        VtArray<float> radii;
        radii.reserve(pointCount);
        for (auto i = decltype(pointCount){0}; i < pointCount; ++i) {
            const auto* scale = points->data<float>(attr, i);
            // Using the more complex logic for clarity. The compiler will optimize this. Hopefully.
            radii.push_back(2.0f * (scale[0] + scale[1] + scale[2]) / 3.0f);
        }
        pointsSchema.GetWidthsAttr().Set(radii, outTime);
    }

    if (_getAttribute<PARTIO::INT>(points, _idNames, attr)) {
        pointsSchema.GetIdsAttr().Set(_convertAttribute<long>(points, pointCount, attr), outTime);
    }

    const auto numAttributes = points->numAttributes();

    for (auto i = decltype(numAttributes){0}; i < numAttributes; ++i) {
        if (!points->attributeInfo(i, attr) || _isBuiltinAttribute(attr.name)) { continue; }

        if (_attributeIsType<PARTIO::INT>(attr)) {
            _addAttr(pointsSchema, attr.name, SdfValueTypeNames->IntArray,
                     _convertAttribute<int>(points, pointCount, attr), outTime);
        } else if (_attributeIsType<PARTIO::FLOAT>(attr)) {
            _addAttr(pointsSchema, attr.name, SdfValueTypeNames->FloatArray,
                     _convertAttribute<float>(points, pointCount, attr), outTime);
        } else if (_attributeIsType<PARTIO::VECTOR>(attr)) {
            _addAttr(pointsSchema, attr.name, SdfValueTypeNames->Vector3fArray,
                     _convertAttribute<GfVec3f>(points, pointCount, attr), outTime);
        } else if (attr.type == PARTIO::FLOAT && attr.count == 4) {
            _addAttr(pointsSchema, attr.name, SdfValueTypeNames->Float4Array,
                     _convertAttribute<GfVec4f>(points, pointCount, attr), outTime);
        }
    }

    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE
