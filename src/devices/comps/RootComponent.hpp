#include "componentCore.hpp"
#include "CompHandlers/RootHandler.hpp"
#include <string>
#include <vector>
#include <variant>
#include <tuple>
#include <map>
#include "TH1F.h"
#include "TH2F.h"
#include "TGraph.h"

class TDirectory;
class TFile;

enum class PlotType { HIST1D, HIST2D, GRAPH };
using RootTypes = std::tuple<TH1F, TH2F, TGraph>;
using RootObjectVariants = std::variant<RootObject<TH1F>, RootObject<TH2F>, RootObject<TGraph>>;

template<PlotType T>
using PlotTypeToROOT = std::tuple_element_t<static_cast<size_t>(T), RootTypes>;

template <typename TypeOfDrawableObject>
struct RootObject {
    TypeOfDrawableObject* plotObject = nullptr;
    std::string name;
    bool normalized = false;
    bool logY = false;
    bool logX = false;
    bool logZ = false;
    int rebin = 1;  
    std::string title;

    //RootObject<TypeOfDrawableObject>& setLogY(bool enable){ logY = enable; return *this; }
    //RootObject<TypeOfDrawableObject>& setLogX(bool enable){ logX = enable; return *this; }
    //RootObject<TypeOfDrawableObject>& setLogZ(bool enable){ logZ = enable; return *this; }
    //auto& setLogY(bool enable){ logY = enable; return *this; }
    //auto& setLogX(bool enable){ logX = enable; return *this; }
    //auto& setLogZ(bool enable){ logZ = enable; return *this; }

};

class RootComponent : public BaseComponent {
public:
template<typename DeviceType> RootComponent(DeviceType& parentDevice) : BaseComponent(&parentDevice) { initialize(); }
virtual void update() override;
virtual void initialize() override;

std::vector<RootObjectVariants> RootObjects;

template<typename Type>
RootObject<Type> createPlot(const std::string& name, PlotType type){
    using Type = typename PlotTypeToROOT<type>::type;
    RootObject<Type> newObj;
    newObj.name = name;
    RootObjects.push_back(newObj);
    rootHandler.CreateEmptyDrawable(*this, newObj);
    return newObj;
}

private:
RootHandler& rootHandler = RootHandler::Instance();
friend class RootHandler;
TDirectory* rootDir = nullptr;
TFile* rootFile = nullptr;
std::string dirName;

};