#include <root_handler.hpp>
#include <stdexcept>


PlotHandle RootHandler::createHistogram1D(const std::string& name, int bins, double min, double max); {

    
    return PlotHandle(PlotHandle::Type::HIST1D, ptr);
}