/*
* pawslauncherwindow.cpp - Author: Mike Gist
*
* Copyright (C) 2007 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
#include <fstream>

#include <iutil/stringarray.h>
#include <isndsys/ss_renderer.h>

#include "paws/pawsbutton.h"
#include "paws/pawscheckbox.h"
#include "paws/pawscombo.h"
#include "paws/pawsimagedrawable.h"
#include "paws/pawslistbox.h"
#include "paws/pawsokbox.h"
#include "paws/pawstextbox.h"
#include "paws/pawswidget.h"
#include "paws/pawsyesnobox.h"

#include "globals.h"
#include "pawslauncherwindow.h"

using namespace CS::Threading;
using namespace std;

pawsLauncherWindow::pawsLauncherWindow()
    :launcherMain(NULL),launcherUpdater(NULL),launcherSettings(NULL),
     resolution(NULL),updateAvailable(NULL),notify(NULL)
{
}

bool pawsLauncherWindow::PostSetup()
{
    configFile.AttachNew(new csConfigFile(LAUNCHER_CONFIG_FILENAME, psLaunchGUI->GetVFS()));
    configUser.AttachNew(new csConfigFile("/planeshift/userdata/planeshift.cfg", psLaunchGUI->GetVFS()));

    launcherMain = FindWidget("LauncherMain");
    launcherSettings = FindWidget("LauncherSettings");
    launcherUpdater = FindWidget("LauncherUpdater");
    resolution = (pawsComboBox*)FindWidget("ScreenResolution");

    launcherMain->OnGainFocus();

    // Load game settings.
    LoadSettings();

    // Get server news.
    newsUpdater.AttachNew(new Thread(new NewsUpdater(this), true));

    // Setup update available window.
    updateAvailable = (pawsYesNoBox*)FindWidget("UpdateAvailable");
    updateAvailable->SetCallBack(HandleUpdateButton, updateAvailable, "An update to PlaneShift is available. Do you wish to update now?");
    updateAvailable->SetAlwaysOnTop(true);

    // Setup notify window.
    notify = (pawsOkBox*)FindWidget("Notify");
    notify->SetAlwaysOnTop(true);

    //check if we allow update with this pslaunch. By default we do
    //but if the setting files say otherwise we don't. Useful
    //when the system is used through other distribution systems
    //if we don't allow update hide the repair buttons
    if(!configFile->GetBool("Update.Enable", true))
    {
        FindWidget("RepairButton")->Hide();
    }

    return true;
}

void pawsLauncherWindow::UpdateNews()
{
    pawsMessageTextBox* serverNews = (pawsMessageTextBox*)FindWidget("ServerNews");
    psLaunchGUI->GetDownloader()->DownloadFile(configFile->GetStr("Launcher.News.URL", "http://planeshift.ezpcusa.com/servernews"),
      "/planeshift/userdata/servernews", true, true, 3, true);
    
    csRef<iDataBuffer> newsPath = psLaunchGUI->GetVFS()->GetRealPath("/planeshift/userdata/servernews");

    ifstream newsFile(newsPath->GetData(), ifstream::in);
    csString buffer;

    buffer.Append("Your client version is ");
    buffer.Append(psLaunchGUI->GetCurrentClientVersion());
    buffer.Append("\n");
    buffer.Append("Your launcher version is ");
    buffer.Append(UPDATER_VERSION);
    buffer.Append("\n\n");

    while(newsFile.good())
    {
        buffer.Append((char)newsFile.get());
    }
    buffer.Truncate(buffer.Length()-1);
    serverNews->Clear();
    serverNews->AddMessage(buffer.GetDataSafe());
    newsFile.close();
    psLaunchGUI->GetFileUtil()->RemoveFile("/planeshift/userdata/servernews", true);

    // Scroll to top.
    serverNews->ResetScroll();
}

pawsButton* pawsLauncherWindow::FindButton(WidgetID id)
{
    return static_cast<pawsButton*>(FindWidget(id));
}

bool pawsLauncherWindow::OnButtonPressed(int /*mouseButton*/, int /*keyModifier*/, pawsWidget* widget)
{
    int ID = widget->GetID();

    if(ID == QUIT_BUTTON)
    {
        psLaunchGUI->Quit();
    }
    else if(ID == PLAY_BUTTON)
    {
        // TODO: Grey out the Play button instead of doing nothing.
        if(!psLaunchGUI->UpdateChecked())
        {
            pawsMessageTextBox* serverNews = (pawsMessageTextBox*)FindWidget("ServerNews");
	    if(serverNews)
                serverNews->AddMessage("Warning: Did not finish checking for updates.");
	}
        psLaunchGUI->ExecClient(true);
        psLaunchGUI->Quit();
    }
    else if(ID == SETTINGS_BUTTON)
    {
        launcherMain->Hide();
        launcherSettings->Show();
        launcherSettings->OnGainFocus();
        FindButton(SETTINGS_GENERAL_BUTTON)->SetState(true, false);
    }
    else if(ID == REPAIR_BUTTON)
    {
        psLaunchGUI->PerformRepair();
        pawsMessageTextBox* output = (pawsMessageTextBox*)FindWidget("UpdaterOutput");
        output->Clear();
        launcherMain->Hide();
        launcherUpdater->Show();
        launcherUpdater->OnGainFocus();
    }
    else if(ID == UPDATER_YES_BUTTON)
    {
        widget->Hide();
        FindWidget("UpdaterNoButton")->Hide();
        FindWidget("UpdaterCancelButton")->Show();
        psLaunchGUI->PerformUpdate(false, true);
    }
    else if(ID == UPDATER_NO_BUTTON)
    {
        FindWidget("UpdaterYesButton")->Hide();
        widget->Hide();
        launcherUpdater->Hide();
        launcherMain->Show();
        launcherMain->OnGainFocus();
        psLaunchGUI->PerformUpdate(false, false);
    }
    else if(ID == UPDATER_OK_BUTTON)
    {
        UpdateNews();
        FindWidget("UpdaterNoButton")->Hide();
        FindWidget("UpdaterYesButton")->Hide();
        widget->Hide();
        launcherUpdater->Hide();
        launcherMain->Show();
        launcherMain->OnGainFocus();
    }
    else if(ID == UPDATER_CANCEL_BUTTON)
    {
        widget->Hide();
        launcherUpdater->Hide();
        launcherMain->Show();
        launcherMain->OnGainFocus();
        psLaunchGUI->CancelUpdater();
    }
    else if(ID == SETTINGS_CANCEL_BUTTON)
    {
        LoadSettings();
        FindWidget("SettingsAudio")->Hide();
        FindWidget("SettingsControls")->Hide();
        FindWidget("SettingsGeneral")->Show();
        FindWidget("SettingsGraphics")->Hide();
        FindButton(SETTINGS_AUDIO_BUTTON)->SetState(false, false);
        FindButton(SETTINGS_CONTROLS_BUTTON)->SetState(false, false);
        FindButton(SETTINGS_GRAPHICS_BUTTON)->SetState(false, false);
        launcherSettings->Hide();
        launcherMain->Show();
        launcherMain->OnGainFocus();
    }
    else if(ID == SETTINGS_OK_BUTTON)
    {
        FindWidget("SettingsAudio")->Hide();
        FindWidget("SettingsControls")->Hide();
        FindWidget("SettingsGeneral")->Show();
        FindWidget("SettingsGraphics")->Hide();
        FindButton(SETTINGS_AUDIO_BUTTON)->SetState(false, false);
        FindButton(SETTINGS_CONTROLS_BUTTON)->SetState(false, false);
        FindButton(SETTINGS_GRAPHICS_BUTTON)->SetState(false, false);
        launcherSettings->Hide();
        launcherMain->Show();
        launcherMain->OnGainFocus();
        SaveSettings();
        LoadSettings();
    }
    else if(ID == SETTINGS_AUDIO_BUTTON)
    {
        FindWidget("SettingsAudio")->Show();
        FindWidget("SettingsControls")->Hide();
        FindWidget("SettingsGeneral")->Hide();
        FindWidget("SettingsGraphics")->Hide();
        FindButton(SETTINGS_AUDIO_BUTTON)->SetState(true, false);
        FindButton(SETTINGS_CONTROLS_BUTTON)->SetState(false, false);
        FindButton(SETTINGS_GENERAL_BUTTON)->SetState(false, false);
        FindButton(SETTINGS_GRAPHICS_BUTTON)->SetState(false, false);
        launcherSettings->OnGainFocus();
    }
    else if(ID == SETTINGS_CONTROLS_BUTTON)
    {
        FindWidget("SettingsAudio")->Hide();
        FindWidget("SettingsControls")->Show();
        FindWidget("SettingsGeneral")->Hide();
        FindWidget("SettingsGraphics")->Hide();
        FindButton(SETTINGS_AUDIO_BUTTON)->SetState(false, false);
        FindButton(SETTINGS_CONTROLS_BUTTON)->SetState(true, false);
        FindButton(SETTINGS_GENERAL_BUTTON)->SetState(false, false);
        FindButton(SETTINGS_GRAPHICS_BUTTON)->SetState(false, false);
        launcherSettings->OnGainFocus();
    }
    else if(ID == SETTINGS_GENERAL_BUTTON)
    {
        FindWidget("SettingsAudio")->Hide();
        FindWidget("SettingsControls")->Hide();
        FindWidget("SettingsGeneral")->Show();
        FindWidget("SettingsGraphics")->Hide();
        FindButton(SETTINGS_AUDIO_BUTTON)->SetState(false, false);
        FindButton(SETTINGS_CONTROLS_BUTTON)->SetState(false, false);
        FindButton(SETTINGS_GENERAL_BUTTON)->SetState(true, false);
        FindButton(SETTINGS_GRAPHICS_BUTTON)->SetState(false, false);
        launcherSettings->OnGainFocus();
    }
    else if(ID == SETTINGS_GRAPHICS_BUTTON)
    {
        FindWidget("SettingsAudio")->Hide();
        FindWidget("SettingsControls")->Hide();
        FindWidget("SettingsGeneral")->Hide();
        FindWidget("SettingsGraphics")->Show();
        FindButton(SETTINGS_AUDIO_BUTTON)->SetState(false, false);
        FindButton(SETTINGS_CONTROLS_BUTTON)->SetState(false, false);
        FindButton(SETTINGS_GENERAL_BUTTON)->SetState(false, false);
        FindButton(SETTINGS_GRAPHICS_BUTTON)->SetState(true, false);
        launcherSettings->OnGainFocus();
    }
    else if(ID == NOTIFY_OK_BUTTON)
    {
        notify->Hide();
    }
    else if(ID == DELETE_CACHE)
    {
        pawsYesNoBox *shadercacherequest = (pawsYesNoBox*)FindWidget("ShaderCacheRequest");
        shadercacherequest->SetCallBack(HandleCacheButton, this, "Do you want to delete the shadercache? This is usually helpful if you are experiencing crashes after an update.");
        shadercacherequest->SetAlwaysOnTop(true);
        shadercacherequest->Show();
    }
    else if(13100 < widget->GetID() && widget->GetID() < 13200)
    {
        pawsComboBox* graphicsPreset = (pawsComboBox*)FindWidget("GraphicsPreset");
        graphicsPreset->Select("Custom");
    }

    return true;
}

void pawsLauncherWindow::HandleUpdateButton(bool choice, void *updatewindow)
{
    pawsWidget* updateWindow = (pawsWidget*)updatewindow;
    psLaunchGUI->PerformUpdate(choice, false);
    updateWindow->Hide();
}

void pawsLauncherWindow::HandleCacheButton(bool choice, void* launcher)
{
    pawsLauncherWindow* launcherWindow = (pawsLauncherWindow*)launcher;
    if(choice)
    {
        pawsOkBox* notify = (pawsOkBox*)launcherWindow->FindWidget("Notify");
        if(launcherWindow->DeleteShaderCache())
        {
            notify->SetText("Shadercache deleted successfully");
        }
        else
        {
            notify->SetText("The shadercache couldn't be removed.\n"
                            "This is usually caused by the cache not"
                            " being present");
        }
        launcherWindow->FindWidget("ShaderCacheRequest")->Hide();
        notify->Show();
    }
}

void pawsLauncherWindow::HandleAspectRatio(csString ratio)
{
    const char* current = resolution->GetSelectedRowString();
    resolution->Clear();

    if(ratio == "17:9")
    {
        resolution->NewOption("2048x1080");
        if(!resolution->Select(current))
        {
            resolution->Select("2048x1080");
        }
    }
    else if(ratio == "16:10")
    {
        resolution->NewOption("2560x1600");
        resolution->NewOption("1920x1200");
        resolution->NewOption("1680x1050");
        resolution->NewOption("1440x900");
        resolution->NewOption("1280x800");
        resolution->NewOption("320x200");
        if(!resolution->Select(current))
        {
            resolution->Select("1920x1200");
        }
    }
    else if(ratio == "16:9")
    {
        resolution->NewOption("2560x1440");
        resolution->NewOption("1920x1080");
        resolution->NewOption("1366x768");
        resolution->NewOption("1280x720");
        resolution->NewOption("854x480");
        if(!resolution->Select(current))
        {
            resolution->Select("1920x1080");
        }
    }
    else if(ratio == "5:4")
    {
        resolution->NewOption("2560x2048");
        resolution->NewOption("1280x1024");
        if(!resolution->Select(current))
        {
            resolution->Select("1280x1024");
        }
    }
    else if(ratio == "5:3")
    {
        resolution->NewOption("1280x768");
        resolution->NewOption("800x480");
        if(!resolution->Select(current))
        {
            resolution->Select("1280x768");
        }
    }
    else if(ratio == "4:3")
    {
        resolution->NewOption("2048x1536");
        resolution->NewOption("1600x1200");
        resolution->NewOption("1400x1050");
        resolution->NewOption("1280x960");
        resolution->NewOption("1024x768");
        resolution->NewOption("800x600");
        resolution->NewOption("768x576");
        resolution->NewOption("640x480");
        resolution->NewOption("320x240");
        if(!resolution->Select(current))
        {
            resolution->Select("1024x768");
        }
    }
    else if(ratio == "3:2")
    {
        resolution->NewOption("1440x960");
        resolution->NewOption("1280x854");
        resolution->NewOption("1152x768");
        resolution->NewOption("720x480");
        if(!resolution->Select(current))
        {
            resolution->Select("1280x854");
        }
    }
}

bool pawsLauncherWindow::DeleteShaderCache()
{
    return psLaunchGUI->GetFileUtil()->RemoveFile("/planeshift/userdata/shadercache");
}

void pawsLauncherWindow::LoadSettings()
{
    csConfigFile configPSC("/planeshift/psclient.cfg", psLaunchGUI->GetVFS());
    const csString languagepath = "/planeshift/lang/";

    // Aspect Ratio and Screen Resolution
    pawsComboBox* aspect = (pawsComboBox*)FindWidget("AspectRatio");
    aspect->Clear();
    aspect->NewOption("17:9");
    aspect->NewOption("16:10");
    aspect->NewOption("16:9");
    aspect->NewOption("5:4");
    aspect->NewOption("5:3");
    aspect->NewOption("4:3");
    aspect->NewOption("3:2");

    csString setting = configUser->GetStr("Video.AspectRatio");
    if(setting.Compare(""))
    {
        setting = configPSC.GetStr("Video.AspectRatio");
    }
    aspect->Select(setting);
    HandleAspectRatio(setting);

    setting = configUser->GetStr("Video.ScreenWidth");
    if(setting.Compare(""))
    {
        setting = configPSC.GetStr("Video.ScreenWidth");
    }

    csString height = configUser->GetStr("Video.ScreenHeight");
    if(height.Compare(""))
    {
        height = configPSC.GetStr("Video.ScreenHeight");
    }
    setting.AppendFmt("x%s", height.GetData());

    if(!resolution->Select(setting))
    {
        resolution->NewOption(setting);
        resolution->Select(setting);
    }

    // Full screen
    pawsCheckBox* fullscreen = (pawsCheckBox*)FindWidget("Fullscreen");
    if(configUser->KeyExists("Video.FullScreen"))
    {
        fullscreen->SetState(configUser->GetBool("Video.FullScreen"));
    }
    else
    {
        fullscreen->SetState(configPSC.GetBool("Video.FullScreen"));
    }

    // Sound
    pawsCheckBox* enableSound = (pawsCheckBox*)FindWidget("EnableSound");
    if(configUser->KeyExists("System.PlugIns.iSndSysRenderer"))
    {
        enableSound->SetState(strcmp(configUser->GetStr("System.PlugIns.iSndSysRenderer"), "crystalspace.sndsys.renderer.null"));
    }
    else
    {
        enableSound->SetState(strcmp(configPSC.GetStr("System.PlugIns.iSndSysRenderer"), "crystalspace.sndsys.renderer.null"));
    }

    bool openALAvailable;
    {
      // Try to load OpenAL plugin to see if it's supported
      csRef<iSndSysRenderer> oal = csQueryRegistryOrLoad<iSndSysRenderer> (
         PawsManager::GetSingleton().GetObjectRegistry(), "crystalspace.sndsys.renderer.openal", false);
      openALAvailable = oal.IsValid();
    }

    pawsComboBox* soundRenderer = (pawsComboBox*)FindWidget("SoundRenderer");
    soundRenderer->Clear();
    if (openALAvailable) soundRenderer->NewOption("OpenAL");
    soundRenderer->NewOption("Software");

    if(enableSound->GetState())
    {
        setting = configUser->GetStr("System.PlugIns.iSndSysRenderer");
    }
    else
    {
        setting = configPSC.GetStr("System.PlugIns.iSndSysRenderer");
    }

    if(openALAvailable && setting.Compare("crystalspace.sndsys.renderer.openal"))
    {
        soundRenderer->Select("OpenAL");
    }
    else
    {
        soundRenderer->Select("Software");
    }

    // Graphics
    pawsComboBox* screenDepth = (pawsComboBox*)FindWidget("ScreenDepth");
    screenDepth->Clear();
    screenDepth->NewOption("16");
    screenDepth->NewOption("32");

    setting = configUser->GetStr("Video.ScreenDepth");
    if(setting.Compare(""))
    {
        setting = configPSC.GetStr("Video.ScreenDepth");
    }
    screenDepth->Select(setting);

    // AA
    pawsComboBox* antiAliasing = (pawsComboBox*)FindWidget("AntiAiasing");
    antiAliasing->Clear();
    antiAliasing->NewOption("0x");
    antiAliasing->NewOption("2x");
    antiAliasing->NewOption("2xQ");
    antiAliasing->NewOption("4x");
    antiAliasing->NewOption("4xQ");
    antiAliasing->NewOption("8x");
    antiAliasing->NewOption("8xQ");
    antiAliasing->NewOption("16x");
    antiAliasing->NewOption("16xQ");

    setting = configUser->GetStr("Video.OpenGL.MultiSamples");
    if(setting.Compare(""))
    {
        setting = configPSC.GetStr("Video.OpenGL.MultiSamples");
    }
    bool quality;
    if(configUser->KeyExists("Video.OpenGL.MultisampleFavorQuality"))
    {
        quality = configUser->GetBool("Video.OpenGL.MultisampleFavorQuality");
    }
    else
    {
        quality = configPSC.GetBool("Video.OpenGL.MultisampleFavorQuality");
    }
    if(quality && !setting.Compare('0'))
    {
        setting.Append("xQ");
    }
    else
    {
        setting.Append("x");
    }
    antiAliasing->Select(setting);

    // Anistropic
    pawsComboBox* anistopicFiltering = (pawsComboBox*)FindWidget("AnisotropicFiltering");
    anistopicFiltering->Clear();
    anistopicFiltering->NewOption("1x");
    anistopicFiltering->NewOption("2x");
    anistopicFiltering->NewOption("4x");
    anistopicFiltering->NewOption("8x");
    anistopicFiltering->NewOption("16x");

    setting = configUser->GetStr("Video.OpenGL.TextureFilterAnisotropy");
    if(setting.Compare(""))
    {
        setting = configPSC.GetStr("Video.OpenGL.TextureFilterAnisotropy");
    }
    if(setting.Compare("0"))
    {   //compatibility with invalid settings
        setting = "1";
    }
    setting.Append("x");
    anistopicFiltering->Select(setting);

    // Texture Quality
    pawsComboBox* textureQuality = (pawsComboBox*)FindWidget("TextureQuality");
    textureQuality->Clear();
    textureQuality->NewOption("Highest");
    textureQuality->NewOption("High");
    textureQuality->NewOption("Medium");
    textureQuality->NewOption("Low");
    textureQuality->NewOption("Very Low");

    // TextureDownsample
    int ds = 0;
    if(configUser->KeyExists("Video.OpenGL.TextureDownsample"))
    {
      ds = configUser->GetInt("Video.OpenGL.TextureDownsample");
    }
    else
    {
      ds = configPSC.GetInt("Video.OpenGL.TextureDownsample", ds);
    }

    // TextureLODBias
    float tlb = 0.0f;
    if(configUser->KeyExists("Video.OpenGL.TextureLODBias"))
    {
      tlb = configUser->GetFloat("Video.OpenGL.TextureLODBias");
    }
    else
    {
      tlb = configPSC.GetFloat("Video.OpenGL.TextureLODBias", tlb);
    }

    switch(ds)
    {
    case 0:
        {
            if(tlb < 0)
            {
                setting = "Highest";
            }
            else
            {
                setting = "High";
            }
            break;
        }
    case 1:
        {
            setting = "Medium";
            break;
        }
    case 2:
        {
            setting = "Low";
            break;
        }
    case 4:
        {
            setting = "Very Low";
            break;
        }
    }
    textureQuality->Select(setting);

    // Shaders
    pawsComboBox* shaders = (pawsComboBox*)FindWidget("Shaders");
    shaders->Clear();
    shaders->NewOption("Highest");
    shaders->NewOption("High");
    shaders->NewOption("Medium");
    shaders->NewOption("Low");
    shaders->NewOption("Lowest");

    setting = configUser->GetStr("PlaneShift.Graphics.Shaders");
    if(setting.Compare(""))
    {
        setting = configPSC.GetStr("PlaneShift.Graphics.Shaders");
    }
    shaders->Select(setting);

    // Real time Shadows (not used for now)
    pawsCheckBox* enableShadows = (pawsCheckBox*)FindWidget("EnableShadows");
    if(configUser->KeyExists("PlaneShift.Graphics.Shadows"))
    {
        enableShadows->SetState(configUser->GetBool("PlaneShift.Graphics.Shadows"));
    }
    else
    {
        enableShadows->SetState(configPSC.GetBool("PlaneShift.Graphics.Shadows"));
    }

    // Enable Bloom and HDR
    pawsCheckBox* enableBloom = (pawsCheckBox*)FindWidget("EnableBloom");
    pawsCheckBox* enableHDR = (pawsCheckBox*)FindWidget("EnableHDR");
    if(enableShadows->GetState())
    {
        enableBloom->SetState(configUser->KeyExists("RenderManager.Unshadowed.Effects"));
        enableHDR->SetState(configUser->KeyExists("RenderManager.Unshadowed.HDR.Enabled") &&
            configUser->GetBool("RenderManager.Unshadowed.HDR.Enabled"));
    }
    else
    {
        enableBloom->SetState(configUser->KeyExists("RenderManager.ShadowPSSM.Effects"));
        enableHDR->SetState(configUser->KeyExists("RenderManager.ShadowPSSM.HDR.Enabled") &&
            configUser->GetBool("RenderManager.ShadowPSSM.HDR.Enabled"));
    }

    // Grass
    pawsCheckBox* enableGrass = (pawsCheckBox*)FindWidget("EnableGrass");
    if(configUser->KeyExists("PlaneShift.Graphics.EnableGrass"))
    {
        enableGrass->SetState(configUser->GetBool("PlaneShift.Graphics.EnableGrass"));
    }
    else
    {
        enableGrass->SetState(configPSC.GetBool("PlaneShift.Graphics.EnableGrass"));
    }

    // Weather
    pawsCheckBox* enableWeather = (pawsCheckBox*)FindWidget("EnableWeather");
    if(configUser->KeyExists("PlaneShift.Weather.Enabled"))
    {
        enableWeather->SetState(configUser->GetBool("PlaneShift.Weather.Enabled"));
    }
    else
    {
        enableWeather->SetState(configPSC.GetBool("PlaneShift.Weather.Enabled"));
    }

    // VBO
    pawsCheckBox* VBO = (pawsCheckBox*)FindWidget("VBO");
    if(configUser->KeyExists("Video.OpenGL.UseExtension.GL_ARB_vertex_buffer_object"))
    {
        VBO->SetState(configUser->GetBool("Video.OpenGL.UseExtension.GL_ARB_vertex_buffer_object"));
    }
    else
    {
        VBO->SetState(configPSC.GetBool("Video.OpenGL.UseExtension.GL_ARB_vertex_buffer_object"));
    }

    // Loader Cache
    pawsCheckBox* loaderCache = (pawsCheckBox*)FindWidget("LoaderCache");
    if(configUser->KeyExists("Planeshift.Loading.Cache"))
    {
        loaderCache->SetState(configUser->GetBool("Planeshift.Loading.Cache"));
    }
    else
    {
        loaderCache->SetState(configPSC.GetBool("Planeshift.Loading.Cache"));
    }

    // Background Loading
    bool alwaysRunNow = true;
    pawsComboBox* backgroundLoading = (pawsComboBox*)FindWidget("BackgroundLoading");
    backgroundLoading->Clear();
    backgroundLoading->NewOption("World");
    backgroundLoading->NewOption("Models");
    backgroundLoading->NewOption("Off");
    if(configUser->KeyExists("ThreadManager.AlwaysRunNow"))
    {
        alwaysRunNow = configUser->GetBool("ThreadManager.AlwaysRunNow");
    }
    else
    {
        alwaysRunNow = configPSC.GetBool("ThreadManager.AlwaysRunNow");
    }

    if (alwaysRunNow) {
        backgroundLoading->Select("Off");
    }
    else if(configUser->KeyExists("PlaneShift.Loading.BackgroundWorldLoading"))
    {
        if(configUser->GetBool("PlaneShift.Loading.BackgroundWorldLoading"))
        {
            backgroundLoading->Select("World");
        }
        else
        {
            backgroundLoading->Select("Models");
        }
    }
    else
    {
        if(configPSC.GetBool("PlaneShift.Loading.BackgroundWorldLoading"))
        {
            backgroundLoading->Select("World");
        }
        else
        {
            backgroundLoading->Select("Models");
        }
    }

    // Particles
    pawsComboBox* particles = (pawsComboBox*)FindWidget("Particles");
    particles->Clear();
    particles->NewOption("High");
    particles->NewOption("Medium");
    if(configUser->KeyExists("PlaneShift.Graphics.Particles"))
    {
        particles->Select(configUser->GetStr("PlaneShift.Graphics.Particles", "High"));
    }
    else
    {
        particles->Select(configPSC.GetStr("PlaneShift.Graphics.Particles", "High"));
    }

    // Fill the languages
    pawsComboBox* languages = (pawsComboBox*)FindWidget("Languages");
    languages->Clear();
    
    csRef<iStringArray> langfiles = psLaunchGUI->GetVFS()->FindFiles(languagepath); //find the folders with languages
    for (size_t i = 0; i < langfiles->GetSize(); i++)
    {
        csString file = langfiles->Get(i);
        if(psLaunchGUI->GetVFS()->Exists(csString(file+"stringtable.xml"))) //check if it's an usable language
        {
            file = file.Slice(languagepath.Length(),file.Length()-languagepath.Length());
            languages->NewOption(file.Slice(0,file.Length()-1)); //add found language names based on folder
        }
    }

    // Load the current language
    csString language = configUser->GetStr("PlaneShift.GUI.Language");	
    if(!strcmp(language,""))
    {
        // Try loading the default language.
        language = configPSC.GetStr("PlaneShift.GUI.Language");
        if(!strcmp(language,""))
        {
            // No language selected.. shouldn't happen but it's not fatal.
            languages->Select("english"); //english is the default fallback
            return;
        }
    }

    languages->Select(language);

    // Fill the skins
    pawsComboBox* skins = (pawsComboBox*)FindWidget("Skins");
    skins->Clear();
    csString skinPath = configPSC.GetStr("PlaneShift.GUI.Skin.Dir");
    csRef<iStringArray> files = psLaunchGUI->GetVFS()->FindFiles(skinPath);
    for (size_t i = 0; i < files->GetSize(); i++)
    {
        csString file = files->Get(i);
        file = file.Slice(skinPath.Length(),file.Length()-skinPath.Length());

        size_t dot = file.FindLast('.');
        csString ext = file.Slice(dot+1,3);
        if (ext == "zip")
        {
            skins->NewOption(file.Slice(0,dot));
        }
    }

    // Load the current skin
    csString skin = configUser->GetStr("PlaneShift.GUI.Skin.Selected");	
    if(!strcmp(skin,""))
    {
        // Try loading the default skin.
        skin = configPSC.GetStr("PlaneShift.GUI.Skin.Selected");
        if(!strcmp(skin,""))
        {
            // No skin selected.. shouldn't happen but it's not fatal.
            return;
        }
    }
    printf("Skin loaded: %s\n", skin.GetData());
    LoadSkin(skin);

    // Graphics Preset
    pawsComboBox* graphicsPreset = (pawsComboBox*)FindWidget("GraphicsPreset");
    graphicsPreset->Clear();
    graphicsPreset->NewOption("Highest");
    graphicsPreset->NewOption("High");
    graphicsPreset->NewOption("Medium");
    graphicsPreset->NewOption("Low");
    graphicsPreset->NewOption("Lowest");
    graphicsPreset->NewOption("Custom");

    setting = configUser->GetStr("PlaneShift.Graphics.Preset");
    if(setting.Compare(""))
    {
        setting = configPSC.GetStr("PlaneShift.Graphics.Preset", "Custom");
    }
    graphicsPreset->Select(setting);
}

void pawsLauncherWindow::SaveSettings()
{
    // Graphics Preset
    pawsComboBox* graphicsPreset = (pawsComboBox*)FindWidget("GraphicsPreset");
    configUser->SetStr("PlaneShift.Graphics.Preset", graphicsPreset->GetSelectedRowString());

    // Common
    pawsComboBox* shaders = (pawsComboBox*)FindWidget("Shaders");
    csString shaderSelection = shaders->GetSelectedRowString();

    pawsCheckBox* enableShadows = (pawsCheckBox*)FindWidget("EnableShadows");
    pawsCheckBox* enableBloom = (pawsCheckBox*)FindWidget("EnableBloom");
    pawsCheckBox* enableHDR = (pawsCheckBox*)FindWidget("EnableHDR");

    switch(graphicsPreset->GetSelectedRowNum())
    {
    case HIGHEST:
        {
            enableShadows->SetState(true);
            enableBloom->SetState(true);
            enableHDR->SetState(true);
            shaderSelection = "Highest";
            configUser->SetInt("Video.ScreenDepth", 32);
            configUser->SetInt("Video.OpenGL.MultiSamples", 8);
            configUser->SetInt("Video.OpenGL.TextureFilterAnisotropy", 16);
            configUser->SetFloat("Video.OpenGL.TextureLODBias", -0.1f);
            configUser->SetInt("Video.OpenGL.TextureDownsample", 0);
            configUser->SetBool("PlaneShift.Graphics.EnableGrass", true);
            configUser->SetBool("Video.OpenGL.UseExtension.GL_ARB_vertex_buffer_object", true);
            configUser->SetStr("PlaneShift.Graphics.Particles", "High");
            break;
        }
    case HIGH:
        {
            enableBloom->SetState(true);
            shaderSelection = "High";
            configUser->SetInt("Video.ScreenDepth", 32);
            configUser->SetInt("Video.OpenGL.MultiSamples", 4);
            configUser->SetInt("Video.OpenGL.TextureFilterAnisotropy", 8);
            configUser->SetInt("Video.OpenGL.TextureDownsample", 0);
            configUser->SetBool("PlaneShift.Graphics.EnableGrass", true);
            configUser->SetBool("Video.OpenGL.UseExtension.GL_ARB_vertex_buffer_object", true);
            configUser->SetStr("PlaneShift.Graphics.Particles", "High");
            break;
        }
    case MEDIUM:
        {
            shaderSelection = "Medium";
            configUser->SetInt("Video.ScreenDepth", 32);
            configUser->SetInt("Video.OpenGL.MultiSamples", 2);
            configUser->SetInt("Video.OpenGL.TextureFilterAnisotropy", 4);
            configUser->SetInt("Video.OpenGL.TextureDownsample", 0);
            configUser->SetBool("PlaneShift.Graphics.EnableGrass", true);
            configUser->SetBool("Video.OpenGL.UseExtension.GL_ARB_vertex_buffer_object", true);
            configUser->SetStr("PlaneShift.Graphics.Particles", "Medium");
            break;
        }
    case LOW:
        {
            shaderSelection = "Low";
            configUser->SetInt("Video.ScreenDepth", 32);
            configUser->SetInt("Video.OpenGL.MultiSamples", 0);
            configUser->SetInt("Video.OpenGL.TextureFilterAnisotropy", 1);
            configUser->SetInt("Video.OpenGL.TextureDownsample", 2);
            configUser->SetBool("PlaneShift.Graphics.EnableGrass", false);
            configUser->SetBool("Video.OpenGL.UseExtension.GL_ARB_vertex_buffer_object", true);
            configUser->SetStr("PlaneShift.Graphics.Particles", "Medium");
            configUser->SetBool("PlaneShift.Weather.Enabled", false);
            break;
        }
    case LOWEST:
        {
            shaderSelection = "Lowest";
            configUser->SetInt("Video.ScreenDepth", 16);
            configUser->SetInt("Video.OpenGL.MultiSamples", 0);
            configUser->SetInt("Video.OpenGL.TextureFilterAnisotropy", 1);
            configUser->SetInt("Video.OpenGL.TextureDownsample", 4);
            configUser->SetBool("PlaneShift.Graphics.EnableGrass", false);
            configUser->SetBool("Video.OpenGL.UseExtension.GL_ARB_vertex_buffer_object", true);
            configUser->SetStr("PlaneShift.Graphics.Particles", "Medium");
            configUser->SetBool("PlaneShift.Weather.Enabled", false);
            break;
        }
    case CUSTOM:
        {
            pawsComboBox* screenDepth = (pawsComboBox*)FindWidget("ScreenDepth");
            configUser->SetStr("Video.ScreenDepth", screenDepth->GetSelectedRowString());

            pawsComboBox* antiAliasing = (pawsComboBox*)FindWidget("AntiAiasing");
            csString selected = antiAliasing->GetSelectedRowString();
            configUser->SetStr("Video.OpenGL.MultiSamples", selected.Slice(0, selected.FindFirst("x")));

            pawsComboBox* anistopicFiltering = (pawsComboBox*)FindWidget("AnisotropicFiltering");
            selected = anistopicFiltering->GetSelectedRowString();
            configUser->SetStr("Video.OpenGL.TextureFilterAnisotropy", selected.Slice(0, selected.FindFirst("x")));

            pawsComboBox* textureQuality = (pawsComboBox*)FindWidget("TextureQuality");

            float texlodbias = 0;
            int downsample = 0;

            if(textureQuality->GetSelectedRowString().Compare("Highest"))
            {
                texlodbias = -0.1f;
            }
            if(textureQuality->GetSelectedRowString().Compare("Medium"))
            {
                downsample = 1;
            }
            else if(textureQuality->GetSelectedRowString().Compare("Low"))
            {
                downsample = 2;
            }
            else if(textureQuality->GetSelectedRowString().Compare("Very Low"))
            {
                downsample = 4;
            }

            configUser->SetFloat("Video.OpenGL.TextureLODBias", texlodbias);
            configUser->SetInt("Video.OpenGL.TextureDownsample", downsample);

            pawsCheckBox* enableGrass = (pawsCheckBox*)FindWidget("EnableGrass");
            configUser->SetBool("PlaneShift.Graphics.EnableGrass", enableGrass->GetState());

            pawsCheckBox* enableWeather = (pawsCheckBox*)FindWidget("EnableWeather");
            configUser->SetBool("PlaneShift.Weather.Enabled", enableWeather->GetState());

            pawsCheckBox* VBO = (pawsCheckBox*)FindWidget("VBO");
            configUser->SetBool("Video.OpenGL.UseExtension.GL_ARB_vertex_buffer_object", VBO->GetState());

            pawsComboBox* particles = (pawsComboBox*)FindWidget("Particles");
            configUser->SetStr("PlaneShift.Graphics.Particles", particles->GetSelectedRowString());
            break;
        }
    };

    configUser->SetStr("PlaneShift.Graphics.Shaders", shaderSelection);

    // Reset common options.
    configUser->SetStr("Engine.RenderManager.Default", "crystalspace.rendermanager.unshadowed");
    configUser->DeleteKey("PlaneShift.Graphics.Shadows");
    configUser->DeleteKey("Video.ShaderManager.Tags.per_pixel_lighting.Presence");
    configUser->DeleteKey("RenderManager.Unshadowed.Layers");
    configUser->DeleteKey("RenderManager.Unshadowed.Effects");
    configUser->DeleteKey("RenderManager.ShadowPSSM.Effects");
    configUser->DeleteKey("RenderManager.Unshadowed.HDR.Enabled");
    configUser->DeleteKey("RenderManager.ShadowPSSM.HDR.Enabled");

    if(shaderSelection == "High" || shaderSelection == "Highest")
    {
        //configUser->SetBool("PlaneShift.Graphics.Shadows", enableShadows->GetState());
        if(enableShadows->GetState())
        {
            // Not working yet.
            //configUser->SetStr("Engine.RenderManager.Default", "crystalspace.rendermanager.shadow_pssm");
        }

        if(enableBloom->GetState())
        {
            configUser->SetStr("RenderManager.Unshadowed.Effects", "/data/posteffects/bloom.xml");
            configUser->SetStr("RenderManager.ShadowPSSM.Effects", "/data/posteffects/bloom.xml");
        }
        configUser->SetBool("RenderManager.Unshadowed.HDR.Enabled", enableHDR->GetState());
        configUser->SetBool("RenderManager.ShadowPSSM.HDR.Enabled", enableHDR->GetState());
    }
    else if(shaderSelection == "Medium")
    {
        configUser->SetStr("RenderManager.Unshadowed.Layers", "/data/renderlayers/lighting_default_pvl.xml");
    }
    else if(shaderSelection == "Low")
    {
        configUser->SetStr("RenderManager.Unshadowed.Layers", "/data/renderlayers/lighting_simple.xml"); 
        configUser->SetStr("Video.ShaderManager.Tags.per_pixel_lighting.Presence", "forbidden");
    }
    else if(shaderSelection == "Lowest")
    {
        configUser->SetStr("RenderManager.Unshadowed.Layers", "/data/renderlayers/lighting_basic.xml"); 
        configUser->SetStr("Video.ShaderManager.Tags.per_pixel_lighting.Presence", "forbidden");
    }

    pawsCheckBox* loaderCache = (pawsCheckBox*)FindWidget("LoaderCache");
    configUser->SetBool("Planeshift.Loading.Cache", loaderCache->GetState());

    pawsComboBox* backgroundLoading = (pawsComboBox*)FindWidget("BackgroundLoading");
    if(backgroundLoading->GetSelectedRowString() == "World")
    {
        configUser->SetBool("PlaneShift.Loading.BackgroundWorldLoading", true);
        configUser->SetBool("ThreadManager.AlwaysRunNow", false);
    }
    else if(backgroundLoading->GetSelectedRowString() == "Models")
    {
        configUser->SetBool("PlaneShift.Loading.BackgroundWorldLoading", false);
        configUser->SetBool("ThreadManager.AlwaysRunNow", false);
    }
    else if(backgroundLoading->GetSelectedRowString() == "Off")
    {
        configUser->SetBool("PlaneShift.Loading.BackgroundWorldLoading", false);
        configUser->SetBool("ThreadManager.AlwaysRunNow", true);
    }

    // Aspect Ratio and Screen Resolution
    pawsComboBox* aspect = (pawsComboBox*)FindWidget("AspectRatio");
    configUser->SetStr("Video.AspectRatio", aspect->GetSelectedRowString());
    csString res = resolution->GetSelectedRowString();
    configUser->SetStr("Video.ScreenWidth", res.Slice(0, res.FindFirst('x')));
    configUser->SetStr("Video.ScreenHeight", res.Slice(res.FindFirst('x')+1));

    // Full screen
    pawsCheckBox* fullscreen = (pawsCheckBox*)FindWidget("Fullscreen");
    configUser->SetBool("Video.FullScreen", fullscreen->GetState());

    // Sound
    pawsCheckBox* enableSound = (pawsCheckBox*)FindWidget("EnableSound");
    if(enableSound->GetState())
    {
        pawsComboBox* soundRenderer = (pawsComboBox*)FindWidget("SoundRenderer");
        if(soundRenderer->GetSelectedRowString().Compare("OpenAL"))
        {
            configUser->SetStr("System.PlugIns.iSndSysRenderer", "crystalspace.sndsys.renderer.openal");
        }
        else
        {
            configUser->SetStr("System.PlugIns.iSndSysRenderer", "crystalspace.sndsys.renderer.software");
        }
        configUser->SetStr("System.Plugins.iSoundManager", "crystalspace.planeshift.sound.soundmngr");
    }
    else
    {
        configUser->SetStr("System.PlugIns.iSndSysRenderer", "crystalspace.sndsys.renderer.null");
        configUser->SetStr("System.Plugins.iSoundManager", "crystalspace.planeshift.sound.dummy");
    }

    pawsComboBox* languages = (pawsComboBox*)FindWidget("Languages");
    configUser->SetStr("PlaneShift.GUI.Language", languages->GetSelectedRowString());

    pawsComboBox* skins = (pawsComboBox*)FindWidget("Skins");
    configUser->SetStr("PlaneShift.GUI.Skin.Selected", skins->GetSelectedRowString().Append(".zip"));

    // Save everything
    configUser->Save();
}

void pawsLauncherWindow::LoadSkin(const char* name)
{
    pawsComboBox* skins = (pawsComboBox*)FindWidget("Skins");
    pawsMultiLineTextBox* desc = (pawsMultiLineTextBox*)FindWidget("SkinDescription");
    pawsWidget* preview = FindWidget("SkinPreview");
    pawsButton* previewBtn = (pawsButton*)FindWidget("PreviewButton");
    pawsCheckBox* previewBox = (pawsCheckBox*)FindWidget("PreviewBox");

    csConfigFile configPSC("/planeshift/psclient.cfg", psLaunchGUI->GetVFS());
    csString skinPath = configPSC.GetStr("PlaneShift.GUI.Skin.Dir");

	// Create full path to skin.
    csString zip = skinPath + name;

    // This .zip could be a file or a dir
    csString slash(CS_PATH_SEPARATOR);
    if (psLaunchGUI->GetVFS()->Exists(zip + slash))
        zip += slash;
    else if(psLaunchGUI->GetVFS()->Exists(zip + ".zip"))
    {
        zip += ".zip";
    }
    else if(psLaunchGUI->GetVFS()->Exists(zip + ".zip" + slash))
    {
        zip += ".zip";
        zip += slash;
    }

    if (!psLaunchGUI->GetVFS()->Exists(zip))
    {
        printf("Current skin %s doesn't exist, skipping..\n", zip.GetData());
        return;
    }

    // Make sure the skin is selected
    if(skins->GetSelectedRowString() != name)
    {
        if(!skins->Select(name))
        {
            if(!skins->Select(csString(name).Slice(0, csString(name).FindLast('.'))))
            {
                return;
            }
        }
    }

    // Get the path
    csRef<iDataBuffer> real = psLaunchGUI->GetVFS()->GetRealPath(zip);
    
    // Mount the skin
    psLaunchGUI->GetVFS()->Unmount("/skin", mountedPath);
    psLaunchGUI->GetVFS()->Mount("/skin", real->GetData());
    mountedPath = real->GetData();

    // Parse XML
    csRef<iDocument> xml = ParseFile(PawsManager::GetSingleton().GetObjectRegistry(),"/skin/skin.xml");
    if(!xml)
    {
        Error1("Parse error in /skin/skin.xml");
        return;
    }
    csRef<iDocumentNode> root = xml->GetRoot();
    if(!root)
    {
        Error1("No XML root in /skin/skin.xml");
        return;
    }
    root = root->GetNode("xml");
    if(!root)
    {
        Error1("No <xml> tag in /skin/skin.xml");
        return;
    }
    csRef<iDocumentNode> skinInfo = root->GetNode("skin_information");
    if(!skinInfo)
    {
        Error1("No <skin_information> in /skin/skin.xml");
        return;
    }

    // Read XML
    csRef<iDocumentNode> nameNode           = skinInfo->GetNode("name");
    csRef<iDocumentNode> authorNode         = skinInfo->GetNode("author");
    csRef<iDocumentNode> descriptionNode    = skinInfo->GetNode("description");
    csRef<iDocumentNode> mountNode          = root->GetNode("mount_path");

    bool success = true;

    if(!mountNode)
    {
        printf("skin.xml is missing mount_path!\n");
        success = false;
    }

    if(!authorNode)
    {
        printf("skin.xml is missing author!\n");
        success = false;
    }

    if(!descriptionNode)
    {
        printf("skin.xml is missing description!\n");
        success = false;
    }

    if(!nameNode)
    {
        printf("skin.xml is missing name!\n");
        success = false;
    }

    if(!success)
        return;


    // Move data to variables
    csString skinname,author,description;
    skinname = nameNode->GetContentsValue();
    author = authorNode->GetContentsValue();
    description = descriptionNode->GetContentsValue();

    currentSkin = name;

    // Print the info
    csString info;
    info.Format("%s\nAuthor: %s\n%s",skinname.GetData(),author.GetData(),description.GetData());
    desc->SetText(info);

    // Reset the backgrounds
    preview->RemoveTitle();
    preview->SetBackground("Blue Background");
    previewBtn->SetBackground("Blue Background");
    previewBox->SetImages("radiooff","radioon");

    // Load the skin
    success = success && LoadResource("Examine Background","skintest_bg",mountNode->GetContentsValue());
    success = success && LoadResource("Standard Button","skintest_btn",mountNode->GetContentsValue());
    success = success && LoadResource("Blue Title","skintest_title",mountNode->GetContentsValue());
    success = success && LoadResource("radiooff","skintest_roff",mountNode->GetContentsValue());
    success = success && LoadResource("radioon","skintest_ron",mountNode->GetContentsValue());
    success = success && LoadResource("quit","quit",mountNode->GetContentsValue());

    if(!success)
    {
        csString str;
        str.Format("Couldn't load skin %s! Check the console for detailed output.", skinname.GetData());
        //PawsManager::GetSingleton().CreateWarningBox(str);
        return;
    }

    preview->SetTitle("Skin preview","skintest_title","center","true");
    preview->SetMaxAlpha(1);
    preview->SetBackground("skintest_bg");

    previewBtn->SetMaxAlpha(1);
    previewBtn->SetBackground("skintest_btn");

    previewBox->SetImages("skintest_roff","skintest_ron");
}

bool pawsLauncherWindow::LoadResource(const char* resource,const char* resname, const char* mountPath)
{
    // Remove the resource if it exists
    PawsManager::GetSingleton().GetTextureManager()->Remove(resname);

    // Open the image list
    csRef<iDocument> xml = ParseFile(PawsManager::GetSingleton().GetObjectRegistry(),"/skin/imagelist.xml");
    if(!xml)
    {
        Error1("Parse error in /skin/imagelist.xml");
        return false;
    }
    csRef<iDocumentNode> root = xml->GetRoot();
    if(!root)
    {
        Error1("No XML root in /skin/imagelist.xml");
        return false;
    }
    csRef<iDocumentNode> topNode = root->GetNode("image_list");
    if(!topNode)
    {
        Error1("No <image_list> in /skin/imagelist.xml");
        return false;
    }

    csRef<iDocumentNodeIterator> iter = topNode->GetNodes();

    // Find the resource
    csString filename;
    while ( iter->HasNext() )
    {
        csRef<iDocumentNode> node = iter->Next();

        if ( node->GetType() != CS_NODE_ELEMENT )
            continue;

        if ( strcmp( node->GetValue(), "image" ) == 0 )
        {
            if(!strcmp(node->GetAttributeValue( "resource" ),resource))
            {
                filename = node->GetAttributeValue( "file" );
                break;
            }
        }
    }

    if(!filename.Length())
    {
        printf("Couldn't locate resource %s in the current skin!\n",resource);
        return false;
    }


    // Skin uses /paws/skin which we can't use since the app is already using that
    // So we need to replace that with /skin which we can and do use
    filename.DeleteAt(0,strlen(mountPath));
    filename.Insert(0,"/skin/");

    if(!psLaunchGUI->GetVFS()->Exists(filename))
    {
        printf("Skin is missing the '%s' resource at file '%s'!\n",resource,filename.GetData());
        return false;
    }

    csRef<iPawsImage> img;
    img.AttachNew(new pawsImageDrawable(filename.GetData(), resname, false, csRect(), 0, 0, 0, 0));
    PawsManager::GetSingleton().GetTextureManager()->AddPawsImage(img);

    return true;
}

void pawsLauncherWindow::OnListAction(pawsListBox* widget, int /*status*/)
{
    switch(widget->GetID())
    {
    case SKINS:
        {
            pawsComboBox* skins = (pawsComboBox*)FindWidget("Skins");
            csString selected = skins->GetSelectedRowString();
            if(!selected.Compare(currentSkin))
            {
                LoadSkin(selected);
            }
            return;
        }
    case ASPECT_RATIO:
        {
            pawsComboBox* aspect = (pawsComboBox*)FindWidget("AspectRatio");
            HandleAspectRatio(aspect->GetSelectedRowString());
            return;
        }
    }

    if(13100 < widget->GetID() && widget->GetID() < 13200)
    {
        pawsComboBox* graphicsPreset = (pawsComboBox*)FindWidget("GraphicsPreset");
        graphicsPreset->Select("Custom");
    }
}
