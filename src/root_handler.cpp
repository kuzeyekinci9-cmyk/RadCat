#include "RootHandler.hpp"

#include "TH1F.h"
#include "TH2F.h"
#include "TGraph.h"
#include "TCanvas.h"
#include "TAxis.h"
#include "TFile.h"


RootHandler::RootHandler() {
}

RootHandler::~RootHandler() {
    //Delete data objects and canvases when handler is destroyed
    for (auto& [name, h] : m_hists1D) if (h) delete h;
    for (auto& [name, h] : m_hists2D) if (h) delete h;
    for (auto& [name, g] : m_graphs)  if (g) delete g;
    for (auto& [name, c] : m_canvases) if (c) delete c;

    // Clear maps
    m_hists1D.clear();
    m_hists2D.clear();
    m_graphs.clear();
    m_canvases.clear();
}

    
bool RootHandler::checkExists(const std::string &name)
{
    //Check if a histogram or graph with the given name already exists in any of the maps 
    if (m_hists1D.count(name) || m_hists2D.count(name) || m_graphs.count(name))
    {
        std::cerr << "Warning: Plot '" << name << "' already exists." << std::endl;
        return true;
    }
    return false;
}


// =============================================================
// CREATION METHODS
// =============================================================
void RootHandler::createHist1D(const std::string &name, const std::string &title, int bins, double min, double max)
{

    if (checkExists(name)) return;
    // Create and store the histogram
    TH1F *hist = new TH1F(name.c_str(), title.c_str(), bins, min, max);
    m_hists1D[name] = hist;
    // Initialize default draw options
    m_drawOptions[name] = DrawOptions();
}

void RootHandler::createHist2D(const std::string &name, const std::string &title, int xbins, double xmin, double xmax, int ybins, double ymin, double ymax)
{
    if (checkExists(name)) return;
    // Create and store the histogram
    TH2F *hist = new TH2F(name.c_str(), title.c_str(), xbins, xmin, xmax, ybins, ymin, ymax);
    m_hists2D[name] = hist;
    // Initialize default draw options
    m_drawOptions[name] = DrawOptions();
}

void RootHandler::createGraph(const std::string &name, const std::string &title)
{
    if (checkExists(name)) return;
    // Create and store the graph
    TGraph *g = new TGraph();
    g->SetTitle(title.c_str());
    g->SetName(name.c_str());
    m_graphs[name] = g;
    // Initialize default draw options
    m_drawOptions[name] = DrawOptions();
}

// =============================================================
// DATA FILLING METHODS
// =============================================================
void RootHandler::fillHist1D(const std::string &name, double value)
{
    auto i = m_hists1D.find(name);
    if (i != m_hists1D.end())
    {
        i->second->Fill(value);
    }
}

void RootHandler::fillHist2D(const std::string &name, double x, double y)
{
    auto i = m_hists2D.find(name);
    if (i != m_hists2D.end())
    {
        i->second->Fill(x, y);
    }
}

void RootHandler::addPointToGraph(const std::string &name, double x, double y)
{
    auto i = m_graphs.find(name);
    if (i != m_graphs.end())
    {
        TGraph *g = i->second;
        g->SetPoint(g->GetN(), x, y);
    }
}


DrawOptions& RootHandler::getDrawOptions(const std::string &name)
{// Return the reference to the options struct. 
    // Create a new one if it doesn't exist.
    return m_drawOptions[name];
}

void RootHandler::setLogY(const std::string& name, bool enable) { getDrawOptions(name).logY = enable; }
void RootHandler::setLogX(const std::string& name, bool enable) { getDrawOptions(name).logX = enable; }
void RootHandler::setLogZ(const std::string& name, bool enable) { getDrawOptions(name).logZ = enable; }
void RootHandler::setRebin(const std::string& name, int factor) { getDrawOptions(name).rebin = factor; }
void RootHandler::setNormalize(const std::string& name, bool enable) { getDrawOptions(name).normalize = enable; }
void RootHandler::setTitle(const std::string& name, const std::string& title) { getDrawOptions(name).title = title; }



void RootHandler::draw(const std::string &name)
{
    //Check if canvas already exists, otherwise create it
    TCanvas *c = nullptr;
    if (m_canvases.count(name)){
        c=m_canvases[name];

    } else {
        c = new TCanvas(("c_" + name).c_str(), name.c_str(), 800, 600);
        m_canvases [name] = c;

    }
    
    c->cd();       //Set canvas as active
    c->Clear(); //Clear previous drawings

    DrawOptions &options = getDrawOptions(name);

    if (m_hists1D.count(name))
    {
        TH1 *hOriginal = m_hists1D[name];
        
        TH1 *hDraw = (TH1*)hOriginal->Clone((name + "_draw").c_str());
        if (!options.title.empty()) hDraw->SetTitle(options.title.c_str());

        if (options.rebin > 1){
            hDraw->Rebin(options.rebin);
        }
        if (options.normalize) {
            if (hDraw->Integral() != 0)
                hDraw->Scale(1.0 / hDraw->Integral());
        }
           
        if (options.logY)
            c->SetLogy();
            
        hDraw->Draw();
    }
    else if (m_hists2D.count(name))
    {
        TH2 *hOriginal = m_hists2D[name];
        TH2 *hDraw = (TH2*)hOriginal->Clone((name + "_draw").c_str());
        if (!options.title.empty()) hDraw->SetTitle(options.title.c_str());
        if (options.rebin > 1) hDraw->Rebin2D(options.rebin, options.rebin);
        if (options.logX) c->SetLogx();
        if (options.logY) c->SetLogy();
        if (options.logZ) c->SetLogz();
        
        hDraw->Draw("COLZ"); // Draw with color map and z axis
    }
    else if (m_graphs.count(name))
    {
        TGraph *g = m_graphs[name];
        if (!options.title.empty()) g->SetTitle(options.title.c_str());
        
        g->Draw("APL"); // A:Axis, P:Points, L:Line
    }
    else
    {
        std::cerr << "Error: No plot found with name " << name << std::endl;
        delete c;
        return;
    }

    c->Update();
}

void RootHandler::saveToRootFile(const std::string &filename)
{
    TFile *f = new TFile(filename.c_str(), "RECREATE");
    // "RECREATE": Creates a new file, overwriting if exists.

    if (!f || f->IsZombie()) {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return;
    }
    // Create a folder inside the ROOT file
    if (!m_hists1D.empty()) {
        f->mkdir("Histograms1D")->cd(); // Create folder and enter it
        for (auto const& [name, h] : m_hists1D) h->Write();
        f->cd(); // Go back to root directory
    }
    if (!m_hists2D.empty()) {
        f->mkdir("Histograms2D")->cd();
        for (auto const& [name, h] : m_hists2D) h->Write();
        f->cd();
    }
     if (!m_graphs.empty()) {
        f->mkdir("Graphs")->cd();
        for (auto const& [name, g] : m_graphs)  g->Write();
        f->cd();
    }
    f->Close();
    delete f; 
    std::cout << "Save complete. File closed." << std::endl;
}

void RootHandler::resetAll() {
    //Reset all histograms and graphs to empty state
    for (auto& [name, h] : m_hists1D) h->Reset(); 
    for (auto& [name, h] : m_hists2D) h->Reset();
    
    for (auto& [name, g] : m_graphs) {
        g->Set(0); 
    }
    
    for (auto& [name, c] : m_canvases) {
        c->Clear();
        c->Update();
    }
    std::cout << "All plots have been reset." << std::endl;
}