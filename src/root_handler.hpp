#ifndef ROOTHANDLER_HPP
#define ROOTHANDLER_HPP

#include <string>
#include <map>
#include <iostream>

class TH1;
class TH2;
class TGraph;
class TCanvas;

struct DrawOptions {
    bool normalized = false;
    bool logY = false;
    bool logX = false;
    bool logZ = false;
    int rebin = 1;
    std::string title;
};

class RootHandler {
public:
    RootHandler();
    ~RootHandler();
        // Creation 
    void createHist1D(const std::string& name, const std::string& title, int bins, double min, double max);
    void createHist2D(const std::string& name, const std::string& title, int xbins, double xmin, double xmax, int ybins, double ymin, double ymax);
    void createGraph(const std::string& name, const std::string& title);
        // Filling
    void fillHist1D(const std::string& name, double value);
    void fillHist2D(const std::string& name, double x, double y);
    void addPointToGraph(const std::string& name, double x, double y); 
        // Drawing options
    void setLogY(const std::string& name, bool enable);
    void setLogX(const std::string& name, bool enable);
    void setLogZ(const std::string& name, bool enable); 
    void setRebin(const std::string& name, int factor);
    void setNormalize(const std::string& name, bool enable);
    void setTitle(const std::string& name, const std::string& title);
        // Drawing
    void draw(const std::string& name);
    
    void saveToRootFile(const std::string& filename); 
    void resetAll(); 

private:
    std::map<std::string, TH1*> m_hists1D;  // Map of 1D histograms 
    std::map<std::string, TH2*> m_hists2D; 
    std::map<std::string, TGraph*> m_graphs;
    std::map<std::string, TCanvas*> m_canvases;
    std::map<std::string, DrawOptions> m_drawOptions;

    bool checkExists(const std::string& name);
    
   
    DrawOptions& getDrawOptions(const std::string &name);
};

#endif 