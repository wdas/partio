#include <pxr/pxr.h>
#include <pxr/base/tf/fileUtils.h>
#include <pxr/usd/usd/clipsAPI.h>

#include <usdMaya/primWriterRegistry.h>

#include <Partio.h>

#include <boost/regex.hpp>

#include <thread>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {
    constexpr auto _cacheDir = "cacheDir";
    constexpr auto _cachePrefix = "cachePrefix";
    const std::array<std::string, 3> _extensionList = {"usd", "usda", "usdc"};

    template <size_t N>
    std::string searchFile(const std::string& prefix, const std::string& suffix, const std::array<std::string, N>& extensions) {
        for (const auto& ext : extensions) {
            std::stringstream ss; ss << prefix << "result.topology" << suffix << ext;
            const auto ret = ss.str();
            if (TfIsFile(ret)) {
                return ret;
            }
        }

        return "";
    }

    class partioVisualizerWriter : public MayaTransformWriter {
    public:
        partioVisualizerWriter(const MDagPath & iDag, const SdfPath& uPath, bool instanceSource, usdWriteJobCtx& jobCtx) :
            MayaTransformWriter(iDag, uPath, instanceSource, jobCtx) {
            mUsdPrim = getUsdStage()->DefinePrim(getUsdPath());
            TF_AXIOM(mUsdPrim);

            static boost::regex re;
            static std::once_flag once_flag;
            std::call_once(once_flag, []() {
                std::stringstream ss;
                ss << "(.+[\\.|_])([0-9]+)([\\.|_])(";
                const auto supportedFormats = PARTIO::supportedReadFormats();
                assert(supportedFormats.size() != 0);
                auto formatIt = supportedFormats.begin();
                ss << *formatIt;
                for (; formatIt != supportedFormats.end(); ++formatIt) {
                    ss << "|" << *formatIt;
                }
                ss << ")$";
                re = boost::regex(ss.str().c_str());
            });
            MFnDagNode dgNode(iDag);
            const std::string cacheFile((dgNode.findPlug(_cacheDir).asString() +
                                         dgNode.findPlug(_cachePrefix).asString()).asChar());
            boost::cmatch match;
            if (boost::regex_search(cacheFile.c_str(), match, re)) {
                const auto manifestPath = searchFile(match[1].str(), match[3].str(), _extensionList);
                if (manifestPath.empty()) {
                    TF_WARN("Stitched path does not exists for cache %s", cacheFile.c_str());
                } else {                    
                    mUsdPrim.GetReferences().AppendReference(SdfReference(manifestPath));
                    UsdClipsAPI clips(mUsdPrim);
                    clips.SetClipManifestAssetPath(SdfAssetPath(manifestPath));
                    clips.SetClipPrimPath("/points");
                    std::stringstream ss;
                    ss << match[1].str();
                    const auto numDigits = match[2].str().length();
                    for (auto i = decltype(numDigits){0}; i < numDigits; ++i) { ss << "#"; }
                    ss << match[3].str() << match[4].str();
                    clips.SetClipTemplateAssetPath(ss.str());
                    clips.SetClipTemplateStartTime(getArgs().startTime);
                    clips.SetClipTemplateEndTime(getArgs().endTime);
                    clips.SetClipTemplateStride(1);
                }
            } else {
                TF_WARN("%s is not a valid path to a PartIO cache.", cacheFile.c_str());
            }
        }

        ~partioVisualizerWriter() { };
    };

    using partioVisualizerWriterPtr = std::shared_ptr<partioVisualizerWriter>;
}

TF_REGISTRY_FUNCTION_WITH_TAG(PxrUsdMayaPrimWriterRegistry, partioVisualizerWriter) {

    PxrUsdMayaPrimWriterRegistry::Register("partioVisualizer",
                 [](const MDagPath& iDag,
                    const SdfPath& uPath, bool instanceSource,
                    usdWriteJobCtx& jobCtx) -> MayaPrimWriterPtr { return std::make_shared<partioVisualizerWriter>(
                    iDag, uPath, instanceSource, jobCtx); });
}
