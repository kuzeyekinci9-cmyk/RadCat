#include "RootHandler.hpp"

#include "TH1F.h"
#include "TH2F.h"
#include "TGraph.h"
#include "TCanvas.h"
#include "TStyle.h"
#include "TObject.h"


// Constructor:
RootHandler::RootHandler() {}
RootHandler::~RootHandler() {}

DrawableObject<TH1F> RootHandler::createHist1D(TDirectory* targetDir, const std::string& name, const std::string& title, int bins, double min, double max) {
    TDirectory* mainDir = gDirectory; // Save current directory
    targetDir->cd(); // Change to target directory
    TH1F* hist = new TH1F(name.c_str(), title.c_str(), bins, min, max);
    mainDir->cd(); // Restore original directory
    return DrawableObject<TH1F>(hist);
}

DrawableObject<TH2F> RootHandler::createHist2D(TDirectory* targetDir, const std::string& name, const std::string& title, int xbins, double xmin, double xmax, int ybins, double ymin, double ymax) {
    TDirectory* mainDir = gDirectory; // Save current directory
    targetDir->cd(); // Change to target directory
    TH2F* hist = new TH2F(name.c_str(), title.c_str(), xbins, xmin, xmax, ybins, ymin, ymax);
    mainDir->cd(); // Restore original directory
    return DrawableObject<TH2F>(hist);
}

DrawableObject<TGraph> RootHandler::createGraph(TDirectory* targetDir, const std::string& name, const std::string& title) {
    TDirectory* mainDir = gDirectory; // Save current directory
    targetDir->cd(); // Change to target directory
    TGraph* graph = new TGraph();
    graph->SetName(name.c_str());
    graph->SetTitle(title.c_str());
    mainDir->cd(); // Restore original directory
    return DrawableObject<TGraph>(graph);
}

TCanvas* RootHandler::createCanvas(const std::string& name, const std::string& title, int width, int height) {
    TCanvas* canvas = new TCanvas(name.c_str(), title.c_str(), width, height);
    return canvas;
}


