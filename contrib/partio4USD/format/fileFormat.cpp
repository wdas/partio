#include "fileFormat.h"

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

UsdPartIOFileFormat::~UsdPartIOFileFormat() {

}

bool UsdPartIOFileFormat::CanRead(const std::string &file) const {
    return true;
}
bool UsdPartIOFileFormat::Read(const SdfLayerBasePtr& layerBase,
                               const std::string& resolvedPath,
                               bool metadataOnly) const {
    return false;
}

bool UsdPartIOFileFormat::ReadFromString(const SdfLayerBasePtr& layerBase,
                                         const std::string& str) const {
    return false;
}

PXR_NAMESPACE_CLOSE_SCOPE
