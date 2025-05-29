/**
 * Copyright (C) 2019 - 2020 WerWolv
 * 
 * This file is part of EdiZon
 * 
 * EdiZon is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 * 
 * EdiZon is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with EdiZon.  If not, see <http://www.gnu.org/licenses/>.
 */

#define TESLA_INIT_IMPL
#include <tesla.hpp>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <switch.h>
#include <filesystem>

#include <switch/nro.h>
#include <switch/nacp.h>

#include <edizon.h>
#include <helpers/util.h>
#include "utils.hpp"
#include "cheat.hpp"

#include <unistd.h>
#include <netinet/in.h>

using namespace tsl;

class GuiMain : public tsl::Gui {
public:
    GuiMain(std::string section) { 
        this->m_section = section;
    }
    ~GuiMain() { }

    virtual tsl::elm::Element* createUI() {
        auto rootFrame = new tsl::elm::OverlayFrame("PluginName"_tr, VERSION);
        if (!edz::cheat::CheatManager::hasCheatProcess()) {
            auto warning = new tsl::elm::CustomDrawer([](tsl::gfx::Renderer *renderer, u16 x, u16 y, u16 w, u16 h){
                renderer->drawString("\uE150", false, 180, 250, 90, renderer->a(0xFFFF));
                renderer->drawString("NoCheatsLoadedErrorCheatsCustomDrawerText"_tr.c_str(), false, 110, 340, 25, renderer->a(0xFFFF));
            });
            rootFrame->setContent(warning);
        } else {
            if (edz::cheat::CheatManager::getCheats().size() == 0) {
                auto warning = new tsl::elm::CustomDrawer([](tsl::gfx::Renderer *renderer, u16 x, u16 y, u16 w, u16 h){
                    renderer->drawString(("AppIDMainCustomDrawerText"_tr + formatString("%016lX", edz::cheat::CheatManager::getTitleID())).c_str(), false, x, y + 15, 15, renderer->a(tsl::style::color::ColorDescription));
                    renderer->drawString(("BuildIDMainCustomDrawerText"_tr + formatString("%016lX", edz::cheat::CheatManager::getBuildID())).c_str(), false, x, y + 35, 15, renderer->a(tsl::style::color::ColorDescription));
                    renderer->drawString(("ProcessIDMainCustomDrawerText"_tr + formatString("%016lX", edz::cheat::CheatManager::getProcessID())).c_str(), false, x, y + 55, 15, renderer->a(tsl::style::color::ColorDescription));
                    renderer->drawString("\uE150", false, 170, 310, 90, renderer->a(0xFFFF));
                    renderer->drawString("NoCheatsLoadedErrorCheatsCustomDrawerText"_tr.c_str(), false, 110, 390, 25, renderer->a(0xFFFF));
                });
                rootFrame->setContent(warning);
            } else {
                auto list = new tsl::elm::List();
                std::string head = "SectionNameCategoryHeaderText"_tr + this->m_section;
                if(this->m_section.compare("") != 0) list->addItem(new tsl::elm::CategoryHeader(head));
                else {
                    list->addItem(new tsl::elm::CategoryHeader("AvailableCheatsCategoryHeaderText"_tr, true));
                    list->addItem(new tsl::elm::CustomDrawer([](tsl::gfx::Renderer *renderer, s32 x, s32 y, s32 w, s32 h) {
                        renderer->drawString(("AppIDMainCustomDrawerText"_tr + formatString("%016lX", edz::cheat::CheatManager::getTitleID())).c_str(), false, x + 5, y + 20, 15, renderer->a(tsl::style::color::ColorDescription));
                        renderer->drawString(("BuildIDMainCustomDrawerText"_tr + formatString("%016lX", edz::cheat::CheatManager::getBuildID())).c_str(), false, x + 5, y + 40, 15, renderer->a(tsl::style::color::ColorDescription));
                        renderer->drawString(("ProcessIDMainCustomDrawerText"_tr + formatString("%016lX", edz::cheat::CheatManager::getProcessID())).c_str(), false, x + 5, y + 60, 15, renderer->a(tsl::style::color::ColorDescription));
                    }), 70);
                }

                bool skip = false, inSection = false, submenus = true;;
                std::string skipUntil = "";

                for (auto &cheat : edz::cheat::CheatManager::getCheats()) {
                    if(cheat->getID() == 1 && cheat->getName().find("--DisableSubmenus--") != std::string::npos)
                        submenus = false;

                    if(submenus){
                        // Find section start and end
                        if(this->m_section.length() > 0 && !inSection && cheat->getName().find("--SectionStart:" + this->m_section + "--") == std::string::npos) continue;
                        else if(cheat->getName().find("--SectionStart:" + this->m_section + "--") != std::string::npos) { inSection = true; continue; }
                        else if(inSection && cheat->getName().find("--SectionEnd:" + this->m_section + "--") != std::string::npos) break;

                        // new section
                        if(!skip && cheat->getName().find("--SectionStart:") != std::string::npos){

                            //remove formatting
                            std::string name = cheat->getName();
                            replaceAll(name, "--", "");
                            replaceAll(name, "SectionStart:", "");

                            //create submenu button
                            auto cheatsSubmenu = new tsl::elm::ListItem(name);
                            cheatsSubmenu->setClickListener([name = name](s64 keys) {
                                if (keys & HidNpadButton_A) {
                                    tsl::changeTo<GuiMain>(name);
                                    return true;
                                }
                                return false;
                            });
                            list->addItem(cheatsSubmenu);
                            this->m_numCheats++;

                            //skip over items in section
                            skip = true;
                            skipUntil = "--SectionEnd:" + name + "--";
                        }
                        // found end of child section
                        else if (skip && cheat->getName().compare(skipUntil) == 0){
                            skip = false;
                            skipUntil = "";
                        }
                        // items to add to section
                        else if(!skip && (inSection || this->m_section.length() < 1)) {
                            std::string cheatNameCheck = cheat->getName();
                            replaceAll(cheatNameCheck, ":ENABLED", "");

                            auto cheatToggleItem = new tsl::elm::ToggleListItem(/*formatString("%d:%s: %s", cheat->getID(), (cheat->isEnabled() ? "y" : "n"),*/ cheatNameCheck/*.c_str()).c_str()*/, cheat->isEnabled());
                            cheatToggleItem->setStateChangedListener([&cheat](bool state) { cheat->setState(state); });

                            this->m_cheatToggleItems.insert({cheat->getID(), cheatToggleItem});
                            list->addItem(cheatToggleItem);
                            this->m_numCheats++;
                        }
                    } else {
                        if(cheat->getName().find("--SectionStart:") != std::string::npos || cheat->getName().find("--SectionEnd:") != std::string::npos || cheat->getName().find("--DisableSubmenus--") != std::string::npos)
                            continue;

                        std::string cheatNameCheck = cheat->getName();
                        replaceAll(cheatNameCheck, ":ENABLED", "");

                        auto cheatToggleItem = new tsl::elm::ToggleListItem(cheatNameCheck, cheat->isEnabled());
                        cheatToggleItem->setStateChangedListener([&cheat](bool state) { cheat->setState(state); });

                        this->m_cheatToggleItems.insert({cheat->getID(), cheatToggleItem});
                        list->addItem(cheatToggleItem);
                        this->m_numCheats++;
                    }
                }

                // display if no cheats in submenu
                if(this->m_numCheats < 1) {
                    auto warning = new tsl::elm::CustomDrawer([](tsl::gfx::Renderer *renderer, u16 x, u16 y, u16 w, u16 h){
                        renderer->drawString("\uE150", false, 180, 250, 90, renderer->a(0xFFFF));
                        renderer->drawString("NoCheatsInSubmenuCheatsCustomDrawerText"_tr.c_str(), false, 110, 340, 25, renderer->a(0xFFFF));
                    });
                    rootFrame->setContent(warning);
                } else rootFrame->setContent(list);
            }
        }
        return rootFrame;
    }

    void replaceAll(std::string& str, const std::string& from, const std::string& to) {
        if(from.empty())
            return;
        size_t start_pos = 0;
        while((start_pos = str.find(from, start_pos)) != std::string::npos) {
            str.replace(start_pos, from.length(), to);
            start_pos += to.length();
        }
    }

    virtual void update() override {
        for (auto const& [cheatId, toggleElem] : this->m_cheatToggleItems)
            for(auto &cheat : edz::cheat::CheatManager::getCheats())
                if(cheat->getID() == cheatId)
                    toggleElem->setState(cheat->isEnabled());
    }

private:
    int m_numCheats = 0;
    std::string m_section;
    std::map<u32, tsl::elm::ToggleListItem*> m_cheatToggleItems;
};

class EdiZonOverlay : public tsl::Overlay {
public:
    EdiZonOverlay() { }
    ~EdiZonOverlay() { }

    void initServices() override {
        std::string jsonStr = R"(
            {
                "PluginName": "CheatsManager",
                "NoCheatsLoadedErrorCheatsCustomDrawerText": "No Cheats loaded!",
                "SectionNameCategoryHeaderText": "Section: ",
                "AvailableCheatsCategoryHeaderText": "Available cheats",
                "NoCheatsInSubmenuCheatsCustomDrawerText": "No Cheats in Submenu!",
                "AppIDMainCustomDrawerText": "Program ID:",
                "BuildIDMainCustomDrawerText": "Build ID:",
                "ProcessIDMainCustomDrawerText": "Process ID:"
            }
        )";
        std::string lanPath = std::string("sdmc:/switch/.overlays/lang/") + APPTITLE + "/";
        fsdevMountSdmc();
        tsl::hlp::doWithSmSession([&lanPath, &jsonStr]{
            tsl::tr::InitTrans(lanPath, jsonStr);
        });
        fsdevUnmountDevice("sdmc");

        // GDB Check & Saved Cheat Enabling
        if(edz::cheat::CheatManager::isCheatServiceAvailable()){
            edz::cheat::CheatManager::initialize();
            for (auto &cheat : edz::cheat::CheatManager::getCheats()) {
                if(cheat->getName().find(":ENABLED") != std::string::npos){
                    cheat->setState(true);
                }
            }
        }
    }

    virtual void exitServices() override {
        if(edz::cheat::CheatManager::isCheatServiceAvailable())
            edz::cheat::CheatManager::exit();
    }

    std::unique_ptr<tsl::Gui> loadInitialGui() override {
        return initially<GuiMain>("");
    }
};

int main(int argc, char **argv) {
    return tsl::loop<EdiZonOverlay>(argc, argv);
}