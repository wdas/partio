#ifndef PARTIOVIZTRANSLATOR_H
#define PARTIOVIZTRANSLATOR_H

#include "translators/shape/ShapeTranslator.h"

class CPartioVizTranslator : public CShapeTranslator
{
public:
    static void* creator();

    virtual AtNode* CreateArnoldNodes();
    //virtual void ProcessRenderFlags(AtNode* node);

    static void NodeInitializer ( CAbTranslator context );
    void Export ( AtNode* anode );
    void ExportMotion ( AtNode* anode, unsigned int step );
    virtual void Update ( AtNode* anode );
    virtual void UpdateMotion ( AtNode* anode, uint step );

protected:
    CPartioVizTranslator()  :
        CShapeTranslator()
    {
        // Just for debug info, translator creates whatever arnold nodes are required
        // through the CreateArnoldNodes method
        m_abstract.arnold = "procedural";
		
    }
    void ExportBoundingBox ( AtNode* procedural );

    void ExportPartioVizShaders ( AtNode* procedural );
    virtual void ExportShaders();

    AtNode* ExportInstance ( AtNode *instance, const MDagPath& masterInstance );
    AtNode* ExportProcedural ( AtNode* procedural, bool update );
    bool fileCacheExists ( const char* fileName );


    MFnDagNode m_DagNode;
	MString m_customAttrs;
};

#endif // PARTIOVIZTRANSLATOR_H
