#include "fileFormat.h"

#include <pxr/usd/usd/usdaFileFormat.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdGeom/points.h>
#include <pxr/usd/usdGeom/primvar.h>
#include <pxr/usd/usdGeom/tokens.h>

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
#include "constants.inl"
    using partio_t = std::unique_ptr<PARTIO::ParticlesData, std::function<void(PARTIO::ParticlesData*)>>;
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
    void _addPrimvar(UsdGeomPoints& points, const std::string& name, const SdfValueTypeName& typeName,
                     const VtArray <T>& a, const UsdTimeCode& usdTime) {
        const static std::string _primvars("primvars:");
        auto attr = points.GetPrim().CreateAttribute(TfToken(_primvars + name), typeName, false, SdfVariabilityVarying);
        UsdGeomPrimvar(attr).SetInterpolation(UsdGeomTokens->vertex);
        attr.Set(a, usdTime);
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
    auto layerBaseHandle = TfDynamic_cast<SdfLayerHandle>(layerBase);
    if (!TF_VERIFY(layerBaseHandle)) {
        return false;
    }

    partio_t points(
        PARTIO::read(resolvedPath.c_str()), [](PARTIO::ParticlesData* d) { d->release(); });
    if (points == nullptr) {
        TF_WARN("[partIO] Can't open file %s", resolvedPath.c_str());
        return false;
    }

    auto layer = SdfLayer::CreateAnonymous(".usda");
    auto stage = UsdStage::Open(layer);

    auto pointsSchema = UsdGeomPoints::Define(stage, _pointsPath);
    stage->SetDefaultPrim(pointsSchema.GetPrim());

    ScopeExit scopeExit([&layerBaseHandle, &layer]() { layerBaseHandle->TransferContent(layer); });

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
    const auto positions = _convertAttribute<GfVec3f>(points, pointCount, attr);
    pointsSchema.GetPointsAttr().Set(positions, outTime);

    if (_getAttribute<PARTIO::VECTOR>(points, _velocityNames, attr)) {
        pointsSchema.GetVelocitiesAttr().Set(_convertAttribute<GfVec3f>(points, pointCount, attr), outTime);
    }

    VtArray<float> widths;
    if (_getAttribute<PARTIO::FLOAT>(points, _radiusNames, attr)) {
        widths = _convertAttribute<float>(points, pointCount, attr);
        for (auto& width : widths) {
            width = width * 2.0f;
        }
        pointsSchema.GetWidthsAttr().Set(widths, outTime);
    } else if (_getAttribute<PARTIO::VECTOR>(points, _scaleNames, attr)) {
        widths.reserve(pointCount);
        for (auto i = decltype(pointCount){0}; i < pointCount; ++i) {
            const auto* scale = points->data<float>(attr, i);
            // Using the more complex logic for clarity. The compiler will optimize this. Hopefully.
            widths.push_back(2.0f * (scale[0] + scale[1] + scale[2]) / 3.0f);
        }
        pointsSchema.GetWidthsAttr().Set(widths, outTime);
    }

    // In case there are no widths supplied
    if (widths.size() != positions.size()) {
        widths.resize(positions.size());
    }

    VtArray<GfVec3f> extent;
    if (pointsSchema.ComputeExtent(positions, widths, &extent)) {
        pointsSchema.GetExtentAttr().Set(extent, outTime);
    }

    if (_getAttribute<PARTIO::INT>(points, _idNames, attr)) {
        pointsSchema.GetIdsAttr().Set(_convertAttribute<long>(points, pointCount, attr), outTime);
    }

    const auto numAttributes = points->numAttributes();

    for (auto i = decltype(numAttributes){0}; i < numAttributes; ++i) {
        if (!points->attributeInfo(i, attr) || _isBuiltinAttribute(attr.name)) { continue; }

        if (_attributeIsType<PARTIO::INT>(attr)) {
            _addPrimvar(pointsSchema, attr.name, SdfValueTypeNames->IntArray,
                        _convertAttribute<int>(points, pointCount, attr), outTime);
        } else if (_attributeIsType<PARTIO::FLOAT>(attr)) {
            _addPrimvar(pointsSchema, attr.name, SdfValueTypeNames->FloatArray,
                        _convertAttribute<float>(points, pointCount, attr), outTime);
        } else if (_attributeIsType<PARTIO::VECTOR>(attr)) {
            _addPrimvar(pointsSchema, attr.name, SdfValueTypeNames->Vector3fArray,
                        _convertAttribute<GfVec3f>(points, pointCount, attr), outTime);
        } else if (attr.type == PARTIO::FLOAT && attr.count == 4) {
            _addPrimvar(pointsSchema, attr.name, SdfValueTypeNames->Float4Array,
                        _convertAttribute<GfVec4f>(points, pointCount, attr), outTime);
        }
    }

    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE
