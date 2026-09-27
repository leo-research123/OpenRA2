#include "ui_resources.hpp"
#include "yrpp/MouseClass.h"
#include "yrpp/MixFileClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/CCINIClass.h"
#include "yrpp/Drawing.h"
#include "yrpp/BeaconManagerClass.h"
#include "rules_runtime.hpp"
#include <cstdio>

namespace game {
namespace { UiResources* beacon_art_owner=nullptr; }
void UiResources::clear() noexcept {
    if(beacon_art_owner==this) {
        BeaconManagerClass::ReleaseArt();beacon_art_owner=nullptr;
    }
    // Retire borrowed original art before its owner frees the SHP references.
    const auto* diplomacy=image(UiImage::briefing);
    const auto* options=image(UiImage::options);
    if ((diplomacy && RadarClass::DiplomacyShape==diplomacy) ||
        (options && RadarClass::OptionsShape==options)) {
        RadarClass::Instance.SetRadarButtonsVisible(false);
        for(auto* button:{&RadarClass::DiplomacyButton,&RadarClass::OptionsButton}){
            if(GadgetClass::StuckOn==button)GadgetClass::StuckOn=nullptr;
            if(GadgetClass::Hovered==button)GadgetClass::Hovered=nullptr;
            button->IsPressed=false;
        }
        RadarClass::DiplomacyButton.SetShape(nullptr,0,0);
        RadarClass::OptionsButton.SetShape(nullptr,0,0);
        RadarClass::Instance.DisposeOfArt();
    }
    // Sidebar command controls borrow the same per-side art. Release both
    // their device capture and their pointer before a faction reload frees it.
    ShapeButtonClass* commands[]{&SidebarClass::ToggleRepairButton,&SidebarClass::ToggleSellButton};
    const UiImage kinds[]{UiImage::repair,UiImage::sell};
    for(int i=0;i<2;++i)if(image(kinds[i]) && commands[i]->ShapeData==image(kinds[i])) {
        GScreenClass::Instance.RemoveButton(commands[i]);
        commands[i]->IsPressed=false;commands[i]->SetShape(nullptr,0,0);
    }
    if (image(UiImage::radar) && RadarClass::RadarAnim==image(UiImage::radar)) RadarClass::RadarAnim=nullptr;
    for (auto& image : images_) image.reset();
    if(BitFont::Instance==font_.get())BitFont::Instance=nullptr;
    font_.reset();
    side_=-1;
}
bool UiResources::load(int side) noexcept {
    if (side_==side) return true;
    clear(); error_[0]=0;
    try {
        if (!MixFileClass::LoadSidebarMixes(side)) {
            std::snprintf(error_,sizeof(error_),"Could not mount sidebar archives for side %d",side); return false;
        }
        font_=std::make_unique<BitFont>("GAME.FNT");
        if (!font_->InternalPTR) {
            std::snprintf(error_,sizeof(error_),"Could not read original GAME.FNT"); clear(); return false;
        }
        CCFileClass palette_file("SIDEBAR.PAL");
        if (palette_file.ReadBytes(&palette_,sizeof(palette_))!=sizeof(palette_)) {
            std::snprintf(error_,sizeof(error_),"Could not read SIDEBAR.PAL"); return false;
        }
        for (auto& color : palette_.Entries) { color.R<<=2; color.G<<=2; color.B<<=2; }
        CCFileClass cameo_file("CAMEO.PAL");
        if(cameo_file.ReadBytes(&cameo_palette_,sizeof(cameo_palette_))!=sizeof(cameo_palette_)){std::snprintf(error_,sizeof(error_),"Could not read CAMEO.PAL");return false;}
        for(auto& color:cameo_palette_.Entries){color.R<<=2;color.G<<=2;color.B<<=2;}
        const char* names[]{"CREDITS.SHP","TOP.SHP",side==2 ? "RADARY.SHP" : "RADAR.SHP",
            "SIDE1.SHP","SIDE2.SHP","SIDE2B.SHP","SIDE3.SHP","ADDON.SHP",
            "LSPACER.SHP","LENDCAP.SHP","BTTNBKGD.SHP","RENDCAP.SHP","REPAIR.SHP","SELL.SHP",
            "R-DN.SHP","R-UP.SHP","DIPLOBTN.SHP","OPTBTN.SHP","TAB00.SHP","TAB01.SHP","TAB02.SHP","TAB03.SHP","POWERP.SHP","GCLOCK2.SHP"};
        for (unsigned i=0;i<images_.size();++i) {
            char button[32]; const char* name;
            if (i<static_cast<unsigned>(UiImage::button0)) name=names[i];
            else { std::snprintf(button,sizeof(button),"Button%02u.SHP",i-static_cast<unsigned>(UiImage::button0)); name=button; }
            CCFileClass probe(name);
            if (!probe.Exists()) {
                if (i>=static_cast<unsigned>(UiImage::button0)) continue; // Unassigned command slots may have no shape.
                std::snprintf(error_,sizeof(error_),"Missing original UI image: %s",name); clear(); return false;
            }
            auto image=std::make_unique<SHPReference>(name);
            image->Load();
            auto* data=image->GetData();
            if (!data || data->Width<=0 || data->Height<=0 || data->Frames<=0) {
                std::snprintf(error_,sizeof(error_),"Invalid original UI image: %s",name); clear(); return false;
            }
            images_[i]=std::move(image);
        }
        CCFileClass ui_file("UIMD.INI"); CCINIClass ini;
        if (ini.ReadCCFile(&ui_file,false,false)<=0) {
            std::snprintf(error_,sizeof(error_),"Could not read UIMD.INI"); clear(); return false;
        }
        const auto* defaults=default_rules_runtime();
        auto runtime=defaults ? *defaults : RulesRuntimeServices{};
        static const int no_command=-1;
        runtime.no_command=&no_command;
        runtime.command_position=[](void*,int command,int position) noexcept {
            if (command<0 || command>=25) return false;
            TabClass::CommandPositions[command]=position; return true;
        };
        runtime.command_count=[](void*,int count) noexcept { TabClass::CommandCount=count; return count>=0; };
        bool parsed=false;
        struct Read { CCINIClass& ini; bool& parsed; } read{ini,parsed};
        if (!with_rules_runtime(runtime,[](void* pointer) {
            auto& r=*static_cast<Read*>(pointer); r.parsed=RulesClass::Read_AdvancedCommandBar(&r.ini,false);
        },&read) || !parsed) {
            std::snprintf(error_,sizeof(error_),"Could not read original advanced command bar"); clear(); return false;
        }
        Drawing::SetTooltipColorForSide(side);
        BeaconManagerClass::Instance.LoadArt();beacon_art_owner=this;
        BitFont::Instance=font_.get();
        side_=side;
        return true;
    } catch (...) { std::snprintf(error_,sizeof(error_),"Original UI resource loading failed"); clear(); return false; }
}
}
