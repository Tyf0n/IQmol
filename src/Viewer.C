/*******************************************************************************

  Copyright (C) 2011 Andrew Gilbert

  This file is part of IQmol, a free molecular visualization program. See
  <http://iqmol.org> for more details.

  IQmol is free software: you can redistribute it and/or modify it under the
  terms of the GNU General Public License as published by the Free Software
  Foundation, either version 3 of the License, or (at your option) any later
  version.

  IQmol is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
  details.

  You should have received a copy of the GNU General Public License along
  with IQmol.  If not, see <http://www.gnu.org/licenses/>.

********************************************************************************/

#include "Viewer.h"
#include "ViewerModel.h"
#include "IQmol.h"
#include "QsLog.h"
#include "MoleculeLayer.h"
#include "ShaderLibrary.h"
#include "EFPFragmentLayer.h"
#include "Preferences.h"
#include "ManipulatedFrameSetConstraint.h"
#include <QStandardItem>
#include <QDropEvent>
#include <QtDebug>
#include <QUrl>
#include <cmath>
#include "gl2ps.h"


using namespace qglviewer;

namespace IQmol {

Vec Layer::GLObject::s_cameraPosition = Vec(0.0, 0.0, 0.0);

const Qt::Key Viewer::s_buildKey(Qt::Key_Alt);
const Qt::Key Viewer::s_selectKey(Qt::Key_Shift);
const Qt::Key Viewer::s_manipulateSelectionKey(Qt::Key_Control);
const Qt::KeyboardModifier Viewer::s_buildModifier(Qt::AltModifier);
const Qt::KeyboardModifier Viewer::s_selectModifier(Qt::ShiftModifier);
const Qt::KeyboardModifier Viewer::s_manipulateSelectionModifier(Qt::ControlModifier);

QFont  Viewer::s_labelFont("Helvetica", 14, QFont::Black);
QFontMetrics Viewer::s_labelFontMetrics(Viewer::s_labelFont);


//! Window set up is done here
Viewer::Viewer(ViewerModel& model, QWidget* parent) : QGLViewer(parent), 
   m_viewerModel(model), 
   m_selectionHighlighting(true),
   m_labelType(Layer::Atom::None),
   m_selectHandler(this),
   m_manipulateHandler(this),
   m_buildEFPFragmentHandler(this),
   m_buildAtomHandler(this),
   m_buildFunctionalGroupHandler(this),
   m_buildMoleculeHandler(this),
   m_reindexAtomsHandler(this),
   m_manipulateSelectionHandler(this),
   m_snapper(0)
{ 
   setAcceptDrops(true);

   // Disable the default keybindings, the menu handles those we want
   setShortcut(DRAW_AXIS, 0);
   setShortcut(DRAW_GRID, 0);
   setShortcut(DISPLAY_FPS, 0);
   setShortcut(ENABLE_TEXT, 0);
   setShortcut(EXIT_VIEWER, 0);
   setShortcut(SAVE_SCREENSHOT, 0);
   setShortcut(CAMERA_MODE, 0);
   setShortcut(FULL_SCREEN, 0);
   setShortcut(STEREO, 0);
   setShortcut(ANIMATION, 0);
   setShortcut(HELP, 0);
   setShortcut(EDIT_CAMERA, 0);
   setShortcut(SNAPSHOT_TO_CLIPBOARD, 0);
   // Disable zoom selection, which makes no sense in our context
   setWheelBinding(Qt::MetaModifier, CAMERA, NO_MOUSE_ACTION);

   // The following two lines must be in this order
   setDefaultBuildElement(6);
   setActiveViewerMode(BuildAtom);  // this should get overwritten by the MainWindow class
   setSceneRadius(DefaultSceneRadius);
   resetView();
}



//! The OpenGL context is not available in the constructor, so all GL setup
//! must be done here.
void Viewer::init()
{
   makeCurrent();

   setManipulatedFrame(new ManipulatedFrame());
   manipulatedFrame()->setConstraint(new ManipulatedFrameSetConstraint());

   s_labelFont.setStyleStrategy(QFont::OpenGLCompatible);
   s_labelFont.setPointSize(Preferences::LabelFontSize());

   setForegroundColor(Preferences::ForegroundColor());
   setBackgroundColor(Preferences::BackgroundColor());

   if (0) {
      glDisable(GL_LIGHT0);
      glEnable(GL_LIGHT1);
   }else {
      glEnable(GL_LIGHT0);
      glDisable(GL_LIGHT1);
   }

   //setFPSIsDisplayed(true);

   // E
   const GLfloat light_ambient[4]  = {0.3, 0.3, 0.3, 1.0};
   const GLfloat light_diffuse[4]  = {0.75, 0.75, 0.75, 0.5};
   const GLfloat light_specular[4] = {1.0, 1.0, 1.0, 1.0};

   glLightfv(GL_LIGHT1, GL_AMBIENT,  light_ambient);
   glLightfv(GL_LIGHT1, GL_SPECULAR, light_specular);
   glLightfv(GL_LIGHT1, GL_DIFFUSE,  light_diffuse);

   //glLightf( GL_LIGHT1, GL_SPOT_EXPONENT, 2.0);
   //glLightf( GL_LIGHT1, GL_SPOT_CUTOFF,   95.0);
   //glLightf( GL_LIGHT1, GL_CONSTANT_ATTENUATION,  0.1f);
   //glLightf( GL_LIGHT1, GL_LINEAR_ATTENUATION,    0.3f);
   //glLightf( GL_LIGHT1, GL_QUADRATIC_ATTENUATION, 0.3f);
}



void Viewer::draw()
{
   makeCurrent();
   ShaderLibrary& library( ShaderLibrary::instance());


   // Position the light(s)
#if 0
   Vec cameraPosition(camera()->position());
   Vec cameraDirection(camera()->viewDirection());
   Vec cameraRight(camera()->rightVector());
   Vec cameraUp(camera()->upVector());

   double cameraDistance(camera()->sceneRadius());
   Vec lightPosition(0.0, 5.0, 0.0);
   Vec lightDirection(lightPosition.unit());
   lightDirection = cameraDirection;

   GLfloat pos[] = {lightPosition[0],  lightPosition[1],  lightPosition[2], 0.0};
   GLfloat dir[] = {lightDirection[0], lightDirection[1], lightDirection[2], 0.0};

   glLightfv(GL_LIGHT1, GL_POSITION, pos);
   glLightfv(GL_LIGHT1, GL_SPOT_DIRECTION, dir);
#endif


/*
   // Default material
   GLfloat mat_specular[] = { 0.9, 0.9, 0.9, 1.0 };
   glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, mat_specular);
   GLfloat mat_ambient[] = { 0.2, 0.2, 0.2, 1.0 };
   glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, mat_ambient);
   GLfloat mat_diffuse[] = { 0.8, 0.8, 0.8, 1.0 };
   glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, mat_diffuse);
   GLfloat mat_shininess = 50.0;
   glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, mat_shininess);
*/

   library.resumeShader();

   glShadeModel(GL_SMOOTH);
   glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);  


   // create the master object lists, do the surfaces neeed to be filtered out?
   m_objects = m_viewerModel.getVisibleObjects();
   m_selectedObjects = m_viewerModel.getSelectedObjects();

   Layer::GLObject::SetCameraPosition(camera()->position());

   drawGlobals();

   drawObjects(m_objects);

   drawSelected(m_selectedObjects);
   drawObjects(m_currentBuildHandler->buildObjects());
  
   library.suspendShader();

   if (m_labelType != Layer::Atom::None) drawLabels(m_objects);
   if (m_currentHandler->selectionMode() != Handler::None) {
      drawSelectionRectangle(m_selectHandler.region());
   }
   displayGeometricParameter(m_selectedObjects);
   
   //drawLight(GL_LIGHT1);
}


void Viewer::setSceneRadius(double const radius)
{
   float r = std::max(radius, DefaultSceneRadius);
   QGLViewer::setSceneRadius(r);
   Vec position(camera()->position());
   position = 3.0*r*position.unit();
   camera()->setPosition(position);
   updateGL();
}


void Viewer::resetView()
{
   setMouseBinding(Qt::LeftButton, QGLViewer::CAMERA, QGLViewer::ROTATE);
   double radius = std::max(DefaultSceneRadius, m_viewerModel.sceneRadius());

/*
   // do the interpolation in spherical coordinates to avoid slicing through the 
   // molecule. dunno how to get this working
   
   Vec p(camera()->position());
   double r(p.norm());
   double th(std::acos(p.z/r));
   double ph(std::atan2(p.y, p.x));

   int nSteps(10);
   // Interpolate to (0.0, 0.0, 3.0*radius);
   double step(1.0/nSteps);
   double dr((3.0*radius-r)*step);
   double dth((0.0-th)*step);
   double dph((0.0-ph)*step);

   Quaternion begin(camera()->orientation());
   Quaternion end;

   qDebug() << "Interpolating with steps " << dr << dth << dph;
   qDebug() << "Interpolation starting at" << p.x << p.y << p.z;
   for (int i = 1; i <= nSteps; ++i) {
       r += dr; th += dth; ph += dph;
       Vec v( r*std::sin(th)*std::cos(ph), r*std::sin(th)*std::sin(ph), r*std::cos(th));
       camera()->interpolateTo(Frame(v, Quaternion::slerp(begin, end, i*step)), 1.0);
       qDebug() << "Interpolating to" << v.x << v.y << v.z;
   }

*/

   // snapTo controls if the view is reset instantly, or via an animation.  At
   // the moment the animation pathway can be confusing, so we just snap to the
   // standard view.
   bool snapTo(true);
   if (snapTo) {
      camera()->setPosition( Vec(0.0f, 0.0, 3.0*radius) );
      camera()->setOrientation(0.0f, 0.0f);
      camera()->lookAt( Vec(0.0f, 0.0f, 0.0f) );
   }else {
      camera()->interpolateTo(Frame(Vec(0.0, 0.0, 3.0*radius), Quaternion()), 1.0);
   }

   m_viewerModel.sceneRadiusChanged(radius);
   updateGL();

}


Vec Viewer::worldCoordinatesOf(QMouseEvent* e, qglviewer::Vec const& hint)
{
   Camera* cam(camera());
   Vec v(cam->projectedCoordinatesOf(hint));
   v.setValue(e->x(), e->y(), v.z);
   v = cam->unprojectedCoordinatesOf(v);
   return v;
}


void Viewer::setLabelType(int const type)
{
   s_labelFont.setPointSize(Preferences::LabelFontSize());
   switch (type) {
      case Layer::Atom::Index:   m_labelType = Layer::Atom::Index;   break;
      case Layer::Atom::Element: m_labelType = Layer::Atom::Element; break;
      case Layer::Atom::Charge:  m_labelType = Layer::Atom::Charge;  break;
      case Layer::Atom::Mass:    m_labelType = Layer::Atom::Mass;    break;
      case Layer::Atom::Spin:    m_labelType = Layer::Atom::Spin;    break;
      case Layer::Atom::Reindex: m_labelType = Layer::Atom::Reindex; break;
      default:                   m_labelType = Layer::Atom::None;    break;
   }
   updateGL();
}


void Viewer::setDefaultBuildElement(unsigned int element)
{
   m_buildAtomHandler.setBuildElement(element);
   m_currentBuildHandler = &m_buildAtomHandler;
}


void Viewer::setDefaultBuildFragment(QString const& filePath, Viewer::Mode const mode)
{
   switch (mode) {
      case BuildFunctionalGroup:
         m_currentBuildHandler = &m_buildFunctionalGroupHandler;
         break;
      case BuildEFP:
         m_currentBuildHandler = &m_buildEFPFragmentHandler;
         break;
      case BuildMolecule:
         m_currentBuildHandler = &m_buildMoleculeHandler;
         break;
      default:
         qDebug() << "Invalid Viewer::Mode in setDefaultBuildFragment";
         return;
   }

   m_currentBuildHandler->setBuildFile(filePath);
}


void Viewer::drawGlobals() 
{ 
   m_viewerModel.displayGlobals(); 
}

void Viewer::drawObjects(GLObjectList const& objects)
{
   GLObjectList::const_iterator object;
   for (object = objects.begin(); object != objects.end(); ++object) {
       (*object)->draw();
   }
}


void Viewer::drawSelected(GLObjectList const& objects)
{
   if (!m_selectionHighlighting || objects.isEmpty()) return;

   // create a stencil of the highlighted region
   glClearStencil(0x4);
   glDisable(GL_LIGHTING);
   glClear(GL_STENCIL_BUFFER_BIT);
   glEnable(GL_STENCIL_TEST);
   glStencilFunc(GL_ALWAYS, 0x0, 0x4);
   glStencilMask(0x4);
   glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

   GLObjectList::const_iterator object;
   for (object = objects.begin(); object != objects.end(); ++object) {
       (*object)->draw();
   }

   glStencilFunc(GL_EQUAL, 0x4, 0x4);
   glStencilOp(GL_KEEP, GL_KEEP, GL_INVERT);

   for (object = objects.begin(); object != objects.end(); ++object) {
       glEnable(GL_BLEND);
       glBlendFunc(GL_ONE, GL_ONE);
       (*object)->drawSelected();
   }

   glDisable(GL_STENCIL_TEST);
   glDisable(GL_BLEND);
   glEnable(GL_LIGHTING);
}


void Viewer::drawLabels(GLObjectList const& objects)
{
   AtomList atomList;
   Layer::Atom* atom;
   Layer::Charge* charge;
   bool selectedOnly = (m_selectedObjects.count() > 0);

   GLObjectList::const_iterator object;
   for (object = objects.begin(); object!= objects.end(); ++object) {
       if ( (atom = qobject_cast<Layer::Atom*>(*object)) ) {
          atom->drawLabel(*this, m_labelType, s_labelFontMetrics);
          if ( !selectedOnly || atom->isSelected() ) atomList.append(atom);
       }else if ( (m_labelType == Layer::Atom::Charge) && 
                  (charge = qobject_cast<Layer::Charge*>(*object)) ) {
          charge->drawLabel(*this, s_labelFontMetrics);
       }
   }

   glDisable(GL_LIGHTING);
   qglColor(foregroundColor());

   QString mesg = selectedOnly ? "Selection " : "Total ";
   AtomList::const_iterator iter;
   double value(0.0);

   if (m_labelType == Layer::Atom::Mass) {
      for (iter= atomList.begin(); iter != atomList.end(); ++iter) {
          value += (*iter)->getMass();
      }
      mesg += "mass: " + QString::number(value,'f', 3);
      drawText(width()-s_labelFontMetrics.width(mesg), height()-10, mesg);
   }else if (m_labelType == Layer::Atom::Charge) {
      for (iter = atomList.begin(); iter != atomList.end(); ++iter) {
          value += (*iter)->getCharge();
      }
      mesg += "charge: " + QString::number(value,'f', 2);
      drawText(width()-s_labelFontMetrics.width(mesg), height()-10, mesg);
   }else if (m_labelType == Layer::Atom::Spin) {
      for (iter = atomList.begin(); iter != atomList.end(); ++iter) {
          value += (*iter)->getSpin();
      }
      mesg += "spin: " + QString::number(value,'f', 2);
      drawText(width()-s_labelFontMetrics.width(mesg), height()-10, mesg);
   }

   glEnable(GL_LIGHTING);
}


void Viewer::displayGeometricParameter(GLObjectList const& selection)
{
   QString msg;
   Layer::Atom *a, *b, *c, *d;
   Layer::Primitive*A, *B;
   Layer::Bond *bond;
   Layer::EFPFragment *efp;

   switch (selection.size()) {
      case 0:
         return;
         break;
      case 1: 
         if ( (bond = qobject_cast<Layer::Bond*>(selection[0])) ) {
             QChar ch(0x00c5);
             msg = "Bond length: " + QString::number(bond->length(), 'f', 5) + " " + ch;
         }else if ( (a = qobject_cast<Layer::Atom*>(selection[0])) ) {
             msg = a->getAtomicSymbol() + " atom";
         }else if ( (efp = qobject_cast<Layer::EFPFragment*>(selection[0])) ) {
             msg = efp->text() + " EFP";
         }
         break;

      case 2:
         if ( (a = qobject_cast<Layer::Atom*>(selection[0])) &&
              (b = qobject_cast<Layer::Atom*>(selection[1])) ) {
            QChar ch(0x00c5);
            msg = "Distance: " + 
                  QString::number(Layer::Primitive::distance(a,b), 'f', 5) + " " + ch;
         }else if ( (A = qobject_cast<Layer::Primitive*>(selection[0])) &&
                    (B = qobject_cast<Layer::Primitive*>(selection[1])) ) {
            QChar ch(0x00c5);
            msg = "C.O.M. Distance: " + 
                  QString::number(Layer::Primitive::distance(A,B),'f',5) + " " + ch;
         }
         break;

      case 3:
         if ( (a = qobject_cast<Layer::Atom*>(selection[0])) &&
              (b = qobject_cast<Layer::Atom*>(selection[1])) &&
              (c = qobject_cast<Layer::Atom*>(selection[2])) ) {
            QChar ch(0x00b0);
            msg = "Angle: " + 
                  QString::number(Layer::Primitive::angle(a,b,c), 'f', 5) + " " + ch;
         }
         break;

      case 4:
         if ( (a = qobject_cast<Layer::Atom*>(selection[0])) &&
              (b = qobject_cast<Layer::Atom*>(selection[1])) &&
              (c = qobject_cast<Layer::Atom*>(selection[2])) &&
              (d = qobject_cast<Layer::Atom*>(selection[3])) ) {
            QChar ch(0x00b0);
            msg = "Torsion: " + 
                  QString::number(Layer::Primitive::torsion(a,b,c,d), 'f', 3) + " " + ch;
         }
         break;

      default:
         break;
   }

   displayMessage(msg); 
}



// ---------------- Animation functions ---------------
void Viewer::pushAnimators(AnimatorList const& animators)
{
   //QLOG_TRACE() << "Pushing" << animators.size() << "animators";
   AnimatorList::const_iterator iter;
   for (iter = animators.begin(); iter != animators.end(); ++iter) {
       if (!m_animatorList.contains(*iter)) {
      //QLOG_TRACE() << "Pushing animator:" << (*iter);
          m_animatorList.append(*iter);
          connect(*iter, SIGNAL(finished(Animator::Base*)),
             this, SLOT(popAnimator(Animator::Base*)));
       }
   }

   if (!animationIsStarted()) {
      QLOG_DEBUG() << "Starting animation";
      startAnimation();
   }
}


void Viewer::popAnimators(AnimatorList const& animators)
{
   //QLOG_TRACE() << "Popping " << animators.size() << "animators";
   AnimatorList::const_iterator iter;
   for (iter = animators.begin(); iter != animators.end(); ++iter) {
      //QLOG_TRACE() << "Popping animator:" << (*iter);
       popAnimator(*iter);
   }
}


void Viewer::popAnimator(Animator::Base* base)
{
   disconnect(base, SIGNAL(finished(Animator::Base*)),
      this, SLOT(popAnimator(Animator::Base*)));
   int n(m_animatorList.size());
   m_animatorList.removeAll(base);
   if (m_animatorList.isEmpty() && n > 0) {
      // turn the lights out if we are last to leave
      QLOG_DEBUG() << "Stoping animation";
      stopAnimation();
      m_viewerModel.determineSymmetry();
   }
}

void Viewer::animate()
{
   AnimatorList animators(m_animatorList);
   AnimatorList::iterator iter;
   for (iter = animators.begin(); iter != animators.end(); ++iter) {
       (*iter)->step();
   }

   if (m_snapper) m_snapper->capture();
}


void Viewer::clearAnimators()
{
   stopAnimation();
   m_animatorList.clear();
}


void Viewer::setRecordingActive(bool activate)
{
   if (activate) {
      if (m_snapper) {
         QLOG_WARN() << "Animation recording started with existing Snapshot object";
      }else {
         m_snapper = new Snapshot(this, Snapshot::Movie);
         if (!m_snapper->requestFileName()) {
            delete m_snapper;
            m_snapper = 0;
            recordingCancelled();
         }
      } 
   }else {
      if (m_snapper) {
         m_snapper->makeMovie();
         delete m_snapper;
      } 
      m_snapper = 0;
   }
}



// ---------------- Selection functions ---------------

void Viewer::drawWithNames() 
{
   int i;
   for (i = 0; i < int(m_objects.size()); ++i) {
       glPushName(i);
       m_objects.at(i)->draw();
       glPopName();
   }

   GLObjectList buildObjects(m_currentBuildHandler->buildObjects());
   for (int j = 0; j < int(buildObjects.size()); ++j) {
       glPushName(i);
       buildObjects.at(j)->draw();
       glPopName();
       ++i;
   }
}


void Viewer::endSelection(const QPoint&) 
{
   glFlush();

   // Get the number of objects that were seen through the pick matrix frustum.
   m_selectionHits = glRenderMode(GL_RENDER);
   //qDebug() << "Viewer::endSelection with nhits:" << m_selectionHits << m_objects.size() ;
   //for (int i = 0; i < m_objects.size(); ++i) qDebug() << m_objects[i];
   setSelectRegionWidth(3);
   setSelectRegionHeight(3);

   if (m_selectionHits == 0) return;

   // If the user clicks, then we only select the front object
   Handler::SelectionMode selectionMode(m_currentHandler->selectionMode());
   if ( (selectionMode == Handler::AddClick) || (selectionMode == Handler::RemoveClick) ||
        (selectionMode == Handler::ToggleClick) ) {
      GLuint zMin = (selectBuffer())[1];
      int iMin = 0;
      for (int i = 1; i < m_selectionHits; ++i) {
          if ((selectBuffer())[4*i+1] < zMin) {
             iMin = i;
             zMin = (selectBuffer())[4*i+1];
          }
      }

      if (selectionMode == Handler::AddClick) {
         addToSelection((selectBuffer())[4*iMin+3]);
      }else if (selectionMode == Handler::RemoveClick) {
         removeFromSelection((selectBuffer())[4*iMin+3]);
      }else {
         toggleSelection((selectBuffer())[4*iMin+3]);
      }

   }else {
      // The selection rectangle is non-zero so we select all the objects
      // behind it.
   
      // Interpret results : each object created 4 values in the selectBuffer().
      // (selectBuffer())[4*i+3] is the id pushed on the stack.

	  // Temporarily switch off GL updating so the selection routines don't
	  // trigger an update which makes the slected item appear incrementally.
      enableUpdate(false);
      for (int i = 0; i < m_selectionHits; ++i) {
          switch (m_currentHandler->selectionMode()) {
             case Handler::Add: 
                addToSelection((selectBuffer())[4*i+3]); 
                break;
             case Handler::Remove: 
                removeFromSelection((selectBuffer())[4*i+3]);  
                break;
             case Handler::Toggle: 
                toggleSelection((selectBuffer())[4*i+3]);  
                break;
             default: 
                addToSelection((selectBuffer())[4*i+3]); 
                break;
          }
      }
      enableUpdate(true);
      updateGL();
   }
}


void Viewer::drawSelectionRectangle(QRect const& rect) const {
   startScreenCoordinatesSystem();
   glBlendFunc(GL_ONE, GL_ONE);
   glDisable(GL_LIGHTING);
   glEnable(GL_BLEND);

   glColor4f(0.0, 0.0, 0.3f, 0.3f);
   glBegin(GL_QUADS);
      glVertex2i(rect.left(),  rect.top());
      glVertex2i(rect.right(), rect.top());
      glVertex2i(rect.right(), rect.bottom());
      glVertex2i(rect.left(),  rect.bottom());
   glEnd();

   glLineWidth(2.0);
   glColor4f(0.4f, 0.4f, 0.5f, 0.5f);

   glBegin(GL_LINE_LOOP);
      glVertex2i(rect.left(),  rect.top());
      glVertex2i(rect.right(), rect.top());
      glVertex2i(rect.right(), rect.bottom());
      glVertex2i(rect.left(),  rect.bottom());
   glEnd();

   glDisable(GL_BLEND);
   glEnable(GL_LIGHTING);
   stopScreenCoordinatesSystem();
}


void Viewer::postSelection(QPoint const&) {
   //displayGeometricParameter();
}


void Viewer::addToSelection(Layer::GLObject* object) 
{
   select(object->index(), QItemSelectionModel::Select);
}


void Viewer::addToSelection(int id) 
{
   if (id < m_objects.size()) select(m_objects[id]->index(), QItemSelectionModel::Select);
}


void Viewer::removeFromSelection(int id) 
{
   if (id < m_objects.size()) {
      select(m_objects[id]->index(), QItemSelectionModel::Deselect);
   }
}


void Viewer::toggleSelection(int id) 
{
   if (id < m_objects.size()) {
      if (m_objects[id]->isSelected()) {
         select(m_objects[id]->index(), QItemSelectionModel::Deselect);
      }else {
         select(m_objects[id]->index(), QItemSelectionModel::Select);
      }
   }
}



// ---------------- Events ---------------
void Viewer::mousePressEvent(QMouseEvent* e)
{
   m_currentHandler->mousePressEvent(e);
   if (!e->isAccepted()) QGLViewer::mousePressEvent(e);
}


void Viewer::mouseMoveEvent(QMouseEvent* e)
{
   m_currentHandler->mouseMoveEvent(e);
   if (!e->isAccepted()) QGLViewer::mouseMoveEvent(e);
}     
      

void Viewer::mouseReleaseEvent(QMouseEvent* e)
{     
   m_currentHandler->mouseReleaseEvent(e);
   if (!e->isAccepted()) QGLViewer::mouseReleaseEvent(e);
}     

   
void Viewer::mouseDoubleClickEvent(QMouseEvent* e)
{     
   m_currentHandler->mouseDoubleClickEvent(e);
   if (!e->isAccepted()) QGLViewer::mouseDoubleClickEvent(e);
}     

      
void Viewer::wheelEvent(QWheelEvent* e) 
{     
   m_currentHandler->wheelEvent(e);
   if (!e->isAccepted()) QGLViewer::wheelEvent(e);
}


void Viewer::keyPressEvent(QKeyEvent *e)
{     
   e->ignore();

   // Check if we need to swtich handlers or 
   // if we are using a global QKeyEvent
   switch (e->key()) {
      case s_selectKey:
         setHandler(Select);
         e->accept();
         break;
      case s_manipulateSelectionKey:
         setHandler(ManipulateSelection);
         e->accept();
         break;
      case s_buildKey:
         m_currentHandler = m_currentBuildHandler;
         setCursor(m_currentHandler->cursorType());
         e->accept();
         break;
   }

   // Offload to the current handler
   if (!e->isAccepted()) m_currentHandler->keyPressEvent(e);
   if (!e->isAccepted()) QGLViewer::keyPressEvent(e);
   return;
}


void Viewer::keyReleaseEvent(QKeyEvent *e)
{
   if ( (e->key() == s_selectKey) ||
        (e->key() == s_buildKey) ||
        (e->key() == s_manipulateSelectionKey) ) {
      e->accept();
      setHandler(m_activeMode);
   } else if (e->key() == Qt::Key_Escape && isFullScreen()) {
      escapeFullScreen();
   }else {
      m_currentHandler->keyReleaseEvent(e);
   }

   if (!e->isAccepted()) QGLViewer::keyReleaseEvent(e);
}


void Viewer::enterEvent(QEvent*)
{
   setActiveViewerMode(m_activeMode);
}
      
   
void Viewer::leaveEvent(QEvent*)
{
   QWidget::setCursor(QCursor(Qt::ArrowCursor));
}

void Viewer::dragEnterEvent(QDragEnterEvent* event)
{  
   if (event->mimeData()->hasUrls()) event->acceptProposedAction();
}
   
   
void Viewer::dropEvent(QDropEvent* event)
{  
   QLOG_INFO() << "Drop Event in Viewer successful";
 
   QList<QUrl> urls(event->mimeData()->urls());
   QList<QUrl>::iterator iter;
   
   for (iter = urls.begin(); iter != urls.end(); ++iter) {
      openFileFromDrop((*iter).path());
      QLOG_DEBUG() << *iter;
   }
   event->acceptProposedAction();
} 


// --------------- Handler Modes ---------------
void Viewer::setActiveViewerMode(Viewer::Mode const mode)
{
   m_previousMode = m_activeMode;
   m_activeMode = mode;
   setHandler(m_activeMode);
   activeViewerModeChanged(m_activeMode);
}


void Viewer::setHandler(Viewer::Mode const mode)
{
   switch (mode) {
      case Manipulate:
         m_currentHandler = &m_manipulateHandler; 
         break;

      case Select:
         m_currentHandler = &m_selectHandler; 
         break;

      case ManipulateSelection:
         m_currentHandler = &m_manipulateSelectionHandler; 
         break;

      case ReindexAtoms:
         m_currentHandler = &m_reindexAtomsHandler; 
         break;

      case BuildAtom:
         m_currentHandler      = &m_buildAtomHandler;
         m_currentBuildHandler = &m_buildAtomHandler;
         break;

      case BuildFunctionalGroup:
         m_currentHandler      = &m_buildFunctionalGroupHandler;
         m_currentBuildHandler = &m_buildFunctionalGroupHandler;
         break;

      case BuildEFP:
         m_currentHandler      = &m_buildEFPFragmentHandler;
         m_currentBuildHandler = &m_buildEFPFragmentHandler;
         break;

      case BuildMolecule:
         m_currentHandler      = &m_buildMoleculeHandler;
         m_currentBuildHandler = &m_buildMoleculeHandler;
         break;
   }

   m_selectionHighlighting = true;
   setCursor(m_currentHandler->cursorType());
}


// This is the only entry point for the ReindexAtom mode
void Viewer::reindexAtoms(Layer::Molecule* molecule)
{
   setActiveViewerMode(ReindexAtoms);
   m_reindexAtomsHandler.reset(molecule);
   setLabelType(Layer::Atom::Reindex);
   displayMessage("Hit Return to commit changes, Esc to cancel");
}



GLObjectList Viewer::startManipulation(QMouseEvent* event) 
{
   ManipulatedFrame* frame(manipulatedFrame());
   /// The value of 100 effectively turns off auto rotation for the selection
   frame->setSpinningSensitivity(100.0);
   ManipulatedFrameSetConstraint* mfsc = (ManipulatedFrameSetConstraint*)(frame->constraint());
   mfsc->clearSet();

   GLObjectList selection;
   GLObjectList::iterator iter;
   selection = m_selectedObjects;
   Layer::Bond* bond;

   if ((selection.size() == 1) &&
      (bond = qobject_cast<Layer::Bond*>(selection.first())) ) {

      // Determine the parent molecule
      MoleculeList parents(bond->findLayers<Layer::Molecule>(Layer::Parents));
      Layer::Molecule* molecule;

      if (parents.isEmpty()) {
         return GLObjectList();
      }else {
         molecule = parents.first();
      }

      Layer::Atom* begin(bond->beginAtom());
      Layer::Atom* end(bond->endAtom());

      Vec a(camera()->projectedCoordinatesOf(begin->getPosition()));
      Vec b(camera()->projectedCoordinatesOf(end->getPosition()));
      Vec e(event->x(), event->y(), 0.0);

      if ((a-e).norm() < (b-e).norm()) {
         Layer::Atom* tmp(begin);
         begin = end;
         end   = tmp;
      }

      AtomList fragment(molecule->getContiguousFragment(begin, end));

      selection.clear();
      AtomList::iterator iter;
      for (iter = fragment.begin(); iter != fragment.end(); ++iter) {
          mfsc->addObjectToSet(*iter);
          selection.append(*iter);
      }
      mfsc->addObjectToSet(end);

      Vec axis(end->getPosition() - begin->getPosition());
      mfsc->setTranslationConstraint(AxisPlaneConstraint::AXIS, axis);
      mfsc->setRotationConstraint(AxisPlaneConstraint::AXIS, axis);
      manipulatedFrame()->setPosition(end->getPosition());

   }else if (selection.size() > 0) {

      mfsc->setTranslationConstraintType(AxisPlaneConstraint::FREE);
      mfsc->setRotationConstraintType(AxisPlaneConstraint::FREE);
      Vec averagePosition;

      GLObjectList::const_iterator iter;
      for (iter = selection.begin(); iter != selection.end(); ++iter) {
          mfsc->addObjectToSet(*iter);
          averagePosition += (*iter)->getPosition();
      }
      manipulatedFrame()->setPosition(averagePosition / selection.size());
   }

   return selection;
}



// --------------- Save Snapshot ---------------
void Viewer::saveSnapshot()
{ 
   QGLViewer::saveSnapshot(false); 
   //Snapshot snap(this);
   //snap.capture();
}


} // end namespace IQmol
