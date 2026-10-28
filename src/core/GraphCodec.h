#ifndef SMARTEXAM_GRAPH_CODEC_H
#define SMARTEXAM_GRAPH_CODEC_H
#include "Types.h"
#include <sstream>
#include <iomanip>
#include <cmath>
#include <stdexcept>
#include <locale>
namespace exam {
inline std::string encodeGraph(const GraphAnswer& graph) {
    if(graph.points.size()>1000 || graph.coefficients.size()>3) throw std::invalid_argument("Слишком много графических данных");
    std::ostringstream text; text.imbue(std::locale::classic()); text << std::setprecision(17);
    text << graph.points.size();
    for(const Point& point:graph.points) {
        if(!std::isfinite(point.x) || !std::isfinite(point.y)) throw std::invalid_argument("Неверные координаты");
        text << ' ' << point.x << ' ' << point.y;
    }
    text << ' ' << graph.coefficients.size();
    for(double value:graph.coefficients) {
        if(!std::isfinite(value)) throw std::invalid_argument("Неверный коэффициент");
        text << ' ' << value;
    }
    return text.str();
}
inline GraphAnswer decodeGraph(const std::string& text) {
    GraphAnswer graph; if(text.empty()) return graph;
    std::istringstream input(text); input.imbue(std::locale::classic());
    std::size_t count;
    if(!(input>>count) || count>1000) throw std::runtime_error("Повреждён графический ответ");
    for(std::size_t i=0;i<count;++i) { Point p; if(!(input>>p.x>>p.y)) throw std::runtime_error("Повреждены координаты"); graph.points.push_back(p); }
    if(!(input>>count) || count>3) throw std::runtime_error("Повреждены коэффициенты");
    for(std::size_t i=0;i<count;++i) { double value; if(!(input>>value)) throw std::runtime_error("Повреждён коэффициент"); graph.coefficients.push_back(value); }
    input>>std::ws; if(!input.eof()) throw std::runtime_error("Лишние графические данные");
    encodeGraph(graph);
    return graph;
}
}
#endif
