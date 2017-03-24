#ifndef PARTIOVIZTRANSLATOR_H
#define PARTIOVIZTRANSLATOR_H

#include "translators/shape/ShapeTranslator.h"

class CPartioVizTranslator : public CShapeTranslator {
public:
    static void* creator();

    virtual AtNode* CreateArnoldNodes();

    static void NodeInitializer(CAbTranslator context);

    void Export(AtNode* anode);

#ifdef MTOA12
    void ExportMotion(AtNode* anode, unsigned int step);
    virtual void Update(AtNode* anode);
    virtual void UpdateMotion(AtNode* anode, uint step);
#elif MTOA14
    void ExportMotion(AtNode* anode);
#endif

protected:
    CPartioVizTranslator() : CShapeTranslator() { }

    void ExportPartioVizShaders(AtNode* procedural);

    virtual void ExportShaders();

    AtNode* ExportInstance(AtNode* instance, const MDagPath& masterInstance);

    AtNode* ExportProcedural(AtNode* procedural, bool update);

    bool fileCacheExists(const char* fileName);

    MFnDagNode m_DagNode;
    MString m_customAttrs;
};

#endif // PARTIOVIZTRANSLATOR_H
