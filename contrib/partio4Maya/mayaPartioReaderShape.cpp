#include <maya/MIOStream.h>
#include <math.h>
#include <stdlib.h>

//#include <cfloat>


#include <maya/MPoint.h>
#include <maya/MPlug.h>

#include <maya/MDagPath.h>
#include <maya/MMaterial.h>

#if defined(OSMac_MachO_)
#include <OpenGL/glu.h>
#else
#include <GL/glu.h>
#endif


#include "mayaPartioReaderShape.h"

#include "Partio.h"

#include <maya/MVectorArray.h>
#include <maya/MDoubleArray.h>
#include <maya/MIntArray.h>
#include <maya/MMatrix.h>

#include <sstream>


#define _USE_MGL_FT_
#include <maya/MGLFunctionTable.h>
static MGLFunctionTable *gGLFT = NULL;

#define LEAD_COLOR				18	// green
#define ACTIVE_COLOR			15	// white
#define ACTIVE_AFFECTED_COLOR	8	// purple
#define DORMANT_COLOR			4	// blue
#define HILITE_COLOR			17	// pale blue

MTypeId mayaPartioReaderShape::id(0x70132);

//CStaticAttrHelper mayaPartioReaderShape::s_attributes(mayaPartioReaderShape::addAttribute);

MObject mayaPartioReaderShape::s_particleCache;
MObject mayaPartioReaderShape::s_decimation;
MObject mayaPartioReaderShape::s_mode;
MObject mayaPartioReaderShape::s_sphereRadius;
MObject mayaPartioReaderShape::s_sphereTesselatation;
MObject mayaPartioReaderShape::s_useFrameExtension;
MObject mayaPartioReaderShape::s_frameNumber;
MObject mayaPartioReaderShape::s_useSubFrame;
MObject mayaPartioReaderShape::s_frameOffset;
MObject mayaPartioReaderShape::s_updateNode;

CPartioReaderCache::CPartioReaderCache()
{

    cacheFile = "";
    decimation = 1;
    useFrameExtension = false;
    fps = 25.0f;
    numParticles = 0;
    mode = 0;
    sphereSize = 0.2f;
    sphereTesselatation = 6;
    bbox = MBoundingBox(MPoint(-1.0f, -1.0f, -1.0f), MPoint(1.0f, 1.0f, 1.0f));

    token = 0;
    dList = 0;
}

/*
MString getBaseName(MString pattern, MString file)
{

    MString basename;

    int idx = file.indexW(pattern);

    if (idx != -1)
    {
        basename = file.substring(0, idx);
        return basename;
    }
    else
    {
        return file;
    }

}

MString getFileExtension(MString file)
{

    //std::string filename = std::string(file);
    //std::string extension = "";
    MString extension;
    //std::string::size_type idx;

    int idx = file.rindex('.');

    if (idx != -1)
    {

        extension = file.substring(idx + 1, file.length());
        return extension;
    }
    else
    {
        return extension;
    }

}

MString getFrameFile(MString base, int frame, float fps)
{

    // get extension
    MString ext = getFileExtension(base);
    MString filename;

    if (ext == "pdc")
    {
        // maya weird timecode for pdc.
        int timecode = (int)(6000 / fps);

        int curFrame = frame * timecode;

        MString baseName = getBaseName(".", base);

        //		ss << baseName << "." << curFrame << "." << ext;
        //		ss >> baseName;
        filename = baseName + curFrame + "." + ext;

    }
    else if (ext == "mc")
    {
        // maya weird nomenclature for mc.
        MString baseName = getBaseName("Frame", base);
        //		ss << baseName << "Frame" << frame << "." << ext;
        //		ss >> baseName;
        //		filename = baseName;
    }
    else
    {
        // for a normal, pratical case...
        MString baseName = getBaseName(".", base);
        //		ss << baseName << "." << frame << "." << ext;
        //		ss >> baseName;
        //		filename = baseName;
    }
    return filename;
}
*/


MStatus mayaPartioReaderShape::initialize()
{
    MFnTypedAttribute tAttr;
    MFnNumericAttribute nAttr;
    MFnEnumAttribute eAttr;

	s_updateNode = nAttr.create( "updateNode", "un", MFnNumericData::kInt);
	nAttr.setWritable( false );
	addAttribute( s_updateNode );

    s_particleCache = tAttr.create("particleCache", "particleCache",
                                   MFnData::kString);
    tAttr.setHidden(false);
    tAttr.setStorable(true);
    addAttribute(s_particleCache);

    s_decimation = nAttr.create("displayDecimation", "displayDecimation",
                                MFnNumericData::kInt, 1);
    nAttr.setStorable(true);
    nAttr.setKeyable(true);
    addAttribute(s_decimation);

    s_mode = eAttr.create("mode", "mode", 0);
    eAttr.addField("bounding Box", 0);
    eAttr.addField("point Cloud", 1);
    eAttr.addField("spheres", 2);
    addAttribute(s_mode);

    s_sphereRadius = nAttr.create("sphereRadius", "sphereRadius",
                                  MFnNumericData::kFloat, 0.2f);
    nAttr.setStorable(true);
    nAttr.setKeyable(true);
    addAttribute(s_sphereRadius);

    s_sphereTesselatation = nAttr.create("displayTesselation",
                                         "displayTesselation", MFnNumericData::kInt, 6);
    nAttr.setStorable(true);
    nAttr.setKeyable(true);
    addAttribute(s_sphereTesselatation);

    s_frameNumber = nAttr.create("frameNumber", "frameNumber",
                                 MFnNumericData::kFloat, 0);
    nAttr.setStorable(true);
    nAttr.setKeyable(true);
    addAttribute(s_frameNumber);
	attributeAffects(s_frameNumber, s_updateNode);

    s_useSubFrame = nAttr.create("useSubFrame", "useSubFrame",
                                 MFnNumericData::kBoolean, 0);
    nAttr.setHidden(false);
    nAttr.setKeyable(true);
    addAttribute(s_useSubFrame);

    s_frameOffset = nAttr.create("frameOffset", "frameOffset",
                                 MFnNumericData::kFloat, 0);
    nAttr.setStorable(true);
    nAttr.setKeyable(true);
    addAttribute(s_frameOffset);


    return (MS::kSuccess);
}

mayaPartioReaderShape::mayaPartioReaderShape()
{
}

mayaPartioReaderShape::~mayaPartioReaderShape()
{
}

void* mayaPartioReaderShape::creator()
{
    return new mayaPartioReaderShape;
}

/* override */
void mayaPartioReaderShape::postConstructor()
{
    // This call allows the shape to have shading groups assigned
    setRenderable(true);
}

/*
MStatus mayaPartioReaderShape::GetPointsFromCache()
{
    MStatus status;

    mayaPartioReaderShape* nonConstThis = const_cast<mayaPartioReaderShape*>(this);
    CPartioReaderCache* cache = nonConstThis->particleCache();
    MString particleFile = cache->cacheFile;

    cache->bbox.clear();
    Partio::ParticlesDataMutable* p = Partio::read(particleFile.asChar());
    if (p)
    {

        // number of particles
        int numParticles = p->numParticles();
        cache->numParticles = numParticles;

        if (numParticles > 0)
        {

            if (p->attributeInfo("position", cache->positionhandle))
            {

                cache->p = p;

                for (int i = 0; i < numParticles; i++)
                {
                    const float* pos = cache->p->data<float> (
                                           cache->positionhandle, i);

                    cache->bbox.expand(MVector(pos[0], pos[1], pos[2]));
                }

                cache->IsParticleLoaded = true;
                cache->updateView = true;
                status = MS::kSuccess;
                return status;
            }

        }
    }
    cache->IsParticleLoaded = false;
    status = MS::kFailure;
    return status;

}
*/

/*

bool mayaPartioReaderShape::getInternalValueInContext(const MPlug& plug,
        MDataHandle& datahandle, MDGContext &context)
{

    bool isOk = true;

    if (plug == s_particleCache)
    {
        datahandle.set(fGeometry.cacheFile);
        isOk = true;
    }
    else if (plug == s_mode)
    {
        datahandle.set(fGeometry.mode);
        isOk = true;
    }
    else if (plug == s_sphereRadius)
    {
        datahandle.set(fGeometry.sphereSize);
        isOk = true;
    }
    else if (plug == s_useFrameExtension)
    {
        datahandle.set(fGeometry.useFrameExtension);
        isOk = true;
    }
    else if (plug == s_frameNumber)
    {
        datahandle.set(fGeometry.frame);
        isOk = true;
    }
    else if (plug == s_frameOffset)
    {
        datahandle.set(fGeometry.frameOffset);
        isOk = true;
    }
    else
    {
        isOk = MPxSurfaceShape::getInternalValue(plug, datahandle);
    }
    return isOk;
}

bool mayaPartioReaderShape::setInternalValueInContext(const MPlug& plug,
        const MDataHandle& datahandle, MDGContext &context)
{
    bool isOk = true;
    if (plug == s_particleCache)
    {
        isOk = true;
    }
    else if (plug == s_useFrameExtension)
    {
        isOk = true;
    }
    else if (plug == s_mode)
    {
        isOk = true;
    }
    else if (plug == s_sphereRadius)
    {
        isOk = true;
    }
    else if (plug == s_frameNumber)
    {
        isOk = true;
    }
    else if (plug == s_useSubFrame)
    {
        isOk = true;
    }
    else if (plug == s_frameOffset)
    {
        isOk = true;
    }
    else
    {
        isOk = MPxSurfaceShape::setInternalValueInContext(plug, datahandle,
                context);
    }

    return isOk;
}

*/


bool mayaPartioReaderShape::isBounded() const
{
    return true;
}

MBoundingBox mayaPartioReaderShape::boundingBox() const
{
    // Returns the bounding box for the shape.
    mayaPartioReaderShape* nonConstThis =
        const_cast<mayaPartioReaderShape*>(this);
    CPartioReaderCache* geom = nonConstThis->particleCache();

    MPoint min = geom->bbox.min();
    MPoint max = geom->bbox.max();

    MBoundingBox bbox(MPoint(min.x - geom->sphereSize,
                             min.y - geom->sphereSize, min.z - geom->sphereSize), MPoint(max.x
                                     + geom->sphereSize, max.y + geom->sphereSize, max.z
                                     + geom->sphereSize));
    return bbox;

}

MStatus mayaPartioReaderShape::compute(const MPlug& plug, MDataBlock& data)
{
	cout <<  "compute" << endl;
	cout <<  plug.name() << endl;
	if (plug != s_updateNode)
	{
		cout << "outearly" << endl;
        return ( MS::kUnknownParameter );
	}
    return MS::kSuccess;
}

//////////////////////////////////////////////////////////////////
////  getPlugData is a util to update the drawing of the UI stuff

bool mayaPartioReaderShape::GetPlugData()
{
	cout << "get plug data" << endl;

	MStatus stat;

	MObject thisNode = thisMObject();

	int update = 0;

	MPlug updatePlug(thisNode, s_updateNode );
	updatePlug.getValue( update );
	if(update != dUpdate){
		dUpdate = update;
		return true;
	}else{
		return false;
	}


	return false;

}

//
// This function gets the values of all the attributes and
// assigns them to the fGeometry. Calling MPlug::getValue
// will ensure that the values are up-to-date.
//
CPartioReaderCache* mayaPartioReaderShape::particleCache()
{
	GetPlugData(); // force update to run compute function
	fGeometry.updateView = true;
    return &fGeometry;
}

// UI IMPLEMENTATION

CPartioReaderUI::CPartioReaderUI()
{
}
CPartioReaderUI::~CPartioReaderUI()
{
}

void* CPartioReaderUI::creator()
{
    return new CPartioReaderUI();
}



void CPartioReaderUI::getDrawRequests(const MDrawInfo & info,
                                      bool /*objectAndActiveOnly*/, MDrawRequestQueue & queue)
{
    // The draw data is used to pass geometry through the
    // draw queue. The data should hold all the information
    // needed to draw the shape.
    //
    MDrawData data;
    MDrawRequest request = info.getPrototype(*this);
    mayaPartioReaderShape* shapeNode = (mayaPartioReaderShape*) surfaceShape();
	CPartioReaderCache* geom = shapeNode->particleCache();

    getDrawData(geom, data);
    request.setDrawData(data);

    // Are we displaying meshes?
    if (!info.objectDisplayStatus(M3dView::kDisplayLocators))
        return;


	//getDrawRequestsWireFrame(request, info);
    queue.add(request);
	/*
    // Use display status to determine what color to draw the object
    //
    switch (info.displayStyle())
    {
        case M3dView::kWireFrame:
            getDrawRequestsWireFrame(request, info);
            queue.add(request);
            break;

        case M3dView::kGouraudShaded:
            request.setToken(kDrawSmoothShaded);
            getDrawRequestsShaded(request, info, queue, data);
            queue.add(request);
            break;

        case M3dView::kFlatShaded:
            request.setToken(kDrawFlatShaded);
            getDrawRequestsShaded(request, info, queue, data);
            queue.add(request);
            break;
        default:
            break;
    }
    */


}

void CPartioReaderUI::draw(const MDrawRequest & request, M3dView & view) const
{
	cout << "draw" << endl;

    MDrawData data = request.drawData();
    CPartioReaderCache * cache = (CPartioReaderCache*) data.geometry();
    int token = request.token();

    int oldToken = cache->token;

    if (oldToken != token)
        cache->updateView = true;

    bool drawTexture = false;
    view.beginGL();

    if (cache->updateView)
    {

        if (cache->dList == 0)
            cache->dList = gGLFT->glGenLists(2);

        MBoundingBox m_bbox = cache->bbox;
        float minPt[4];
        float maxPt[4];
        m_bbox.min().get(minPt);
        m_bbox.max().get(maxPt);
        float bottomLeftFront[3] = { minPt[0], minPt[1], minPt[2] };
        float topLeftFront[3] = { minPt[0], maxPt[1], minPt[2] };
        float bottomRightFront[3] = { maxPt[0], minPt[1], minPt[2] };
        float topRightFront[3] = { maxPt[0], maxPt[1], minPt[2] };
        float bottomLeftBack[3] = { minPt[0], minPt[1], maxPt[2] };
        float topLeftBack[3] = { minPt[0], maxPt[1], maxPt[2] };
        float bottomRightBack[3] = { maxPt[0], minPt[1], maxPt[2] };
        float topRightBack[3] = { maxPt[0], maxPt[1], maxPt[2] };

        switch (cache->mode)
        {
            case 0:

                gGLFT->glNewList(cache->dList, MGL_COMPILE);
                gGLFT->glBegin(MGL_LINE_STRIP);

                gGLFT->glVertex3fv(bottomLeftFront);
                gGLFT->glVertex3fv(bottomLeftBack);
                gGLFT->glVertex3fv(topLeftBack);
                gGLFT->glVertex3fv(topLeftFront);
                gGLFT->glVertex3fv(bottomLeftFront);
                gGLFT->glVertex3fv(bottomRightFront);
                gGLFT->glVertex3fv(bottomRightBack);
                gGLFT->glVertex3fv(topRightBack);
                gGLFT->glVertex3fv(topRightFront);
                gGLFT->glVertex3fv(bottomRightFront);
                gGLFT->glEnd();

                gGLFT->glBegin(MGL_LINES);
                gGLFT->glVertex3fv(bottomLeftBack);
                gGLFT->glVertex3fv(bottomRightBack);

                gGLFT->glVertex3fv(topLeftBack);
                gGLFT->glVertex3fv(topRightBack);

                gGLFT->glVertex3fv(topLeftFront);
                gGLFT->glVertex3fv(topRightFront);
                gGLFT->glEnd();
                gGLFT->glEndList();
                break;

            case 1:
                gGLFT->glPushAttrib(GL_CURRENT_BIT);
                gGLFT->glEnable(GL_POINT_SMOOTH);
                gGLFT->glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
                gGLFT->glEnable(GL_BLEND);
                gGLFT->glDepthMask(GL_TRUE);
                gGLFT->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                gGLFT->glPointSize(cache->sphereSize * 2);
                gGLFT->glNewList(cache->dList, GL_COMPILE);

				/*
				gGLFT->glBegin(GL_POINTS);
                for (int i = 0; i < cache->numParticles; ++i)
                {
                    const float* position = cache->p->data<float> (
                                                cache->positionhandle, i);

                    gGLFT->glVertex3f(position[0], position[1], position[2]);

                    i = i + (cache->decimation - 1);

                }
                gGLFT->glEnd();
                */
                gGLFT->glEndList();
                gGLFT->glDisable(GL_POINT_SMOOTH);
                gGLFT->glPopAttrib();

                break;

            case 2:

                if ((token == kDrawSmoothShaded) || (token == kDrawFlatShaded))
                {
#if		defined(SGI) || defined(MESA)
                    glEnable(GL_POLYGON_OFFSET_EXT);
#else
                    gGLFT->glEnable(GL_POLYGON_OFFSET_FILL);
#endif
                    // Set up the material
                    //
                    MMaterial material = request.material();
                    material.setMaterial(request.multiPath(),
                                         request.isTransparent());

                    // Enable texturing
                    //
                    drawTexture = material.materialIsTextured();
                    if (drawTexture)
                        glEnable(GL_TEXTURE_2D);

                    // Apply the texture to the current view
                    //
                    if (drawTexture)
                    {
                        material.applyTexture(view, data);
                    }
                }

                GLUquadricObj *quadratic = gluNewQuadric();

                switch (token)
                {

                    case kDrawWireframe:
                    case kDrawWireframeOnShaded:
                        gluQuadricDrawStyle(quadratic, GLU_LINE);
                        break;

                    case kDrawSmoothShaded:
                        gluQuadricNormals(quadratic, GLU_SMOOTH);
                        gluQuadricTexture(quadratic, true);
                        gluQuadricDrawStyle(quadratic, GLU_FILL);
                        break;

                    case kDrawFlatShaded:
                        gluQuadricNormals(quadratic, GLU_FLAT);
                        gluQuadricTexture(quadratic, true);
                        gluQuadricDrawStyle(quadratic, GLU_FILL);
                        break;
                }

                gGLFT->glNewList(2, MGL_COMPILE);

                gluSphere(quadratic, cache->sphereSize, cache->sphereTesselatation,
                          cache->sphereTesselatation);
                gGLFT->glEndList();

                gGLFT->glNewList(cache->dList, MGL_COMPILE);
				/*
                for (int i = 0; i < cache->numParticles; ++i)
                {
                    const float* position = cache->p->data<float> (
                                                cache->positionhandle, i);
                    gGLFT->glPushMatrix();
                    gGLFT->glTranslatef(position[0], position[1], position[2]);
                    gGLFT->glCallList(2);
                    gGLFT->glPopMatrix();
                    i = i + (cache->decimation) - 1;

                }
                */

                gGLFT->glEndList();

                if (drawTexture)
                    gGLFT->glDisable(GL_TEXTURE_2D);
                gGLFT->glEnd();

                break;

        }
        cache->updateView = false;

    }
    gGLFT->glCallList(cache->dList);
    view.endGL();
}

void CPartioReaderUI::getDrawRequestsWireFrame(MDrawRequest& request,
        const MDrawInfo& info)
{

    request.setToken(kDrawWireframe);
    M3dView::DisplayStatus displayStatus = info.displayStatus();
    M3dView::ColorTable activeColorTable = M3dView::kActiveColors;
    M3dView::ColorTable dormantColorTable = M3dView::kDormantColors;
    switch (displayStatus)
    {
        case M3dView::kLead:
            request.setColor(LEAD_COLOR, activeColorTable);
            break;
        case M3dView::kActive:
            request.setColor(ACTIVE_COLOR, activeColorTable);
            break;
        case M3dView::kActiveAffected:
            request.setColor(ACTIVE_AFFECTED_COLOR, activeColorTable);
            break;
        case M3dView::kDormant:
            request.setColor(DORMANT_COLOR, dormantColorTable);
            break;
        case M3dView::kHilite:
            request.setColor(HILITE_COLOR, activeColorTable);
            break;
        default:
            break;
    }

}

/*
void CPartioReaderUI::getDrawRequestsShaded(MDrawRequest& request,
        const MDrawInfo& info, MDrawRequestQueue& queue, MDrawData& data)
{

    // Need to get the material info

    MDagPath path = info.multiPath(); // path to your dag object
    M3dView view = info.view();
    ; // view to draw to
    MMaterial material = MPxSurfaceShapeUI::material(path);
    M3dView::DisplayStatus displayStatus = info.displayStatus();

    // Evaluate the material and if necessary, the texture.
    //
    if (!material.evaluateMaterial(view, path))
    {
        cerr << "Couldnt evaluate\n";
    }

    bool drawTexture = true;
    if (drawTexture && material.materialIsTextured())
    {
        material.evaluateTexture(data);
    }

    request.setMaterial(material);

    bool materialTransparent = false;
    material.getHasTransparency(materialTransparent);
    if (materialTransparent)
    {
        request.setIsTransparent(true);
    }

    // create a draw request for wireframe on shaded if
    // necessary.
    //
    if ((displayStatus == M3dView::kActive)
            || (displayStatus == M3dView::kLead) || (displayStatus
                    == M3dView::kHilite))
    {

        MDrawRequest wireRequest = info.getPrototype(*this);
        wireRequest.setDrawData(data);
        getDrawRequestsWireFrame(wireRequest, info);
        wireRequest.setToken(kDrawWireframeOnShaded);
        wireRequest.setDisplayStyle(M3dView::kWireFrame);
        queue.add(wireRequest);
    }
}
*/

bool CPartioReaderUI::select(MSelectInfo &selectInfo,
                             MSelectionList &selectionList, MPointArray &worldSpaceSelectPts) const
//
// Select function. Gets called when the bbox for the object is selected.
// This function just selects the object without doing any intersection tests.
//
{
    MSelectionMask priorityMask(MSelectionMask::kSelectObjectsMask);
    MSelectionList item;
    item.add(selectInfo.selectPath());
    MPoint xformedPt;
    selectInfo.addSelection(item, xformedPt, selectionList,
                            worldSpaceSelectPts, priorityMask, false);
    return true;
}

