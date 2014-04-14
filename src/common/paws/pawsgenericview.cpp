/*
 * Author: Andrew Craig
 *
 * Copyright (C) 2003 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

#include <psconfig.h>

#include <iengine/campos.h>
#include <iengine/collection.h>
#include <imap/loader.h>
#include <iutil/object.h>
#include <iutil/objreg.h>
#include <iutil/vfs.h>

#include "pawsgenericview.h"
#include "pawsmanager.h"
#include "util/log.h"
#include "util/psconst.h"

int pawsGenericView::idName = 0;

pawsGenericView::pawsGenericView()
{
    engine =  csQueryRegistry<iEngine > (PawsManager::GetSingleton().GetObjectRegistry());
    idName++;

    char newName[10];
    sprintf(newName, "NAME%d\n", idName);
    col = engine->CreateCollection(newName);

    loadedMap = false;
}

pawsGenericView::pawsGenericView(const pawsGenericView &origin)
    :pawsWidget(origin),
     stage(origin.stage),
     view(origin.view),
     engine(origin.engine),
     col(origin.col),
     objectPos(origin.objectPos),
     //idName(origin.idName),
     mapName(origin.mapName),
     loadedMap(origin.loadedMap)
{

}

pawsGenericView::~pawsGenericView()
{
    idName--;
}

bool pawsGenericView::Setup(iDocumentNode* node)
{
    csRef<iDocumentNode> mapNode = node->GetNode("map");
    if(mapNode)
    {
        csString mapFile = mapNode->GetAttributeValue("file");
        csString sector  = mapNode->GetAttributeValue("sector");

        LoadMap(mapFile, sector);
        return true;
        //return LoadMap( mapFile, sector );
    }
    return false;
}

bool pawsGenericView::LoadMap(const char* map, const char* sector)
{
    csRef<iEngine> engine =  csQueryRegistry<iEngine > (PawsManager::GetSingleton().GetObjectRegistry());
    csRef<iThreadedLoader> loader =  csQueryRegistry<iThreadedLoader> (PawsManager::GetSingleton().GetObjectRegistry());
    csRef<iVFS> VFS =  csQueryRegistry<iVFS> (PawsManager::GetSingleton().GetObjectRegistry());

    col = engine->CreateCollection(sector);
    stage = engine->FindSector(sector);

    if(!stage)
    {
        csRef<iDocumentSystem> xml(
            csQueryRegistry<iDocumentSystem> (PawsManager::GetSingleton().GetObjectRegistry()));

        csRef<iDocument> doc = xml->CreateDocument();
        csString filename = map;
        filename.Append("/world");
        csRef<iDataBuffer> buf(VFS->ReadFile(filename, false));
        const char* error = doc->Parse(buf);
        if(error)
        {
            Error2("pawsObjectView world parse error: %s", error);
        }

        csRef<iDocumentNode> worldNode = doc->GetRoot()->GetNode("world");

        // Now load the map into the selected region
        csRef<iThreadReturn> itr = loader->LoadMapWait(map, worldNode, false, col);
        if(!itr->WasSuccessful())
            return false;

        stage = engine->FindSector(sector);
        stage->PrecacheDraw();
        CS_ASSERT(stage);
        col->Add(stage->QueryObject());
        if(!stage)
            return false;
    }

    view.AttachNew(new csView(engine, PawsManager::GetSingleton().GetGraphics3D()));
    if(engine->GetCameraPositions()->GetCount() > 0)
    {
        iCameraPosition* cp = engine->GetCameraPositions()->Get(0);
        view->GetCamera()->SetSector(engine->FindSector(cp->GetSector()));
        view->GetCamera()->GetTransform().SetOrigin(cp->GetPosition());
        view->GetCamera()->GetTransform().LookAt(cp->GetForwardVector(), cp->GetUpwardVector());
    }
    else
    {
        view->GetCamera()->SetSector(stage);
        view->GetCamera()->GetTransform().SetOrigin(csVector3(-33,1,-198));
        view->GetCamera()->GetTransform().LookAt(csVector3(0,0,4), csVector3(0,1,0));
    }

    view->SetRectangle(screenFrame.xmin, screenFrame.ymin, screenFrame.Width(), screenFrame.Height());

    return true;
}

void pawsGenericView::Draw()
{
    if(screenFrame.xmin > graphics2D->GetWidth() || screenFrame.ymin > graphics2D->GetHeight() ||
            screenFrame.xmax < 0 || screenFrame.ymax < 0)
    {
        return;
    }

    graphics2D->SetClipRect(0, 0, graphics2D->GetWidth(), graphics2D->GetHeight());
    if(!PawsManager::GetSingleton().GetGraphics3D()->BeginDraw(CSDRAW_3DGRAPHICS))
    {
        return;
    }

    if(!view)
    {
        return;
    }

    iGraphics3D* og3d = view->GetContext();

    view->SetContext(PawsManager::GetSingleton().GetGraphics3D());

    view->SetRectangle(screenFrame.xmin,
                       PawsManager::GetSingleton().GetGraphics3D()->GetHeight() - screenFrame.ymax ,
                       screenFrame.Width(), screenFrame.Height());

    view->GetPerspectiveCamera()->SetPerspectiveCenter((float)(screenFrame.xmin+(screenFrame.Width() >> 1))/graphics2D->GetWidth(),
            1-(float)(screenFrame.ymin+(screenFrame.Height() >> 1))/graphics2D->GetHeight());
    view->Draw();

    PawsManager::GetSingleton().GetGraphics3D()->BeginDraw(CSDRAW_2DGRAPHICS);

    view->SetContext(og3d);
    pawsWidget::Draw();
}
