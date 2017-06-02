#include <Partio.h>

#include <unistd.h>
#include <dirent.h>

#include <boost/regex.hpp>

#include <iostream>
#include <array>
#include <tuple>

#include <pxr/pxr.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdGeom/points.h>
#include <pxr/usd/usdGeom/primvar.h>
#include <pxr/usd/usdGeom/tokens.h>

PXR_NAMESPACE_USING_DIRECTIVE

void printHelp() {
    std::cout << R"HELP(
Generator for a topology file that can be used to feed USD clips.

Usage : partusdtopology [--help] inputFiles outputFile

Arguments
    inputFiles  Path to the particle file. Supports regex in the filename for multiple matches.
    outputfile  Where to output the topology file.

Optional Arguments:
    --help      Print the help message.
)HELP";
}

#include "../fileFormat/constants.inl"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printHelp();
        return 1;
    }

    if (strcmp(argv[1], "--help") == 0) {
        printHelp();
        return 0;
    }

    if (argc < 3) {
        std::cerr << "Invalid number of arguments!" << std::endl;
        printHelp();
        return 1;
    }

    const std::string inputFiles = argv[1];
    const std::string outputFile = argv[2];

    const size_t lastSep = inputFiles.rfind('/');

    std::string filePattern;
    std::string workingDirectory;

    if (lastSep == std::string::npos) {
        char* cwd = getcwd(nullptr, FILENAME_MAX);
        if (cwd == nullptr) {
            std::cerr << "Can't get current working director!" << std::endl;
            return 1;
        }
        workingDirectory = cwd;
        if (workingDirectory.back() != '/') { workingDirectory += "/"; }
        filePattern = inputFiles;
        free(cwd);
    } else {
        workingDirectory = inputFiles.substr(0, lastSep + 1);
        filePattern = inputFiles.substr(lastSep + 1);
    }

    auto* dp = opendir(workingDirectory.c_str());
    if (dp == nullptr) {
        std::cerr << "Can't open directory " << workingDirectory << std::endl;
        return 1;
    }

    auto stage = UsdStage::CreateNew(outputFile);
    auto points = UsdGeomPoints::Define(stage, SdfPath("/points"));
    stage->SetDefaultPrim(points.GetPrim());

    auto read_file = [&points](const std::string filename) {
        auto* headers = PARTIO::readHeaders(filename.c_str());
        if (headers == nullptr) {
            std::cerr << "Can't read the headers of " << filename << " . Falling back to opening the file." << std::endl;
            headers = PARTIO::read(filename.c_str());
            if (headers == nullptr) {
                std::cerr << "Can't read particle file " << filename << std::endl;
                return;
            }
        }

        const auto numAttrs = headers->numAttributes();

        for (auto i = decltype(numAttrs){0}; i < numAttrs; ++i) {
            attr_t attr;
            if (!headers->attributeInfo(i, attr)) { continue; }
            if (_findAttrName(_idNames, attr.name) && _attributeIsType<PARTIO::INT>(attr)) {
                points.CreateIdsAttr();
            } else if (_findAttrName(_positionNames, attr.name) && _attributeIsType<PARTIO::VECTOR>(attr)) {
                points.CreatePointsAttr();
            } else if (_findAttrName(_velocityNames, attr.name) && _attributeIsType<PARTIO::VECTOR>(attr)) {
                points.CreateVelocitiesAttr();
            } else if (_findAttrName(_radiusNames, attr.name) && _attributeIsType<PARTIO::FLOAT>(attr)) {
                points.CreateWidthsAttr();
            } else if (_findAttrName(_scaleNames, attr.name) && _attributeIsType<PARTIO::VECTOR>(attr)) {
                points.CreateWidthsAttr();
            } else { // primvar
                auto _createPrimVar = [&points, &attr] (const SdfValueTypeName& typeName) {
                    const static std::string _primvars("primvars:");
                    auto a = points.GetPrim().CreateAttribute(TfToken(_primvars + attr.name), typeName, false, SdfVariabilityVarying);
                    UsdGeomPrimvar(a).SetInterpolation(UsdGeomTokens->vertex);
                };
                if (_attributeIsType<PARTIO::INT>(attr)) {
                    _createPrimVar(SdfValueTypeNames->IntArray);
                } else if (_attributeIsType<PARTIO::FLOAT>(attr)) {
                    _createPrimVar(SdfValueTypeNames->FloatArray);
                } else if (_attributeIsType<PARTIO::VECTOR>(attr)) {
                    _createPrimVar(SdfValueTypeNames->Vector3fArray);
                }  else if (attr.type == PARTIO::FLOAT && attr.count == 4) {
                    _createPrimVar(SdfValueTypeNames->Float4Array);
                }
            }
        }

        headers->release();
    };

    boost::regex re(filePattern.c_str());
    struct dirent* dirp;
    while ((dirp = readdir(dp)) != nullptr) {
        if (boost::regex_match(dirp->d_name, re)) {
            read_file(workingDirectory + dirp->d_name);
        }
    }

    stage->Save();

    return 0;
}
