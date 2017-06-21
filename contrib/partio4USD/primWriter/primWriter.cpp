#include <pxr/pxr.h>
#include <pxr/base/tf/fileUtils.h>
#include <pxr/usd/usd/clipsAPI.h>
#include <pxr/usd/usdGeom/points.h>

#include <usdMaya/primWriterRegistry.h>

#include <Partio.h>

#include <boost/regex.hpp>

#include <thread>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {
    constexpr auto _cacheDir = "cacheDir";
    constexpr auto _cachePrefix = "cachePrefix";
    const std::array<std::string, 3> _extensionList = {"usd", "usda", "usdc"};
    boost::regex _frameAndExtensionRe;
    boost::regex _frameRe;

    auto _createFileName = [](const std::string& dir, const std::string& prefix, const std::string& suffix, const std::string& ext) -> std::string {
        std::stringstream ss; ss << dir << prefix << "result.topology" << suffix << ext;
        return ss.str();
    };

    auto _clearRegex = [](const std::string& in) -> std::string {
        std::string ret = in;
        size_t pos = -2;
        while ((pos = ret.find('.', pos + 2)) != std::string::npos) {
            ret.replace(pos, 1, "\\.");
        }
        return ret;
    };

    template <size_t N>
    std::string _searchFile(const std::string& dir, const std::string& prefix, const std::string& suffix,
                            const std::array<std::string, N>& extensions) {
        for (const auto& ext : extensions) {
            const auto ret = _createFileName(dir, prefix, suffix, ext);
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
            mUsdPrim = UsdGeomPoints::Define(getUsdStage(), getUsdPath()).GetPrim();
            TF_AXIOM(mUsdPrim);

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
                _frameAndExtensionRe = boost::regex(ss.str().c_str());
                _frameRe = boost::regex("(.+[\\.|_])([0-9]+)([\\.|_].+)");
            });
            MFnDagNode dgNode(iDag);
            std::string cacheDir = dgNode.findPlug(_cacheDir).asString().asChar();
            if (cacheDir.size() > 0 && cacheDir.back() != '/') { cacheDir += "/"; }
            const std::string cachePrefix = dgNode.findPlug(_cachePrefix).asString().asChar();
            boost::cmatch match;
            if (boost::regex_search(cachePrefix.c_str(), match, _frameAndExtensionRe)) {
                const auto match_1 = match[1].str();
                const auto match_3 = match[3].str();
                const auto match_4 = match[4].str();
                auto manifestPath = _searchFile(cacheDir, match_1, match_3, _extensionList);
                if (manifestPath.empty()) {
                    // We will try to stitch the file together using partusdtopology.
                    manifestPath = _createFileName(cacheDir, match_1, match_3, _extensionList.front());
                    std::stringstream ss;
                    ss << "partusdtopology " << "\"" << cacheDir << _clearRegex(match_1) << "[0-9]+"
                       << _clearRegex(match_3) << match_4 << "\" \"" << manifestPath << "\"";
                    std::cerr << "Runing command : " << ss.str() << std::endl;
                    const auto ret = system(ss.str().c_str());
                    if (ret != 0 || !TfIsFile(manifestPath)) {
                        TF_WARN("Manifest file does not exists and/or can't be created. %s", manifestPath.c_str());
                        return;
                    }
                }
                if (!boost::regex_search(cachePrefix.c_str(), match, _frameRe)) { return; }
                std::stringstream ss; ss << cacheDir
                                         << match[1].str()
                                         << "%0"
                                         << match[2].str().length()
                                         << "d"
                                         << match[3].str();
                mFormatString = ss.str();
                mIsValid = true;
                mUsdPrim.GetReferences().AppendReference(SdfReference(manifestPath));
                UsdClipsAPI clips(mUsdPrim);
                clips.SetClipManifestAssetPath(SdfAssetPath(manifestPath));
                clips.SetClipPrimPath("/points");
                mPathBuffer.resize(mFormatString.length() + 1, '\0');
                // We can't reserve anything here, because these parameters are only available in an internal, Luma branch.
                // const auto numberOfFrames = std::max(1ul, static_cast<size_t>(getArgs().endTime - getArgs().startTime));
                // mClipTimes.reserve(numberOfFrames);
                // mClipAssetPaths.reserve(numberOfFrames);
                // mClipActive.reserve(numberOfFrames);

            } else {
                const auto cacheFile = cacheDir + cachePrefix;
                TF_WARN("%s is not a valid path to a PartIO cache.", cacheFile.c_str());
            }
        }

        void write(const UsdTimeCode& usdTime) override {
            if (usdTime.IsDefault()) { return; }

            UsdGeomPoints points(mUsdPrim);
            writeTransformAttrs(usdTime, points);

            MFnDagNode dagNode(getDagPath());
            const auto frame = dagNode.findPlug("time").asInt();

            sprintf(mPathBuffer.data(), mFormatString.c_str(), frame);
            SdfAssetPath assetPath(mPathBuffer.data());
            if (!TfIsFile(assetPath.GetAssetPath())) { return; }

            // We are taking the simplest route here to handle all the cases.
            // Honestly, I can't be bothered to optimize this, since this is such a miniscule operation
            // compared to the path validity checks and other calls we are making.
            const auto timeValue = usdTime.GetValue();
            mClipTimes.push_back(GfVec2d(timeValue, timeValue));
            const auto it = std::find(mClipAssetPaths.begin(), mClipAssetPaths.end(), assetPath);
            if (it == mClipAssetPaths.end()) {
                mClipActive.push_back(GfVec2d(timeValue, static_cast<double>(mClipAssetPaths.size())));
                mClipAssetPaths.push_back(assetPath);
            } else {
                mClipActive.push_back(GfVec2d(timeValue, static_cast<double>(std::distance(mClipAssetPaths.begin(), it))));
            }
        }

        ~partioVisualizerWriter() {
            if (mIsValid) {
                UsdClipsAPI clips(mUsdPrim);
                clips.SetClipTimes(mClipTimes);
                clips.SetClipActive(mClipActive);
                clips.SetClipAssetPaths(mClipAssetPaths);
            }
        };

        VtVec2dArray mClipTimes;
        VtVec2dArray mClipActive;
        VtArray<SdfAssetPath> mClipAssetPaths;
        std::string mFormatString;
        std::vector<char> mPathBuffer;
        bool mIsValid;
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
