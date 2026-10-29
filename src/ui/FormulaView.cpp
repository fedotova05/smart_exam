#include "FormulaView.h"
#include <algorithm>
#include <cmath>
namespace exam {
namespace ui {
FormulaView::FormulaView(const sf::Font &font, float width, float height)
    : Widget(font, width, height), zoom(1), scroll(0), featured(false) {}
FormulaView::Box FormulaView::word(const std::string &text, unsigned size) const {
    Box box;
    sf::Text glyph(unicode(text), font, size);
    glyph.setFillColor(Theme::ink);
    if (text == "x")
        glyph.setStyle(sf::Text::Italic);
    box.width = glyph.getLocalBounds().width + 5;
    box.height = size * 1.3f;
    box.baseline = size;
    box.texts.push_back(glyph);
    return box;
}
void FormulaView::append(Box &target, const Box &child, float x, float y) const {
    for (auto glyph : child.texts) {
        glyph.move(x, y);
        target.texts.push_back(glyph);
    }
    for (auto line : child.lines) {
        line.move(x, y);
        target.lines.push_back(line);
    }
}
FormulaView::Box FormulaView::horizontal(const Box &left, const Box &right) const {
    Box box;
    box.baseline = std::max(left.baseline, right.baseline);
    box.width = left.width + right.width;
    box.height =
        box.baseline + std::max(left.height - left.baseline, right.height - right.baseline);
    append(box, left, 0, box.baseline - left.baseline);
    append(box, right, left.width, box.baseline - right.baseline);
    return box;
}
FormulaView::Box FormulaView::layout(const Expr &node, unsigned size) const {
    if (node->kind == NodeKind::Number)
        return word(node->number.text(), size);
    if (node->kind == NodeKind::Variable)
        return word("x", size);
    Box left = layout(node->left, size);
    if (node->kind == NodeKind::Negate) {
        if (node->left->kind == NodeKind::Add || node->left->kind == NodeKind::Subtract)
            left = horizontal(word("(", size), horizontal(left, word(")", size)));
        return horizontal(word("−", size), left);
    }
    if (node->kind == NodeKind::SquareRoot) {
        Box result = horizontal(word("√", size), left);
        sf::RectangleShape line(sf::Vector2f(left.width, 1));
        line.setFillColor(Theme::ink);
        line.setPosition(result.width - left.width, 0);
        result.lines.push_back(line);
        return result;
    }
    Box right =
        layout(node->right, node->kind == NodeKind::Power ? std::max(12u, size * 2 / 3) : size);
    if (node->kind == NodeKind::Divide) {
        Box result;
        result.width = std::max(left.width, right.width) + 14;
        result.height = left.height + right.height + 12;
        result.baseline = left.height + 12;
        append(result, left, (result.width - left.width) / 2, 0);
        append(result, right, (result.width - right.width) / 2, left.height + 12);
        sf::RectangleShape line(sf::Vector2f(result.width, 1.5f));
        line.setFillColor(Theme::ink);
        line.setPosition(0, left.height + 5);
        result.lines.push_back(line);
        return result;
    }
    if (node->kind == NodeKind::Power) {
        if (node->left->kind != NodeKind::Variable && node->left->kind != NodeKind::Number)
            left = horizontal(word("(", size), horizontal(left, word(")", size)));
        Box result;
        result.width = left.width + right.width;
        result.height = left.height + right.height * 0.6f;
        result.baseline = left.baseline + right.height * 0.6f;
        append(result, left, 0, right.height * 0.6f);
        append(result, right, left.width, 0);
        return result;
    }
    // Скобки сохраняют порядок действий при вложении суммы в произведение.
    if (node->kind == NodeKind::Multiply) {
        if (node->left->kind == NodeKind::Add || node->left->kind == NodeKind::Subtract)
            left = horizontal(word("(", size), horizontal(left, word(")", size)));
        if (node->right->kind == NodeKind::Add || node->right->kind == NodeKind::Subtract)
            right = horizontal(word("(", size), horizontal(right, word(")", size)));
    }
    if (node->kind == NodeKind::Subtract &&
        (node->right->kind == NodeKind::Add || node->right->kind == NodeKind::Subtract))
        right = horizontal(word("(", size), horizontal(right, word(")", size)));
    return horizontal(left, horizontal(word(node->kind == NodeKind::Add        ? " + "
                                            : node->kind == NodeKind::Subtract ? " − "
                                                                               : " · ",
                                            size),
                                       right));
}
void FormulaView::setFormula(const std::string &source) {
    unsigned typeSize = featured ? 36 : 28;
    formula = Box();
    error.clear();
    scroll = 0;
    if (source.empty())
        return;
    try {
        if (source == "none" || source == "{}" || source == "нет корней")
            formula = word("Нет корней", typeSize);
        else if (source == "all")
            formula = word("Все действительные числа", typeSize);
        else if (source.find(';') != std::string::npos) {
            std::size_t start = 0;
            while (start < source.size()) {
                std::size_t end = source.find(';', start);
                std::string part =
                    source.substr(start, end == std::string::npos ? end : end - start);
                if (start > 0)
                    formula = horizontal(formula, word("; ", typeSize));
                formula = horizontal(formula, layout(parseExpression(part), typeSize));
                if (end == std::string::npos)
                    break;
                start = end + 1;
                if (start == source.size())
                    throw MathError("Введите следующий корень", false, static_cast<int>(start));
            }
        } else if (source.find('=') != std::string::npos) {
            Equation equation = parseEquation(source);
            formula =
                horizontal(layout(equation.left, typeSize),
                           horizontal(word(" = ", typeSize), layout(equation.right, typeSize)));
        } else
            formula = layout(parseExpression(source), typeSize);
    } catch (const MathError &problem) {
        error = problem.what();
        if (problem.position >= 0)
            error += " (позиция " + std::to_string(problem.position + 1) + ")";
    }
}
bool FormulaView::handle(const sf::Event &event, sf::Vector2f mouse) {
    if (event.type != sf::Event::MouseWheelScrolled || !contains(mouse))
        return false;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::LControl))
        zoom = std::max(0.5f, std::min(2.f, zoom + event.mouseWheelScroll.delta * 0.1f));
    else
        scroll = std::max(0.f, std::min(std::max(0.f, formula.width * zoom - size.x + 32),
                                        scroll - event.mouseWheelScroll.delta * 35));
    return true;
}
void FormulaView::draw(sf::RenderTarget &target, sf::RenderStates states) const {
    if (canvas.getSize() !=
        sf::Vector2u(static_cast<unsigned>(size.x), static_cast<unsigned>(size.y)))
        canvas.create(static_cast<unsigned>(size.x), static_cast<unsigned>(size.y));
    canvas.clear(featured ? Theme::pale : Theme::background);
    if (!error.empty())
        canvas.draw(
            label(font, wrap(font, unicode(error), 16, size.x - 24, 4), 16, 12, 12, Theme::error));
    else {
        float actualZoom = std::min(zoom, (size.y - 20) / std::max(1.f, formula.height));
        sf::RenderStates local;
        float left = featured ? std::max(16.f, (size.x - formula.width * actualZoom) / 2) : 16.f;
        local.transform.translate(left - scroll, (size.y - formula.height * actualZoom) / 2)
            .scale(actualZoom, actualZoom);
        for (const auto &glyph : formula.texts)
            canvas.draw(glyph, local);
        for (const auto &line : formula.lines)
            canvas.draw(line, local);
    }
    canvas.display();
    states.transform *= getTransform();
    target.draw(sf::Sprite(canvas.getTexture()), states);
}
} // namespace ui
} // namespace exam
