#include "SkillMap.h"
#include "AppContext.h"
#include <algorithm>
#include <cmath>
#include <set>
namespace exam {
namespace ui {
SkillMap::SkillMap(const sf::Font &font, const std::vector<SkillInfo> &skills,
                   const std::map<std::string, std::vector<std::string>> &graph,
                   const StudentSnapshot &state)
    : Widget(font, 600, 430), nodes(skills), edges(graph), snapshot(state), dragged(-1),
      editable(true), resultsHidden(false), scroll(0), originY(0) {
    for (std::size_t i = 0; i < nodes.size(); ++i)
        if (!nodes[i].placed)
            nodes[i].position = Point(15 + (i % 3) * 195, 20 + (i / 3) * 110);
    if (!nodes.empty()) {
        originY = static_cast<float>(nodes.front().position.y);
        for (const auto &node : nodes)
            originY = std::min(originY, static_cast<float>(node.position.y));
        originY = std::max(0.f, originY - 15);
        for (auto &node : nodes)
            node.position.y -= originY;
    }
}
bool SkillMap::handle(const sf::Event &event, sf::Vector2f mouse) {
    sf::Vector2f local = getInverseTransform().transformPoint(mouse);
    local.x /= size.x / 600.f;
    if (event.type == sf::Event::MouseWheelScrolled && contains(mouse)) {
        float height = size.y;
        for (const auto &node : nodes)
            height = std::max(height, static_cast<float>(node.position.y + 100));
        scroll =
            std::max(0.f, std::min(height - size.y, scroll - event.mouseWheelScroll.delta * 50));
        return true;
    }
    local.y += scroll;
    if (event.type == sf::Event::MouseMoved) {
        hovered = -1;
        if (contains(mouse))
            for (std::size_t i = 0; i < nodes.size(); ++i)
                if (sf::FloatRect(nodes[i].position.x, nodes[i].position.y, 170, 90)
                        .contains(local))
                    hovered = int(i);
    }
    if (focused && event.type == sf::Event::KeyPressed && !nodes.empty()) {
        int index = 0;
        for (std::size_t i = 0; i < nodes.size(); ++i)
            if (nodes[i].id == selected)
                index = int(i);
        if (event.key.code == sf::Keyboard::Right || event.key.code == sf::Keyboard::Down)
            index = std::min(int(nodes.size()) - 1, index + 1);
        else if (event.key.code == sf::Keyboard::Left || event.key.code == sf::Keyboard::Up)
            index = std::max(0, index - 1);
        else
            return false;
        if (onSelect)
            onSelect(nodes[index]);
        return true;
    }
    if (event.type == sf::Event::MouseButtonPressed &&
        event.mouseButton.button == sf::Mouse::Left && contains(mouse)) {
        for (int i = static_cast<int>(nodes.size()) - 1; i >= 0; --i)
            if (sf::FloatRect(nodes[i].position.x, nodes[i].position.y, 170, 90).contains(local)) {
                dragged = i;
                offset = sf::Vector2f(local.x - nodes[i].position.x, local.y - nodes[i].position.y);
                return true;
            }
    }
    if (event.type == sf::Event::MouseMoved && dragged >= 0 && editable) {
        nodes[dragged].position =
            Point(std::max(0.f, std::min(600.f - 170, local.x - offset.x)),
                  std::max(0.f, std::min(size.y + scroll - 90, local.y - offset.y)));
        return true;
    }
    if (event.type == sf::Event::MouseButtonReleased && dragged >= 0) {
        SkillInfo node = nodes[dragged];
        node.position.y += originY;
        dragged = -1;
        node.placed = true;
        if (editable && onMove)
            onMove(node);
        else if (onSelect)
            onSelect(node);
        return true;
    }
    return false;
}
void SkillMap::setSelected(const std::string &id) {
    selected = id;
    for (const auto &node : nodes)
        if (node.id == id) {
            if (node.position.y < scroll)
                scroll = node.position.y;
            if (node.position.y + 100 > scroll + size.y)
                scroll = node.position.y + 100 - size.y;
        }
}
namespace {
void rounded(sf::RenderTarget &target, sf::RenderStates states, sf::Vector2f position,
             sf::Vector2f size, sf::Color color, sf::Color border = sf::Color::Transparent) {
    sf::ConvexShape shape(32);
    const float radius = 9;
    for (int corner = 0; corner < 4; ++corner) {
        sf::Vector2f center(corner == 0 || corner == 3 ? size.x - radius : radius,
                            corner < 2 ? size.y - radius : radius);
        for (int i = 0; i < 8; ++i) {
            float angle = (corner * 90.f + i * 90.f / 7) * 3.14159265f / 180;
            shape.setPoint(corner * 8 + i,
                           center + sf::Vector2f(std::cos(angle), std::sin(angle)) * radius);
        }
    }
    shape.setPosition(position);
    shape.setFillColor(color);
    shape.setOutlineThickness(border.a ? -1.f : 0.f);
    shape.setOutlineColor(border);
    target.draw(shape, states);
}
void connection(sf::RenderTarget &target, sf::RenderStates states, sf::Vector2f start,
                sf::Vector2f end, bool horizontal, sf::Color color, float thickness) {
    float direction =
        horizontal ? (end.x >= start.x ? 1.f : -1.f) : (end.y >= start.y ? 1.f : -1.f);
    float bend = std::max(24.f, std::abs(horizontal ? end.x - start.x : end.y - start.y) * .5f);
    sf::Vector2f offset =
        horizontal ? sf::Vector2f(bend * direction, 0) : sf::Vector2f(0, bend * direction);
    sf::Vector2f c1 = start + offset, c2 = end - offset;
    sf::VertexArray ribbon(sf::TriangleStrip);
    for (int i = 0; i <= 32; ++i) {
        float t = i / 32.f, u = 1 - t;
        sf::Vector2f p =
            start * (u * u * u) + c1 * (3 * u * u * t) + c2 * (3 * u * t * t) + end * (t * t * t);
        sf::Vector2f tangent =
            (c1 - start) * (3 * u * u) + (c2 - c1) * (6 * u * t) + (end - c2) * (3 * t * t);
        float length = std::max(.001f, std::sqrt(tangent.x * tangent.x + tangent.y * tangent.y));
        sf::Vector2f normal(-tangent.y / length * thickness / 2,
                            tangent.x / length * thickness / 2);
        ribbon.append(sf::Vertex(p + normal, color));
        ribbon.append(sf::Vertex(p - normal, color));
    }
    target.draw(ribbon, states);
    sf::Vector2f directionVector =
        horizontal ? sf::Vector2f(direction, 0) : sf::Vector2f(0, direction);
    sf::Vector2f normal(-directionVector.y, directionVector.x);
    sf::ConvexShape arrow(3);
    arrow.setPoint(0, end);
    arrow.setPoint(1, end - directionVector * 9.f + normal * 4.f);
    arrow.setPoint(2, end - directionVector * 9.f - normal * 4.f);
    arrow.setFillColor(color);
    target.draw(arrow, states);
}
} // namespace
void SkillMap::draw(sf::RenderTarget &target, sf::RenderStates states) const {
    prepareCanvas(canvas, size);
    canvas.clear(Theme::paper);
    sf::RenderStates content;
    content.transform.translate(0, -scroll);
    float factor = size.x / 600.f;
    std::set<std::string> related;
    related.insert(selected);
    auto selectedEdges = edges.find(selected);
    if (selectedEdges != edges.end())
        related.insert(selectedEdges->second.begin(), selectedEdges->second.end());
    for (const auto &node : nodes) {
        auto found = edges.find(node.id);
        if (found == edges.end())
            continue;
        for (const auto &parent : found->second)
            for (const auto &source : nodes)
                if (source.id == parent) {
                    bool active = node.id == selected || source.id == selected;
                    if (active)
                        related.insert(node.id);
                    sf::Vector2f start((source.position.x + 85) * factor, source.position.y + 45),
                        end((node.position.x + 85) * factor, node.position.y + 45);
                    bool horizontal = std::abs(end.x - start.x) > std::abs(end.y - start.y);
                    if (horizontal) {
                        float sign = end.x >= start.x ? 1.f : -1.f;
                        start.x += 85 * factor * sign;
                        end.x -= (85 * factor + 3) * sign;
                    } else {
                        float sign = end.y >= start.y ? 1.f : -1.f;
                        start.y += 45 * sign;
                        end.y -= 48 * sign;
                    }
                    connection(canvas, content, start, end, horizontal,
                               active ? Theme::accent : sf::Color(184, 195, 181),
                               active ? 2.2f : 1.3f);
                }
    }
    for (std::size_t i = 0; i < nodes.size(); ++i) {
        const auto &node = nodes[i];
        auto found = snapshot.skills.find(node.id);
        bool known = !resultsHidden && found != snapshot.skills.end() && found->second.known &&
                     found->second.observations > 0;
        bool active = node.id == selected;
        sf::Vector2f position(node.position.x * factor, node.position.y),
            cardSize(170 * factor, 90);
        sf::Color border = active                   ? Theme::accent
                           : related.count(node.id) ? sf::Color(160, 179, 156)
                                                    : Theme::line;
        rounded(canvas, content, position + sf::Vector2f(0, 2), cardSize, sf::Color(230, 233, 224));
        rounded(canvas, content, position, cardSize,
                active || int(i) == hovered ? Theme::pale : Theme::white, border);
        drawText(canvas,
                 label(font, wrap(font, unicode(node.name), 14, cardSize.x - 24, 3), 14,
                       position.x + 12, position.y + 10),
                 content);
        bool confident = known && found->second.mastery >= .75;
        sf::Color valueColor = known ? (confident ? Theme::ink : Theme::accent) : Theme::muted;
        std::string state = resultsHidden ? "Пока скрыто"
                            : known       ? (confident ? "Уверенно" : "Повторить")
                                          : "Не проверено";
        drawText(canvas,
                 label(font, unicode(state), 12, position.x + 12, position.y + 65, valueColor),
                 content);
        if (known) {
            drawText(
                canvas,
                label(font,
                      unicode(std::to_string(int(std::round(found->second.mastery * 100))) + "%"),
                      12, position.x + cardSize.x - 43, position.y + 65, valueColor),
                content);
            rectangle(canvas, content, sf::Vector2f((cardSize.x - 24) * found->second.mastery, 3),
                      valueColor, position + sf::Vector2f(12, 83));
        }
    }
    float totalHeight = size.y;
    for (const auto &node : nodes)
        totalHeight = std::max(totalHeight, float(node.position.y + 100));
    if (totalHeight > size.y)
        rectangle(canvas, sf::RenderStates(), sf::Vector2f(3, size.y * size.y / totalHeight),
                  Theme::accent, sf::Vector2f(size.x - 4, size.y * scroll / totalHeight));
    canvas.display();
    states.transform *= getTransform();
    drawCanvas(target, canvasSprite(canvas), states);
}
} // namespace ui
} // namespace exam
