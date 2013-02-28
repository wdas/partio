import pymel.core as pm
import mtoa.ui.ae.templates as templates
from mtoa.ui.ae.templates import AttributeTemplate, registerTranslatorUI


class PartioVisualizerTemplate(templates.ShapeTranslatorTemplate):
    def setup(self):
        self.commonShapeAttributes()
        self.addSeparator()
        self.addControl("aiRenderPointsAs", label="Render Points As")
        self.addControl("aiOverrideRadiusPP", label="Override RadiusPP")
        self.addControl("aiRadiusMultiplier", label="Radius Multiplier")
        self.addControl("aiMaxParticleRadius", label= "Clamp Max Radius")
        self.addSeparator()
        self.addControl("aiStepSize",label="Volume Step Size")
        self.addSeparator()
        self.addControl("aiMotionBlurMultiplier",label="Motion Blur Mult")

templates.registerTranslatorUI(PartioVisualizerTemplate, "partioVisualizer", "partio")