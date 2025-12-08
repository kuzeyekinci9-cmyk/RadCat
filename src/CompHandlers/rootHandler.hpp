#pragma once

// ============================================================================
// INCLUDES
// ============================================================================

#include <string>
#include <memory>
#include <iostream>
#include <type_traits> 
#include <concepts>
#include <type_traits>

#include "TH1.h"
#include "TH2.h"
#include "TGraph.h"
#include "TCanvas.h"
#include "TDirectory.h"
#include "TROOT.h"
#include "Debug.hpp"

const char APL[] = "APL";
const char COLZ[] = "COLZ";

// ============================================================================
// STRUCTURES
// ============================================================================

struct DrawOptions {
    bool normalized = false;
    bool logY = false;
    bool logX = false;
    bool logZ = false;
    int rebin = 1;
    std::string title;
};

// ============================================================================
// TEMPLATE DEFINITIONS
// ============================================================================

// Restrict types to ROOT plotting objects
template<typename T>
concept RootPlots = std::same_as<std::remove_cvref_t<T>, TH1F>
    || std::same_as<std::remove_cvref_t<T>, TH2F>
    || std::same_as<std::remove_cvref_t<T>, TGraph>;

template <RootPlots T>
struct DrawableObject {
    std::shared_ptr<T> rootObj; // Shared pointer to ROOT object of type T
    DrawOptions options; 

    DrawableObject(T* obj = nullptr) {
        if (obj) rootObj = std::shared_ptr<T>(obj);
    } // Constructor

    DrawableObject<T>& setLogY(bool enable = true) { options.logY = enable; return *this; }
    DrawableObject<T>& setLogX(bool enable = true) { options.logX = enable; return *this; }
    DrawableObject<T>& setLogZ(bool enable = true) { options.logZ = enable; return *this; }
    DrawableObject<T>& setRebin(int factor) { options.rebin = factor; return *this; }
    DrawableObject<T>& setNormalize(bool enable = true) { options.normalized = enable; return *this; }
    DrawableObject<T>& setTitle(const std::string& t) { options.title = t; return *this; }

    // Clone the drawable object with its options
    DrawableObject<T> clone() const {
        if (!isValid()) return DrawableObject<T>(nullptr);  
        // ROOT's Clone returns a TObject*, so we need to cast it back to T*
        T* clonePtr = static_cast<T*>(rootObj->Clone()); 
        
        // Create a new DrawableObject with the cloned object
        DrawableObject<T> newObj(clonePtr);
        // Copy the drawing options
        newObj.options = this->options;
        return newObj;
    }

    bool isValid() const { return rootObj != nullptr; }
};

class RootHandler {
public:
    // ========================================================================
    // SINGLETON PATTERN
    // ========================================================================
    static RootHandler& Instance(){ static RootHandler instance; return instance; }

    // ========================================================================
    // OBJECT CREATION
    // ========================================================================
    DrawableObject<TH1F> createHist1D(TDirectory* targetDir, const std::string& name, const std::string& title, int bins, double min, double max);
    DrawableObject<TH2F> createHist2D(TDirectory* targetDir, const std::string& name, const std::string& title, int xbins, double xmin, double xmax, int ybins, double ymin, double ymax);
    DrawableObject<TGraph> createGraph(TDirectory* targetDir, const std::string& name, const std::string& title);

    // ========================================================================
    // DATA FILLING
    // ========================================================================
    void fillHist1D(DrawableObject<TH1F>& obj, double value) {
        if (obj.rootObj) obj.rootObj->Fill(value);
    }

    void fillHist2D(DrawableObject<TH2F>& obj, double x, double y) {
        if (obj.rootObj) obj.rootObj->Fill(x, y);
    }

    void addPointToGraph(DrawableObject<TGraph>& obj, double x, double y) {
        if (obj.rootObj) obj.rootObj->SetPoint(obj.rootObj->GetN(), x, y);
    }

    // ========================================================================
    // CANVAS & RENDERING
    // ========================================================================
    TCanvas* createCanvas(const std::string& name, const std::string& title, int width = 800, int height = 600);

    template <typename T>
    void drawOnCanvas(const DrawableObject<T>& obj, TCanvas* targetCanvas) {
        if (!obj.isValid()) {
            Debug.Error("RootHandler::drawOnCanvas -> Invalid DrawableObject passed!");
            return;
        }
        if (!targetCanvas) {
            Debug.Error("RootHandler::drawOnCanvas -> Target Canvas is NULL!");
            return;
        }
        targetCanvas->cd();
        
        DrawableObject<T> drawCopy = obj.clone(); // TODO: Dont copy if no option
        T* rawObj = drawCopy.rootObj.get();
        const DrawOptions& opt = drawCopy.options;

        if (!opt.title.empty()) rawObj->SetTitle(opt.title.c_str());

        targetCanvas->SetLogx(opt.logX ? 1 : 0);
        targetCanvas->SetLogy(opt.logY ? 1 : 0);
        targetCanvas->SetLogz(opt.logZ ? 1 : 0);

        // Draw based on object type 
        if constexpr (std::is_base_of<TH2, T>::value) { 
            if (opt.rebin > 1) rawObj->Rebin2D(opt.rebin, opt.rebin);
            rawObj->Draw(COLZ);
        }
        else if constexpr (std::is_base_of<TH1, T>::value) {
            if (opt.rebin > 1) rawObj->Rebin(opt.rebin);
            
            if (opt.normalized && rawObj->Integral() != 0) {
                rawObj->Scale(1.0 / rawObj->Integral());
            }
            rawObj->Draw();
        }
        else if constexpr (std::is_base_of<TGraph, T>::value) {
            rawObj->Draw(APL);
        }

        targetCanvas->Update();
    }

private:
    // ========================================================================
    // PRIVATE MEMBERS FOR SINGLETON
    // ========================================================================
    RootHandler();
    ~RootHandler();         
    RootHandler(const RootHandler&) = delete;
    RootHandler& operator=(const RootHandler&) = delete;
};
