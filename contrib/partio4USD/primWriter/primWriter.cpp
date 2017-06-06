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

    auto createFileName = [](const std::string& dir, const std::string& prefix, const std::string& suffix, const std::string& ext) -> std::string {
        std::stringstream ss; ss << dir << prefix << "result.topology" << suffix << ext;
        return ss.str();
    };

    auto clearRegex = [](const std::string& in) -> std::string {
        std::string ret = in;
        size_t pos = -2;
        while ((pos = ret.find('.', pos + 2)) != std::string::npos) {
            ret.replace(pos, 1, "\\.");
        }
        return ret;
    };

    template <size_t N>
    std::string searchFile(const std::string& dir, const std::string& prefix, const std::string& suffix, const std::array<std::string, N>& extensions) {
        for (const auto& ext : extensions) {
            const auto ret = createFileName(dir, prefix, suffix, ext);
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
            std::string cacheDir = dgNode.findPlug(_cacheDir).asString().asChar();
            if (cacheDir.size() > 0 && cacheDir.back() != '/') { cacheDir += "/"; }
            const std::string cachePrefix = dgNode.findPlug(_cachePrefix).asString().asChar();
            boost::cmatch match;
            if (boost::regex_search(cachePrefix.c_str(), match, re)) {
                const auto match_1 = match[1].str();
                const auto match_3 = match[3].str();
                const auto match_4 = match[4].str();
                auto manifestPath = searchFile(cacheDir, match_1, match_3, _extensionList);
                if (manifestPath.empty()) {
                    // We will try to stitch the file together using partusdtopology.
                    manifestPath = createFileName(cacheDir, match_1, match_3, _extensionList.front());
                    std::stringstream ss;
                    ss << "partusdtopology " << "\"" << cacheDir << clearRegex(match_1) << "[0-9]+"
                       << clearRegex(match_3) << match_4 << "\" \"" << manifestPath << "\"";
                    std::cerr << "Runing command : " << ss.str() << std::endl;
                    const auto ret = system(ss.str().c_str());
                    if (ret != 0 || !TfIsFile(manifestPath)) {
                        TF_WARN("Manifest file does not exists and/or can't be created. %s", manifestPath.c_str());
                        return;
                    }
                }
                mUsdPrim.GetReferences().AppendReference(SdfReference(manifestPath));
                UsdClipsAPI clips(mUsdPrim);
                clips.SetClipManifestAssetPath(SdfAssetPath(manifestPath));
                clips.SetClipPrimPath("/points");
                std::stringstream ss;
                ss << cacheDir << match_1;
                const auto numDigits = match[2].str().length();
                for (auto i = decltype(numDigits){0}; i < numDigits; ++i) { ss << "#"; }
                ss << match_3 << match_4;
                clips.SetClipTemplateAssetPath(ss.str());
                clips.SetClipTemplateStartTime(getArgs().startTime);
                clips.SetClipTemplateEndTime(getArgs().endTime);
                clips.SetClipTemplateStride(1);
            } else {
                const auto cacheFile = cacheDir + cachePrefix;
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
