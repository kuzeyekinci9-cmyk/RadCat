#pragma once
#include <string>
#include <map>
#include <iostream>
#include "RootComponent.hpp"
#include "TH1F.h"
#include "TH2F.h"
#include "TGraph.h"
#include "TCanvas.h"
#include "TAxis.h"
#include "TFile.h"

class TCanvas;

class RootHandler {
public: 
    static RootHandler& Instance(){ static RootHandler instance; return instance; }

    
    template<typename PlotType>
    void CreateEmptyDrawable(RootComponent& caller, RootObject<PlotType>& object){
        caller.rootDir->cd();

    }

    // Creation
    void createHist1D(const std::string& name, const std::string& title, int bins, double min, double max);
    void createHist2D(const std::string& name, const std::string& title, int xbins, double xmin, double xmax, int ybins, double ymin, double ymax);
    void createGraph(const std::string& name, const std::string& title);
    // Filling
    void fillHist1D(const std::string& name, double value);
    void fillHist2D(const std::string& name, double x, double y);
    void addPointToGraph(const std::string& name, double x, double y); 
    // Drawing options
    void setRebin(const std::string& name, int factor);
    void setNormalize(const std::string& name, bool enable);
    void setTitle(const std::string& name, const std::string& title);
    // Drawing
    void draw(const std::string& name);
    void saveToRootFile(const std::string& filename); 
    void resetAll();



private:
RootHandler() = default;
~RootHandler() = default;
RootHandler& operator=(const RootHandler&) = delete;

};
