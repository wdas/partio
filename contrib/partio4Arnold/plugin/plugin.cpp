#include "PartioVizTranslator.h"

#include "extension/Extension.h"

extern "C"
{

DLLEXPORT void initializeExtension(CExtension& extension)
{
    MStatus status;

    extension.Requires("partio4Maya");
    //extension.LoadArnoldPlugin("partioProcedural");
    status = extension.RegisterTranslator("partioVisualizer",
                                          "",
                                          CPartioVizTranslator::creator,
                                          CPartioVizTranslator::NodeInitializer);
}

DLLEXPORT void deinitializeExtension(CExtension& extension)
{
}

}

