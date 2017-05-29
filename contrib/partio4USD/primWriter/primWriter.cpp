#include <pxr/pxr.h>

#include <usdMaya/primWriterRegistry.h>
#include <usdMaya/MayaTransformWriter.h>

PXR_NAMESPACE_USING_DIRECTIVE

__attribute__((constructor))
static void doSomeStuff() {
    std::cerr << "I'm doing stuff!" << std::endl;
}

namespace {
    class partioVisualizerWriter : public MayaTransformWriter {
    public:
        partioVisualizerWriter(const MDagPath & iDag, const SdfPath& uPath, bool instanceSource, usdWriteJobCtx& jobCtx) :
            MayaTransformWriter(iDag, uPath, instanceSource, jobCtx) {
        }
        ~partioVisualizerWriter() {};
    };

    using partioVisualizerWriterPtr = std::shared_ptr<partioVisualizerWriter>;
}

TF_REGISTRY_FUNCTION_WITH_TAG(PxrUsdMayaPrimWriterRegistry, partioVisualizerWriter) {
    std::cerr << "Registering partio visualizer writer." << std::endl;
    PxrUsdMayaPrimWriterRegistry::Register("partioVisualizerWriter",
                 [](const MDagPath& iDag,
                    const SdfPath& uPath, bool instanceSource,
                    usdWriteJobCtx& jobCtx) -> MayaPrimWriterPtr { return std::make_shared<partioVisualizerWriter>(
                     iDag, uPath, instanceSource, jobCtx); });
}
