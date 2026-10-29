#include "SkillMap.h"
#include <algorithm>
#include <cmath>
namespace exam {
namespace ui {
SkillMap::SkillMap(const sf::Font &font, const std::vector<SkillInfo> &skills,
                   const std::map<std::string, std::vector<std::string>> &graph,
                   const StudentSnapshot &state)
    : Widget(font, 600, 430), nodes(skills), edges(graph), snapshot(state), dragged(-1), scroll(0) {
    for (std::size_t i = 0; i < nodes.size(); ++i)
        if (!nodes[i].placed)
            nodes[i].position = Point(15 + (i % 3) * 195, 20 + (i / 3) * 80);
}
bool SkillMap::handle(const sf::Event &event, sf::Vector2f mouse) {
    sf::Vector2f local = getInverseTransform().transformPoint(mouse);
    if (event.type == sf::Event::MouseWheelScrolled && contains(mouse)) {
        float height = size.y;
        for (const auto &node : nodes)
            height = std::max(height, static_cast<float>(node.position.y + 80));
        scroll =
            std::max(0.f, std::min(height - size.y, scroll - event.mouseWheelScroll.delta * 50));
        return true;
    }
    local.y += scroll;
    if (event.type == sf::Event::MouseButtonPressed &&
        event.mouseButton.button == sf::Mouse::Left && contains(mouse)) {
        for (int i = static_cast<int>(nodes.size()) - 1; i >= 0; --i)
            if (sf::FloatRect(nodes[i].position.x, nodes[i].position.y, 170, 58).contains(local)) {
                dragged = i;
                offset = sf::Vector2f(local.x - nodes[i].position.x, local.y - nodes[i].position.y);
                return true;
            }
    }
    if (event.type == sf::Event::MouseMoved && dragged >= 0) {
        nodes[dragged].position =
            Point(std::max(0.f, std::min(size.x - 170, local.x - offset.x)),
                  std::max(0.f, std::min(size.y + scroll - 58, local.y - offset.y)));
        return true;
    }
    if (event.type == sf::Event::MouseButtonReleased && dragged >= 0) {
        SkillInfo node = nodes[dragged];
        dragged = -1;
        node.placed = true;
        if (onMove)
            onMove(node);
        if (onSelect)
            onSelect(node);
        return true;
    }
    return false;
}
void SkillMap::draw(sf::RenderTarget &target, sf::RenderStates states) const {
    if (canvas.getSize() != sf::Vector2u(600, 430))
        canvas.create(600, 430);
    canvas.clear(Theme::paper);
    sf::RenderStates content;
    content.transform.translate(0, -scroll);
    for (const auto &node : nodes) {
        auto found = edges.find(node.id);
        if (found == edges.end())
            continue;
        for (const std::string &parent : found->second)
            for (const auto &source : nodes)
                if (source.id == parent) {
                    sf::Vector2f start(source.position.x + 85, source.position.y + 29),
                        end(node.position.x + 85, node.position.y + 29);
                    sf::Vertex line[] = {sf::Vertex(start, Theme::muted),
                                         sf::Vertex(end, Theme::muted)};
                    canvas.draw(line, 2, sf::Lines, content);
                    sf::Vector2f middle = start + (end - start) * 0.55f;
                    float angle = std::atan2(end.y - start.y, end.x - start.x);
                    sf::ConvexShape arrow(3);
                    arrow.setPoint(0, middle);
                    arrow.setPoint(1, middle - sf::Vector2f(10 * std::cos(angle - 0.5f),
                                                            10 * std::sin(angle - 0.5f)));
                    arrow.setPoint(2, middle - sf::Vector2f(10 * std::cos(angle + 0.5f),
                                                            10 * std::sin(angle + 0.5f)));
                    arrow.setFillColor(Theme::muted);
                    canvas.draw(arrow, content);
                }
    }
    for (const auto &node : nodes) {
        auto found = snapshot.skills.find(node.id);
        bool known = found != snapshot.skills.end() && found->second.known;
        rectangle(canvas, content, sf::Vector2f(170, 58), known ? Theme::pale : Theme::background,
                  sf::Vector2f(node.position.x, node.position.y), Theme::line);
        canvas.draw(label(font, shorten(font, unicode(node.name), 16, 154), 16, node.position.x + 8,
                          node.position.y + 8),
                    content);
        std::string value =
            known ? std::to_string(static_cast<int>(found->second.mastery * 100)) + "%"
                  : "Нет наблюдений";
        canvas.draw(label(font, unicode(value), 13, node.position.x + 8, node.position.y + 34,
                          Theme::muted),
                    content);
    }
    canvas.display();
    states.transform *= getTransform();
    target.draw(sf::Sprite(canvas.getTexture()), states);
}
} // namespace ui
} // namespace exam
