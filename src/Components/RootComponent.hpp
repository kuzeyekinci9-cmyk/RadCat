#pragma once
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

template<PlotType T>
using PlotTypeToROOT = std::tuple_element_t<static_cast<size_t>(T), RootTypes>;

// Restrict types to ROOT plotting objects so that DrawableObject can only be instantiated with these types.
template<typename T> concept RootPlots = std::same_as<std::remove_cvref_t<T>, TH1F>
    || std::same_as<std::remove_cvref_t<T>, TH2F>
    || std::same_as<std::remove_cvref_t<T>, TGraph>;

// Wrapper struct to hold a ROOT object along with its drawing options.
// If you are programming devices that use ROOT plotting objects, use this struct to manage the data.
template <RootPlots T> struct DrawableObject {

    // ========================================================================
    // DRAW OPTIONS
    // Struct that holds drawing options for the ROOT object.
    // If you are programming devices that use ROOT plotting objects, use this struct to manage drawing options.
    //
    // Each member has comment explaining its purpose so you can inspect.
    struct DrawOptions {
        //
        bool normalized = false;
        //
        bool logY = false;
        //
        bool logX = false;
        //
        bool logZ = false;
        //
        int rebin = 1;
        //
        std::string title;
    };
    // Read struct for clarity.
    DrawOptions options; 

    // Drawable ROOT object
    std::unique_ptr<T> rootObj = nullptr;

    //
    DrawableObject<T>& setLogY(bool enable = true) { options.logY = enable; return *this; }
    //
    DrawableObject<T>& setLogX(bool enable = true) { options.logX = enable; return *this; }
    //
    DrawableObject<T>& setLogZ(bool enable = true) { options.logZ = enable; return *this; }
    //
    DrawableObject<T>& setRebin(int factor) { options.rebin = factor; return *this; }
    //
    DrawableObject<T>& setNormalize(bool enable = true) { options.normalized = enable; return *this; }
    //
    DrawableObject<T>& setTitle(const std::string& t) { options.title = t; return *this; }

    // Clone the drawable object with its options
    DrawableObject<T> clone() const {
        if (!isValid()){
            Debug.Error("DrawableObject::clone -> Attempting to clone an invalid DrawableObject!");
            return DrawableObject<T>();
        }
        DrawableObject<T> newObj;
        // ROOT's Clone returns a TObject, so we need to cast it back to T
        T* clone = dynamic_cast<T*>(rootObj->Clone());
        newObj.rootObj = std::unique_ptr<T>(clone);
        // Copy the drawing options
        newObj.options = this->options;
        return newObj;
    }

    bool isValid() const { return rootObj != nullptr; }
};

class RootComponent : public BaseComponent {
public:
template<typename DeviceType> RootComponent(DeviceType& parentDevice) : BaseComponent(&parentDevice) { initialize(); }
virtual void update() override;
virtual void initialize() override;

std::vector<std::variant<DrawableObject<TH1F>, DrawableObject<TH2F>, DrawableObject<TGraph>>> RootObjects;

template<typename Type>
DrawableObject<Type> createPlot(const std::string& name, PlotType type){
    using Type = typename PlotTypeToROOT<type>::type;
    DrawableObject<Type> newObj;
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