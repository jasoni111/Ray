//
// TraceUI.h
//
// Handles FLTK integration and other user interface tasks
//
#include <stdio.h>
#include <time.h>
#include <string.h>

#include <FL/fl_ask.h>

#include "TraceUI.h"
#include "../RayTracer.h"

static bool done;

//------------------------------------- Help Functions --------------------------------------------
TraceUI* TraceUI::whoami(Fl_Menu_* o)	// from menu item back to UI itself
{
	return ( (TraceUI*)(o->parent()->user_data()) );
}

//--------------------------------- Callback Functions --------------------------------------------
void TraceUI::cb_load_scene(Fl_Menu_* o, void* v) 
{
	TraceUI* pUI=whoami(o);
	
	char* newfile = fl_file_chooser("Open Scene?", "*.ray", NULL );

	if (newfile != NULL) {
		char buf[256];

		if (pUI->raytracer->loadScene(newfile)) {
			sprintf(buf, "Ray <%s>", newfile);
			done=true;	// terminate the previous rendering
		} else{
			sprintf(buf, "Ray <Not Loaded>");
		}

		pUI->m_mainWindow->label(buf);
	}
}

void TraceUI::cb_save_image(Fl_Menu_* o, void* v) 
{
	TraceUI* pUI=whoami(o);
	
	char* savefile = fl_file_chooser("Save Image?", "*.bmp", "save.bmp" );
	if (savefile != NULL) {
		pUI->m_traceGlWindow->saveImage(savefile);
	}
}

void TraceUI::cb_exit(Fl_Menu_* o, void* v)
{
	TraceUI* pUI=whoami(o);

	// terminate the rendering
	done=true;

	pUI->m_traceGlWindow->hide();
	pUI->m_mainWindow->hide();
}

void TraceUI::cb_exit2(Fl_Widget* o, void* v) 
{
	TraceUI* pUI=(TraceUI *)(o->user_data());
	
	// terminate the rendering
	done=true;

	pUI->m_traceGlWindow->hide();
	pUI->m_mainWindow->hide();
}

void TraceUI::cb_about(Fl_Menu_* o, void* v) 
{
	fl_message("RayTracer Project, FLTK version for CS 341 Spring 2002. Latest modifications by Jeff Maurer, jmaurer@cs.washington.edu");
}

void TraceUI::cb_sizeSlides(Fl_Widget* o, void* v)
{
	TraceUI* pUI=(TraceUI*)(o->user_data());
	
	pUI->m_nSize=int( ((Fl_Slider *)o)->value() ) ;
	int	height = (int)(pUI->m_nSize / pUI->raytracer->aspectRatio() + 0.5);
	pUI->m_traceGlWindow->resizeWindow( pUI->m_nSize, height );
}

void TraceUI::cb_depthSlides(Fl_Widget* o, void* v)
{
	((TraceUI*)(o->user_data()))->m_nDepth=int( ((Fl_Slider *)o)->value() ) ;
}

void TraceUI::cb_attenConstantSlides(Fl_Widget* o, void* v)
{
	((TraceUI*)(o->user_data()))->m_nAttenConstant = int(((Fl_Slider*)o)->value());
}

void TraceUI::cb_attenLinearSlides(Fl_Widget* o, void* v)
{
	((TraceUI*)(o->user_data()))->m_nAttenLinear = int(((Fl_Slider*)o)->value());
}

void TraceUI::cb_attenQuadricSlides(Fl_Widget* o, void* v)
{
	((TraceUI*)(o->user_data()))->m_nAttenQuadric = int(((Fl_Slider*)o)->value());
}

void TraceUI::cb_thresholdSlides(Fl_Widget* o, void* v)
{
	((TraceUI*)(o->user_data()))->m_nThreshold = int(((Fl_Slider*)o)->value());
}

void TraceUI::cb_ambientLightSlides(Fl_Widget* o, void* v)
{
	((TraceUI*)(o->user_data()))->m_nAmbientLight = int(((Fl_Slider*)o)->value());
}

void TraceUI::cb_numOfSupPixelSlides(Fl_Widget* o, void* v)
{
	((TraceUI*)(o->user_data()))->m_nNumOfSupPixel = int(((Fl_Slider*)o)->value());
}

void TraceUI::cb_focalLengthSlides(Fl_Widget* o, void* v)
{
	((TraceUI*)(o->user_data()))->m_nFocalLength = int(((Fl_Slider*)o)->value());
}

void TraceUI::cb_apertureSizeSlides(Fl_Widget* o, void* v)
{
	((TraceUI*)(o->user_data()))->m_nApertureSize = int(((Fl_Slider*)o)->value());
}

void TraceUI::cb_render(Fl_Widget* o, void* v)
{
	char buffer[256];

	TraceUI* pUI=((TraceUI*)(o->user_data()));
	
	if (pUI->raytracer->sceneLoaded()) {
		int width=pUI->getSize();
		int	height = (int)(width / pUI->raytracer->aspectRatio() + 0.5);
		pUI->m_traceGlWindow->resizeWindow( width, height );

		pUI->m_traceGlWindow->show();

		pUI->raytracer->traceSetup(width, height);
		
		// Save the window label
		const char *old_label = pUI->m_traceGlWindow->label();

		// start to render here	
		done=false;
		clock_t prev, now;
		prev=clock();
		
		pUI->m_traceGlWindow->refresh();
		Fl::check();
		Fl::flush();

		for (int y=0; y<height; y++) {
			for (int x=0; x<width; x++) {
				if (done) break;
				
				// current time
				now = clock();

				// check event every 1/2 second
				if (((double)(now-prev)/CLOCKS_PER_SEC)>0.5) {
					prev=now;

					if (Fl::ready()) {
						// refresh
						pUI->m_traceGlWindow->refresh();
						// check event
						Fl::check();

						if (Fl::damage()) {
							Fl::flush();
						}
					}
				}

				pUI->raytracer->max_depth = pUI->getDepth();
				pUI->raytracer->tracePixel( x, y );
		
			}
			if (done) break;

			// flush when finish a row
			if (Fl::ready()) {
				// refresh
				pUI->m_traceGlWindow->refresh();

				if (Fl::damage()) {
					Fl::flush();
				}
			}
			// update the window label
			sprintf(buffer, "(%d%%) %s", (int)((double)y / (double)height * 100.0), old_label);
			pUI->m_traceGlWindow->label(buffer);
			
		}
		done=true;
		pUI->m_traceGlWindow->refresh();

		// Restore the window label
		pUI->m_traceGlWindow->label(old_label);		
	}
}

void TraceUI::cb_stop(Fl_Widget* o, void* v)
{
	done=true;
}

void TraceUI::show()
{
	m_mainWindow->show();
}

void TraceUI::setRayTracer(RayTracer *tracer)
{
	raytracer = tracer;
	m_traceGlWindow->setRayTracer(tracer);
}

int TraceUI::getSize()
{
	return m_nSize;
}

int TraceUI::getDepth()
{
	return m_nDepth;
}

// menu definition
Fl_Menu_Item TraceUI::menuitems[] = {
	{ "&File",		0, 0, 0, FL_SUBMENU },
		{ "&Load Scene...",	FL_ALT + 'l', (Fl_Callback *)TraceUI::cb_load_scene },
		{ "&Save Image...",	FL_ALT + 's', (Fl_Callback *)TraceUI::cb_save_image },
		{ "&Exit",			FL_ALT + 'e', (Fl_Callback *)TraceUI::cb_exit },
		{ 0 },

	{ "&Help",		0, 0, 0, FL_SUBMENU },
		{ "&About",	FL_ALT + 'a', (Fl_Callback *)TraceUI::cb_about },
		{ 0 },

	{ 0 }
};

TraceUI::TraceUI() {
	// init.
	m_nDepth = 0;
	m_nSize = 150;

	m_nAttenConstant = 0;
	m_nAttenLinear = 0;
	m_nAttenQuadric = 0;
	m_nAmbientLight = 0;
	m_nThreshold = 0;
	m_nNumOfSupPixel = 1;
	m_nFocalLength = 5;
	m_nApertureSize = 5;

	m_mainWindow = new Fl_Window(100, 40, 340, 480, "Ray <Not Loaded>");
	m_mainWindow->user_data((void*)(this));	// record self to be used by static callback functions
	// install menu bar
	m_menubar = new Fl_Menu_Bar(0, 0, 320, 25);
	m_menubar->menu(menuitems);

	int h = 5;

	// install slider depth
	m_depthSlider = new Fl_Value_Slider(10, h+=25, 180, 20, "Depth");
	m_depthSlider->user_data((void*)(this));	// record self to be used by static callback functions
	m_depthSlider->type(FL_HOR_NICE_SLIDER);
    m_depthSlider->labelfont(FL_COURIER);
    m_depthSlider->labelsize(12);
	m_depthSlider->minimum(0);
	m_depthSlider->maximum(10);
	m_depthSlider->step(1);
	m_depthSlider->value(m_nDepth);
	m_depthSlider->align(FL_ALIGN_RIGHT);
	m_depthSlider->callback(cb_depthSlides);

	// install slider size
	m_sizeSlider = new Fl_Value_Slider(10, h+=25, 180, 20, "Size");
	m_sizeSlider->user_data((void*)(this));	// record self to be used by static callback functions
	m_sizeSlider->type(FL_HOR_NICE_SLIDER);
    m_sizeSlider->labelfont(FL_COURIER);
    m_sizeSlider->labelsize(12);
	m_sizeSlider->minimum(64);
	m_sizeSlider->maximum(512);
	m_sizeSlider->step(1);
	m_sizeSlider->value(m_nSize);
	m_sizeSlider->align(FL_ALIGN_RIGHT);
	m_sizeSlider->callback(cb_sizeSlides);

	// install slider Attenuation, Constant
	m_attenConstantSlide = new Fl_Value_Slider(10, h += 25, 180, 20, "Attenuation, Constant");
	m_attenConstantSlide->user_data((void*)(this));	// record self to be used by static callback functions
	m_attenConstantSlide->type(FL_HOR_NICE_SLIDER);
	m_attenConstantSlide->labelfont(FL_COURIER);
	m_attenConstantSlide->labelsize(12);
	m_attenConstantSlide->minimum(0);
	m_attenConstantSlide->maximum(1);
	m_attenConstantSlide->step(0.01);
	m_attenConstantSlide->value(m_nAttenConstant);
	m_attenConstantSlide->align(FL_ALIGN_RIGHT);
	m_attenConstantSlide->callback(cb_attenConstantSlides);

	// install Attenuation, Linear
	m_attenLinearSlide = new Fl_Value_Slider(10, h += 25, 180, 20, "Attenuation, Linear");
	m_attenLinearSlide->user_data((void*)(this));	// record self to be used by static callback functions
	m_attenLinearSlide->type(FL_HOR_NICE_SLIDER);
	m_attenLinearSlide->labelfont(FL_COURIER);
	m_attenLinearSlide->labelsize(12);
	m_attenLinearSlide->minimum(0);
	m_attenLinearSlide->maximum(1);
	m_attenLinearSlide->step(0.01);
	m_attenLinearSlide->value(m_nAttenLinear);
	m_attenLinearSlide->align(FL_ALIGN_RIGHT);
	m_attenLinearSlide->callback(cb_attenLinearSlides);

	// install Attenuation, Quadric
	m_attenQuadricSlide = new Fl_Value_Slider(10, h += 25, 180, 20, "Attenuation, Quadric");
	m_attenQuadricSlide->user_data((void*)(this));	// record self to be used by static callback functions
	m_attenQuadricSlide->type(FL_HOR_NICE_SLIDER);
	m_attenQuadricSlide->labelfont(FL_COURIER);
	m_attenQuadricSlide->labelsize(12);
	m_attenQuadricSlide->minimum(0);
	m_attenQuadricSlide->maximum(1);
	m_attenQuadricSlide->step(0.01);
	m_attenQuadricSlide->value(m_nAttenQuadric);
	m_attenQuadricSlide->align(FL_ALIGN_RIGHT);
	m_attenQuadricSlide->callback(cb_attenQuadricSlides);

	// install slider Ambient Light
	m_ambientLightSlide = new Fl_Value_Slider(10, h += 25, 180, 20, "Ambient Light");
	m_ambientLightSlide->user_data((void*)(this));	// record self to be used by static callback functions
	m_ambientLightSlide->type(FL_HOR_NICE_SLIDER);
	m_ambientLightSlide->labelfont(FL_COURIER);
	m_ambientLightSlide->labelsize(12);
	m_ambientLightSlide->minimum(0);
	m_ambientLightSlide->maximum(1);
	m_ambientLightSlide->step(0.01);
	m_ambientLightSlide->value(m_nAmbientLight);
	m_ambientLightSlide->align(FL_ALIGN_RIGHT);
	m_ambientLightSlide->callback(cb_ambientLightSlides);

	// install slider Threshold
	m_thresholdSlide = new Fl_Value_Slider(10, h += 25, 180, 20, "Threshold");
	m_thresholdSlide->user_data((void*)(this));	// record self to be used by static callback functions
	m_thresholdSlide->type(FL_HOR_NICE_SLIDER);
	m_thresholdSlide->labelfont(FL_COURIER);
	m_thresholdSlide->labelsize(12);
	m_thresholdSlide->minimum(0);
	m_thresholdSlide->maximum(1);
	m_thresholdSlide->step(0.01);
	m_thresholdSlide->value(m_nThreshold);
	m_thresholdSlide->align(FL_ALIGN_RIGHT);
	m_thresholdSlide->callback(cb_thresholdSlides);

	h += 25;

	// install slider Number of sup-pixels
	m_numOfSupPixel = new Fl_Value_Slider(10, h += 25, 180, 20, "Number of sup-pixels");
	m_numOfSupPixel->user_data((void*)(this));	// record self to be used by static callback functions
	m_numOfSupPixel->type(FL_HOR_NICE_SLIDER);
	m_numOfSupPixel->labelfont(FL_COURIER);
	m_numOfSupPixel->labelsize(12);
	m_numOfSupPixel->minimum(1);
	m_numOfSupPixel->maximum(5);
	m_numOfSupPixel->step(1);
	m_numOfSupPixel->value(m_nNumOfSupPixel);
	m_numOfSupPixel->align(FL_ALIGN_RIGHT);
	m_numOfSupPixel->callback(cb_numOfSupPixelSlides);

	h += 25;
	h += 25;

	// install slider Focal Length
	m_focalLength = new Fl_Value_Slider(10, h += 25, 180, 20, "Focal Length");
	m_focalLength->user_data((void*)(this));	// record self to be used by static callback functions
	m_focalLength->type(FL_HOR_NICE_SLIDER);
	m_focalLength->labelfont(FL_COURIER);
	m_focalLength->labelsize(12);
	m_focalLength->minimum(1);
	m_focalLength->maximum(5);
	m_focalLength->step(0.01);
	m_focalLength->value(m_nFocalLength);
	m_focalLength->align(FL_ALIGN_RIGHT);
	m_focalLength->callback(cb_focalLengthSlides);

	// install slider Aperture Size
	m_apertureSize = new Fl_Value_Slider(10, h += 25, 180, 20, "Aperture Size");
	m_apertureSize->user_data((void*)(this));	// record self to be used by static callback functions
	m_apertureSize->type(FL_HOR_NICE_SLIDER);
	m_apertureSize->labelfont(FL_COURIER);
	m_apertureSize->labelsize(12);
	m_apertureSize->minimum(1);
	m_apertureSize->maximum(5);
	m_apertureSize->step(1);
	m_apertureSize->value(m_nApertureSize);
	m_apertureSize->align(FL_ALIGN_RIGHT);
	m_apertureSize->callback(cb_apertureSizeSlides);

	m_renderButton = new Fl_Button(240, 27, 70, 25, "&Render");
	m_renderButton->user_data((void*)(this));
	m_renderButton->callback(cb_render);

	m_stopButton = new Fl_Button(240, 55, 70, 25, "&Stop");
	m_stopButton->user_data((void*)(this));
	m_stopButton->callback(cb_stop);

	m_mainWindow->callback(cb_exit2);
	m_mainWindow->when(FL_HIDE);
    m_mainWindow->end();

	// image view
	m_traceGlWindow = new TraceGLWindow(100, 150, m_nSize, m_nSize, "Rendered Image");
	m_traceGlWindow->end();
	m_traceGlWindow->resizable(m_traceGlWindow);
}