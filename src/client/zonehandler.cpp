/*
 * zonehandler.cpp    Keith Fulton <keith@paqrat.com>
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
//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/objreg.h>
#include <iutil/cfgmgr.h>
#include <iutil/object.h>
#include <iengine/engine.h>
#include <iengine/sector.h>
#include <iengine/light.h>
#include <iengine/material.h>
#include <iengine/mesh.h>
#include <iengine/movable.h>
#include <iengine/sector.h>
#include <imesh/partsys.h>
#include <csutil/cscolor.h>
#include <iutil/vfs.h>
#include <csutil/scfstringarray.h>
#include <csutil/sysfunc.h>
#include <ivaria/engseq.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "net/messages.h"
#include "net/clientmsghandler.h"

#include "util/psscf.h"
#include "util/log.h"

#include "paws/pawsmanager.h"
#include "paws/pawsprogressbar.h"

#include "gui/pawsloading.h"

#include "engine/psworld.h"

#include "iclient/ibgloader.h"
#include "iclient/isoundmngr.h"

#include "psclientdr.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "zonehandler.h"
#include "pscamera.h"
#include "pscelclient.h"
#include "modehandler.h"
#include "pscharcontrol.h"
#include "globals.h"

#define LOADING_SECTOR "SectorWhereWeKeepEntitiesResidingInUnloadedMaps"

ZoneHandler::ZoneHandler(MsgHandler* mh,iObjectRegistry* obj_reg, psCelClient *cc)
{
    msghandler     = mh;
    object_reg     = obj_reg;
    loading        = false;
    celclient      = cc;
    haveNewPos     = false;

    msghandler->Subscribe(this,MSGTYPE_NEWSECTOR);

    valid = LoadZoneInfo();
    needsToLoadMaps = false;
    initialRefreshScreen = true;

    loadWindow = NULL;
    loadProgressBar = NULL;
}

ZoneHandler::~ZoneHandler()
{
    if (msghandler)
    {
        msghandler->Unsubscribe(this,MSGTYPE_NEWSECTOR);
    }
    
    csHash<ZoneLoadInfo *, const char*>::GlobalIterator it(zonelist.GetIterator());
    
    while(it.HasNext())
        delete it.Next();
        
}

bool ZoneHandler::FindLoadWindow()
{
    if (loadProgressBar != NULL)
        return true;  // Already found

    loadWindow = static_cast <pawsLoadWindow*> (PawsManager::GetSingleton().FindWidget("LoadWindow"));
    if (loadWindow == NULL)
        return false;

    loadProgressBar = static_cast <pawsProgressBar*> (loadWindow->FindWidget("Progress"));
    if (loadProgressBar == NULL)
        return false;

    return true;
}

ZoneLoadInfo * ZoneHandler::FindZone(const char* sector)
{
    ZoneLoadInfo* zone = zonelist.Get(sector, NULL);

    if (zone == NULL)
        Error2("Error: Could not find zone info for sector %s!\n",sector);

    return zone;
}

bool ZoneHandler::LoadZoneInfo()
{
    iVFS* vfs = psengine->GetVFS ();
    if (!vfs)
        return false;
        
    iDocumentSystem* xml = psengine->GetXMLParser ();
    csRef<iDocument> doc = xml->CreateDocument();

    csRef<iFile> file = vfs->Open("/planeshift/data/zoneinfo.xml", VFS_FILE_READ);
    if (!file)
        return false;

    const char* error = doc->Parse(file);
    if (error)
    {
        Error2("Error Loading Zone Data: %s\n", error);
        return false;
    }

    csRef<iDocumentNodeIterator> zoneIter = doc->GetRoot()->GetNode("zonelist")->GetNodes("zone");

    while (zoneIter->HasNext())
    {
        csRef<iDocumentNode> zoneNode = zoneIter->Next();
        ZoneLoadInfo *zone = new ZoneLoadInfo(zoneNode);
        zonelist.Put(zone->inSector, zone);
    }

    return true;
}


void ZoneHandler::HandleMessage(MsgEntry* me)
{
    psNewSectorMessage msg(me);

    Notify3(LOG_LOAD, "Crossed from sector %s to sector %s.", msg.oldSector.GetData(), msg.newSector.GetData());

    LoadZone(msg.pos, msg.newSector);
}

void ZoneHandler::LoadZone(csVector3 pos, const char* sector)
{
    if(loading || !strcmp(sector, LOADING_SECTOR) || sectorToLoad == sector)
        return;

    newPos = pos;
    sectorToLoad = sector;

    ZoneLoadInfo* zone = FindZone(sectorToLoad);
    if (zone == NULL)
    {
        Error1("Unable to find the sector you have entered in zoneinfo data.\nPlease check zoneinfo.xml");
        return;
    }

    // Move player to the loading sector.
    MovePlayerTo(csVector3(0.0f), LOADING_SECTOR);

    // Load the world.
    if(psengine->BackgroundWorldLoading())
    {
        psengine->GetLoader()->UpdatePosition(pos, sectorToLoad, true);
    }
    else
    {
        if(!psengine->GetLoader()->LoadZones(zone->regions))
        {
            Error2("Unable to load zone '%s'\n", zone->inSector.GetData());
            return;
        }
    }

    // Set load screen if required.
    if(FindLoadWindow() && psengine->GetLoader()->GetLoadingCount() != 0 &&
      (!psengine->BackgroundWorldLoading() || !psengine->HasLoadedMap()))
    {
        loading = true;

        if(psengine->HasLoadedMap())
            loadWindow->Clear();

        loadWindow->AddText(PawsManager::GetSingleton().Translate("Loading world"));
        loadWindow->SetAlwaysOnTop(true);
        loadWindow->Show();

        // If the area has its own loading image, use it
        if (zone->loadImage)
        {
            Debug2(LOG_LOAD, 0, "Setting background %s", zone->loadImage.GetData());
            loadWindow->SetBackground(zone->loadImage.GetData());
        }

        loadProgressBar->SetTotalValue(psengine->GetLoader()->GetLoadingCount());
        loadProgressBar->SetCurrentValue(0.0f);

        psengine->ForceRefresh();
    }
    else
    {
        // Else we can immediately move to the new sector.
        MovePlayerTo(newPos, sectorToLoad);
    }
}

void ZoneHandler::MovePlayerTo(const csVector3 & newPos, const csString & newSector)
{
    if(!celclient->IsReady())
        return;

    csVector3 pos;
    float yrot;
    iSector* sector;
    
    celclient->GetMainPlayer()->GetLastPosition (pos, yrot, sector);            // retrieve last yrot
    
    sector = psengine->GetEngine()->FindSector(newSector);
    if (sector != NULL)
    {
        Notify5(LOG_LOAD, "Setting position of player %f %f %f in sector '%s'", newPos.x, newPos.y, newPos.z, newSector.GetData());
        celclient->GetMainPlayer()->SetPosition(newPos, yrot, sector);          // set new position
    }
    else
    {
        Error2("Couldn't find sector '%s'", newSector.GetData());
    }
}

void ZoneHandler::OnDrawingFinished()
{
    if (loading)
    {
        if(psengine->GetLoader()->GetLoadingCount() == 0)
        {
            if(psengine->HasLoadedMap())
                loadWindow->Hide();

            loading = false;
            loadProgressBar->Completed();
            psengine->SetLoadedMap(true);

            // Move the actor to the new sector.
            MovePlayerTo(newPos, sectorToLoad);

            // Move all entities which belong in these new sectors to them.
            psengine->GetCelClient()->OnMapsLoaded();

            // Reset camera clip distance.
            psCamera* cam = psengine->GetPSCamera();
            if(cam && !cam->GetDistanceCfg().adaptive)
                cam->UseFixedDistanceClipping(cam->GetFixedDistClip());
            psengine->GetPSCamera()->ResetCameraPositioning();

            // Update the lighting.
            psengine->GetModeHandler()->FinishLightFade();
            psengine->GetModeHandler()->DoneLoading(sectorToLoad);
        }
        else
        {
            psengine->GetLoader()->ContinueLoading(true);
            loadProgressBar->SetCurrentValue(loadProgressBar->GetTotalValue() - psengine->GetLoader()->GetLoadingCount());
        }
    }
}

ZoneLoadInfo::ZoneLoadInfo(iDocumentNode *node)
{
    csString trans;

    inSector = node->GetAttributeValue("sector");
    loadImage = node->GetAttributeValue("loadimage");
    trans = node->GetAttributeValue("transitional");
    transitional = (trans=="yes");
    regions.AttachNew(new scfStringArray());
    regions->Push("podium");

    csRef<iDocumentNodeIterator> regionIter = node->GetNodes("region");
    while (regionIter->HasNext())
    {
        // Parse attributes from tag
        csRef<iDocumentNode> regionNode = regionIter->Next();
        regions->Push(regionNode->GetAttributeValue("map"));
    }    
}
