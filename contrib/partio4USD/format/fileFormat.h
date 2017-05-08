#ifndef USDPARTIO_FORMAT_H
#define USDPARTIO_FORMAT_H

#include <pxr/pxr.h>
#include <pxr/usd/sdf/fileFormat.h>
#include <pxr/base/tf/staticTokens.h>

#include <string>
#include <iostream>

PXR_NAMESPACE_OPEN_SCOPE

#define USDPARTIO_FILE_FORMAT_TOKENS  \
    ((Id,      "pio"))                \
    ((Version, "1.0"))                \
    ((Target,  "usd"))

TF_DECLARE_PUBLIC_TOKENS(UsdPartIOFileFormatTokens, USDPARTIO_FILE_FORMAT_TOKENS);

TF_DECLARE_WEAK_AND_REF_PTRS(UsdPartIOFileFormat);
TF_DECLARE_WEAK_AND_REF_PTRS(SdfLayerBase);

class UsdPartIOFileFormat : public SdfFileFormat {
public:
    virtual bool CanRead(const std::string &file) const override;
    virtual bool Read(const SdfLayerBasePtr& layerBase,
                      const std::string& resolvedPath,
                      bool metadataOnly) const override;
protected:
    SDF_FILE_FORMAT_FACTORY_ACCESS;

    virtual ~UsdPartIOFileFormat();
    UsdPartIOFileFormat();

    virtual bool _IsStreamingLayer(const SdfLayerBase& layer) const override {
        return false;
    };
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif //USDPARTIO_FORMAT_H
