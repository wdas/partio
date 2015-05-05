import pymel.core as pm
import mtoa.ui.ae.templates as templates
from mtoa.ui.ae.templates import AttributeTemplate, registerTranslatorUI

class PartioVisualizerTemplate(templates.ShapeTranslatorTemplate):

    def PartioRemapCreate(self, attrName):
        pm.setUITemplate('attributeEditorTemplate', pst=True)
        pm.gradientControl('aiRemapRadiusCurveCtrl', at=attrName)
        pm.setUITemplate(ppt=True)

    def PartioRemapUpdate(self, attrName):
        pm.gradientControl('aiRemapRadiusCurveCtrl', edit=True, at=attrName)

    def setup(self):
        self.commonShapeAttributes()
        self.addSeparator()
        self.addControl("aiExportAttributes", label="Export Extra PP attrs")
        self.addSeparator()
        self.addControl("aiRenderPointsAs", label="Render Points As")
        self.addControl("aiOverrideRadiusPP", label="Override RadiusPP")
        self.addControl("aiRadiusMultiplier", label="Radius Multiplier")
        self.addControl("aiMaxParticleRadius", label="Clamp Max Radius")        
        self.beginLayout(label='Remap Radius', collapse=True)
        self.addCustom('aiRemapRadiusCurve', self.PartioRemapCreate, self.PartioRemapUpdate)
        self.endLayout()
        self.addSeparator()
        self.addControl("aiStepSize",label="Volume Step Size")
        self.addSeparator()
        self.addControl("aiMotionBlurMultiplier",label="Motion Blur Mult")
        self.addSeparator()
        self.addControl("aiOverrideProcedural", label="Override render procedural" )
        self.addControl("aiTranslator", label= "Override MTOA Translator")
        self.beginLayout(label='Debug', collapse=True)
        self.addControl("aiMinParticleRadius", label="Clamp Min Radius")
        self.addControl("aiFilterSmallParticles", label="Filter Small Particles")
        self.endLayout()

templates.registerTranslatorUI(PartioVisualizerTemplate, "partioVisualizer", "partioTranslator")