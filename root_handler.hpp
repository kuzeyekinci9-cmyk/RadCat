
#include <map>
#include <memory>
#include <string>



class PlotHandle {
public:
    enum class Type { HIST1D, HIST2D, GRAPH };
    

    PlotHandle() {
        type = Type::HIST1D;
        impl = nullptr;
    }
    PlotHandle(Type t, std::shared_ptr<void> p) {
        type = t;
        impl = p;
    }

    Type type;
    std::shared_ptr<void> impl;
};

struct DrawOptions {
    bool normalized = false;
    bool logY = false;
    int rebin = 1;
    std::string title;
    
};


class RootHandler {
public:
    RootHandler();
    ~RootHandler();

    template <typename T>
    PlotHandle createHistogram1D(const std::string& name, T data[], int bins, double min, double max);

    PlotHandle createHistogram2D(const std::string& name, int xbins, double xmin, double xmax ,int ybins, double ymin, double ymax);
    PlotHandle createGraph(const std::string& name);

    template <typename T>
    T normalize(T graph) {
        if T == PlotHandle::Type::HIST1D {
            // Normalization logic for 1D histogram

        } else if T == PlotHandle::Type::HIST2D {dfgkjdfkjdfkjdfkj
            // Normalization logic for 2D histogram

        }

        return graph;
    }
private:
    std::map<std::string, PlotHandle> plots_; 
};
