//
// rayUI.h
//
// The header file for the UI part
//

#ifndef __rayUI_h__
#define __rayUI_h__

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Value_Slider.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Button.H>

#include <FL/Fl_Light_Button.H>

#include <FL/fl_file_chooser.H>		// FLTK file chooser

#include "TraceGLWindow.h"

class TraceUI
{
public:
	TraceUI();

	// The FLTK widgets
	Fl_Window* m_mainWindow;
	Fl_Menu_Bar* m_menubar;

	Fl_Slider* m_sizeSlider;
	Fl_Slider* m_depthSlider;

	Fl_Slider* m_attenConstantSlide;
	Fl_Slider* m_attenLinearSlide;
	Fl_Slider* m_attenQuadricSlide;
	Fl_Slider* m_ambientLightSlide;
	Fl_Slider* m_thresholdSlide;

	Fl_Slider* m_numOfSupPixel;

	Fl_Slider* m_focalLength;
	Fl_Slider* m_apertureSize;

	Fl_Button* m_renderButton;
	Fl_Button* m_stopButton;

	TraceGLWindow* m_traceGlWindow;

	Fl_Light_Button* m_softShadow;
	Fl_Light_Button* m_motionBlur;
	Fl_Light_Button* m_glossyReflection;
	Fl_Light_Button* m_dof;
	Fl_Light_Button* m_adaptiveSampling;
	Fl_Light_Button* m_adaptiveIllustrate;
	Fl_Light_Button* m_jitter;
	Fl_Light_Button* m_octTree;
	Fl_Light_Button* m_enableCaustics;

	Fl_Light_Button* m_boxEmissionTexture;
	Fl_Light_Button* m_boxDiffuseTexture;
	Fl_Light_Button* m_boxSpecularTexture;
	Fl_Light_Button* m_boxOpacity;

	// member functions
	void show();

	void setRayTracer(RayTracer* tracer);

	int getSize();
	int getDepth();
	const double& getNumOfSupPixel() const;
	const double& getThreshold() const;
	const double& getAttenConst() const;
	const double& getAttenLinear() const;
	const double& getAttenQuad() const;
	char getIsOctTree() const;

private:
	RayTracer* raytracer;

	int m_nSize;
	int m_nDepth;

	double m_nAttenConstant;
	double m_nAttenLinear;
	double m_nAttenQuadric;
	double m_nAmbientLight;
	double m_nThreshold;
	double m_nNumOfSupPixel;
	double m_nFocalLength;
	double m_nApertureSize;

	// static class members
	static Fl_Menu_Item menuitems[];

	static TraceUI* whoami(Fl_Menu_* o);

	static void cb_load_scene(Fl_Menu_* o, void* v);
	static void cb_save_image(Fl_Menu_* o, void* v);
	static void cb_exit(Fl_Menu_* o, void* v);
	static void cb_about(Fl_Menu_* o, void* v);

	static void cb_emission_image(Fl_Menu_* o, void* v);
	static void cb_diffuse_image(Fl_Menu_* o, void* v);
	static void cb_specular_image(Fl_Menu_* o, void* v);
	static void cb_opacity_image(Fl_Menu_* o, void* v);

	static void cb_exit2(Fl_Widget* o, void* v);

	static void cb_sizeSlides(Fl_Widget* o, void* v);
	static void cb_depthSlides(Fl_Widget* o, void* v);

	static void cb_attenConstantSlides(Fl_Widget* o, void* v);
	static void cb_attenLinearSlides(Fl_Widget* o, void* v);
	static void cb_attenQuadricSlides(Fl_Widget* o, void* v);
	static void cb_ambientLightSlides(Fl_Widget* o, void* v);
	static void cb_thresholdSlides(Fl_Widget* o, void* v);
	static void cb_numOfSupPixelSlides(Fl_Widget* o, void* v);
	static void cb_focalLengthSlides(Fl_Widget* o, void* v);
	static void cb_apertureSizeSlides(Fl_Widget* o, void* v);

	static void cb_render(Fl_Widget* o, void* v);
	static void cb_stop(Fl_Widget* o, void* v);
};

#endif
