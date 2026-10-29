#include "GraphView.h"
#include <cmath>
#include <algorithm>
#include <sstream>
namespace exam {
namespace ui {
GraphView::GraphView(const sf::Font &font, float width, float height)
    : Widget(font, width, height), coordinates(width, height), kind(PartKind::Line), snapping(true),
      panning(false), dragged(-1), snapStep(0.5) {}
void GraphView::setAnswer(PartKind type, const GraphAnswer &value) {
    kind = type;
    answer = value;
    dragged = -1;
}
bool GraphView::handle(const sf::Event &event, sf::Vector2f mouse) {
    sf::Vector2f local = getInverseTransform().transformPoint(mouse);
    bool inside = contains(mouse);
    if (event.type == sf::Event::MouseWheelScrolled && inside) {
        coordinates.zoom(std::pow(1.15, event.mouseWheelScroll.delta), Point(local.x, local.y));
        return true;
    }
    if (event.type == sf::Event::MouseButtonPressed && inside) {
        lastMouse = local;
        if (event.mouseButton.button == sf::Mouse::Right ||
            event.mouseButton.button == sf::Mouse::Middle) {
            panning = true;
            return true;
        }
        if (event.mouseButton.button != sf::Mouse::Left || kind != PartKind::Line)
            return false;
        for (std::size_t i = 0; i < answer.points.size(); ++i) {
            Point at = coordinates.toScreen(answer.points[i]);
            if (std::hypot(at.x - local.x, at.y - local.y) < 15)
                dragged = static_cast<int>(i);
        }
        if (dragged < 0 && answer.points.size() < 2) {
            answer.points.push_back(Point());
            dragged = static_cast<int>(answer.points.size() - 1);
        }
    }
    if ((event.type == sf::Event::MouseMoved || event.type == sf::Event::MouseButtonPressed) &&
        dragged >= 0) {
        Point world = coordinates.toWorld(Point(local.x, local.y));
        if (snapping) {
            world.x = Coordinates::snap(world.x, snapStep);
            world.y = Coordinates::snap(world.y, snapStep);
        }
        world.x = std::max(-10000., std::min(10000., world.x));
        world.y = std::max(-10000., std::min(10000., world.y));
        answer.points[dragged] = world;
        answer.coefficients.clear();
        if (onChange)
            onChange();
        return true;
    }
    if (event.type == sf::Event::MouseMoved && panning) {
        coordinates.pan(local.x - lastMouse.x, local.y - lastMouse.y);
        lastMouse = local;
        return true;
    }
    if (event.type == sf::Event::MouseButtonReleased) {
        dragged = -1;
        panning = false;
    }
    return false;
}
void GraphView::draw(sf::RenderTarget &target, sf::RenderStates states) const {
    if (canvas.getSize() !=
        sf::Vector2u(static_cast<unsigned>(size.x), static_cast<unsigned>(size.y)))
        canvas.create(static_cast<unsigned>(size.x), static_cast<unsigned>(size.y));
    canvas.clear(Theme::paper);
    auto line = [this](float x1, float y1, float x2, float y2, sf::Color color) {
        sf::Vertex points[] = {sf::Vertex(sf::Vector2f(x1, y1), color),
                               sf::Vertex(sf::Vector2f(x2, y2), color)};
        canvas.draw(points, 2, sf::Lines);
    };
    double step = std::pow(10., std::floor(std::log10(50 / coordinates.scale)));
    if (step * coordinates.scale < 25)
        step *= 5;
    Point low = coordinates.toWorld(Point(0, size.y)), high = coordinates.toWorld(Point(size.x, 0));
    for (double x = std::ceil(low.x / step) * step; x <= high.x; x += step) {
        Point p = coordinates.toScreen(Point(x, 0));
        line(p.x, 0, p.x, size.y, Theme::line);
        std::ostringstream value;
        value << x;
        canvas.draw(label(font, unicode(value.str()), 12, p.x + 2,
                          std::max(2.f, std::min(size.y - 18, static_cast<float>(p.y + 2))),
                          Theme::muted));
    }
    for (double y = std::ceil(low.y / step) * step; y <= high.y; y += step) {
        Point p = coordinates.toScreen(Point(0, y));
        line(0, p.y, size.x, p.y, Theme::line);
        std::ostringstream value;
        value << y;
        canvas.draw(label(font, unicode(value.str()), 12,
                          std::max(2.f, std::min(size.x - 40, static_cast<float>(p.x + 3))),
                          p.y + 2, Theme::muted));
    }
    Point origin = coordinates.toScreen(Point());
    line(origin.x, 0, origin.x, size.y, Theme::muted);
    line(0, origin.y, size.x, origin.y, Theme::muted);
    std::vector<double> coefficients = answer.coefficients;
    if (kind == PartKind::Line && answer.points.size() == 2) {
        try {
            coefficients = GeometryChecker::lineCoefficients(answer.points);
        } catch (...) {
            coefficients.clear();
        }
    }
    if ((kind == PartKind::Line && coefficients.size() == 2) ||
        (kind == PartKind::Parabola && coefficients.size() == 3)) {
        sf::VertexArray curve(sf::LineStrip);
        for (int pixel = 0; pixel <= static_cast<int>(size.x); pixel += 2) {
            double x = coordinates.toWorld(Point(pixel, 0)).x;
            double y = kind == PartKind::Line
                           ? coefficients[0] * x + coefficients[1]
                           : (coefficients[0] * x + coefficients[1]) * x + coefficients[2];
            Point screen = coordinates.toScreen(Point(x, y));
            curve.append(sf::Vertex(
                sf::Vector2f(pixel,
                             static_cast<float>(std::max(-100000., std::min(100000., screen.y)))),
                Theme::accent));
        }
        canvas.draw(curve);
    }
    for (const auto &point : answer.points) {
        Point at = coordinates.toScreen(point);
        sf::CircleShape dot(6);
        dot.setFillColor(Theme::accent);
        dot.setPosition(at.x - 6, at.y - 6);
        canvas.draw(dot);
    }
    canvas.display();
    states.transform *= getTransform();
    target.draw(sf::Sprite(canvas.getTexture()), states);
}
} // namespace ui
} // namespace exam
