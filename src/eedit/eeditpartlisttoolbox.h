/*
 * Author: Jorrit Tyberghein
 *
 * Copyright (C) 2010 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation (version 2 of the License)
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */

#ifndef EEDIT_PARTICLES_TOOLBOX_HEADER
#define EEDIT_PARTICLES_TOOLBOX_HEADER

#include "eedittoolbox.h"
#include "paws/pawswidget.h"
#include "paws/pawsspinbox.h"
#include "paws/pawscombo.h"
#include "paws/pawscheckbox.h"
#include "paws/pawscrollbar.h"

#include <csutil/weakref.h>
#include <imesh/particles.h>

class pawsButton;
class pawsListBox;
struct iEngine;
struct iParticleEmitter;
struct iParticleEffector;
struct iMeshObjectFactory;
struct ParticleParameterRow;

/**
 * \addtogroup eedit
 * @{ */

struct ParameterData
{
  csString type;
  csString text;
  csStringArray choices;
  float spinbox_min, spinbox_max, spinbox_step;
  ParameterData (csString type, float spinbox_min = 0, float spinbox_max = 1, float spinbox_step = .1) : 
    type (type), spinbox_min (spinbox_min), spinbox_max (spinbox_max), spinbox_step (spinbox_step)
  { }
};


/** This allows you to open/edit particle systems.
 */
class EEditParticleListToolbox : public EEditToolbox, public pawsWidget, public scfImplementation0<EEditParticleListToolbox>
{
public:
    EEditParticleListToolbox();
    /// TODO: Copy constructor, useless currently. Would be implemented later.
    EEditParticleListToolbox(const EEditParticleListToolbox& origin);
    virtual ~EEditParticleListToolbox();

    /** Fills the particles list with the names of all particle systems in the engine.
     */
    void FillList(iEngine* engine);
    void RefreshEditList();
    void FillParmList(iMeshObjectFactory* fact);
    void FillParmList(iParticleEmitter* emit);
    void FillParmList(iParticleEffector* eff);
    void RefreshParmList();

    void CreateNewEmit (const char* string);
    void CreateNewEffect (const char* string);

    // inheritted from EEditToolbox
    virtual void Update(unsigned int elapsed);
    virtual size_t GetType() const;
    virtual const char * GetName() const;

    // inheritted from pawsWidget
    virtual bool PostSetup(); 
    virtual bool OnButtonPressed(int mouseButton, int keyModifier, pawsWidget* widget);
    virtual bool OnButtonReleased( int button, int keyModifier, pawsWidget* widget);
    virtual void OnListAction(pawsListBox* selected, int status);
    virtual bool OnChange(pawsWidget* widget);
    virtual bool OnScroll(int dir, pawsScrollBar* widget);
    
private:
    // used by pawsListBox to sort the listbox
    static int SortTextBox(pawsWidget * widgetA, pawsWidget * widgetB);

    void HideValues ();
    void ClearParmList ();
    void UpdateParticleValue();
    void SaveParticleSystem (const csString& name);
    void ReloadParticleSystem (const csString& name);

    csArray<iParticleEmitter*> emitters;
    csArray<iParticleEffector*> effectors;
    csRef<iEngine> engine;

    int updatingParticleValue;
    csWeakRef<iParticleSystemFactory> pfact;
    csWeakRef<iMeshObjectFactory> objectFactory;

public:
    csPDelArray<ParticleParameterRow> parameterRows;

    pawsListBox  * partList;
    pawsListBox  * editList;
    pawsListBox  * parmList;
    pawsListBox  * valueList;
    pawsButton   * openPartButton;
    pawsButton   * refreshButton;
    pawsButton   * saveButton;
    pawsButton   * reloadButton;
    pawsSpinBox  * valueNumSpinBox;
    pawsSpinBox  * value2NumSpinBox;
    pawsSpinBox  * value3NumSpinBox;
    pawsComboBox * valueChoices;
    pawsCheckBox * valueBool;
    pawsCheckBox * valueBool2;
    pawsCheckBox * valueBool3;
    pawsCheckBox * valueBool4;
    pawsCheckBox * valueBool5;
    pawsScrollBar* valueScroll1;
    pawsScrollBar* valueScroll2;
    pawsScrollBar* valueScroll3;
    pawsScrollBar* valueScroll4;
    pawsButton   * addParButton;
    pawsButton   * delParButton;
    pawsButton   * addEmitButton;
    pawsButton   * addEffectorButton;
    pawsButton   * delEEButton;
};

CREATE_PAWS_FACTORY(EEditParticleListToolbox);

/** @} */

#endif 
