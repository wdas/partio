using attr_names_t = std::vector<std::string>;

const attr_names_t _positionNames = {"position", "Position", "P"};
const attr_names_t _velocityNames = {"velocity", "Velocity", "v", "vel"};
const attr_names_t _radiusNames = {"radius", "radiusPP", "pscale", "scale", "scalePP"};
const attr_names_t _scaleNames = {"scale", "scalePP"};
const attr_names_t _idNames = {"id", "particleId", "ID", "Id"};
const SdfPath _pointsPath("/points");

inline
bool _findAttrName (const attr_names_t& names, const std::string& attrName) {
    return std::find(names.begin(), names.end(), attrName) != names.end();
};

inline
bool _isBuiltinAttribute(const std::string& attrName) {
    auto _findInVec = [&attrName] (const attr_names_t& names) -> bool {
        return _findAttrName(names, attrName);
    };
    return _findInVec(_positionNames) ||
           _findInVec(_velocityNames) ||
           _findInVec(_radiusNames) ||
           _findInVec(_scaleNames) ||
           _findInVec(_idNames);
}

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
