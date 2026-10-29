#ifndef SMARTEXAM_GRAPH_VIEW_H
#define SMARTEXAM_GRAPH_VIEW_H
#include "ui/widgets/Widget.h"
#include "graph_tasks/GeometryChecker.h"
namespace exam {
namespace ui {
class GraphView : public Widget {
    Coordinates coordinates;
    GraphAnswer answer;
    PartKind kind;
    bool snapping, panning;
    int dragged;
    double snapStep;
    sf::Vector2f lastMouse;
    mutable sf::RenderTexture canvas;
    void draw(sf::RenderTarget &target, sf::RenderStates states) const override;

  public:
    std::function<void()> onChange;
    GraphView(const sf::Font &font, float width, float height);
    void setAnswer(PartKind type, const GraphAnswer &value);
    GraphAnswer getAnswer() const {
        return answer;
    }
    void setSnap(bool enabled) {
        snapping = enabled;
    }
    void setSnapStep(double value) {
        snapStep = value;
    }
    void resetView() {
        coordinates = Coordinates(size.x, size.y);
    }
    bool handle(const sf::Event &event, sf::Vector2f mouse) override;
};
} // namespace ui
} // namespace exam
#endif
