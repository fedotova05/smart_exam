#include "BankScreen.h"
#include "ui/FormulaView.h"
#include "io/Files.h"
#include <algorithm>
#include <sstream>
namespace exam {
namespace ui {
void BankScreen::createTask(bool combined) {
    typeDrafts.clear();
    scoreDrafts.clear();
    draft = TaskVersion();
    draft.id = newId("task");
    draft.title = combined ? "Корни и график функции" : "Новое задание";
    TaskPart part;
    part.id = "a";
    part.prompt = "Решите уравнение";
    part.expression = "x^2-4=0";
    auto skills = context.school.skills();
    if (!skills.empty())
        part.skills.push_back(skills.front().id);
    draft.parts.push_back(part);
    if (combined) {
        draft.commonFunction = "x^2-4";
        draft.description = "Найдите нули функции и постройте её график.";
        part.id = "b";
        part.kind = PartKind::Parabola;
        part.prompt = "Постройте параболу";
        part.expression = draft.commonFunction;
        draft.parts.push_back(part);
    }
    editing = true;
    partIndex = 0;
    editor();
}
void BankScreen::refresh() {
    if (context.role != UserRole::Teacher) {
        clear();
        title = "Банк заданий доступен учителю";
        return;
    }
    if (editing) {
        editor();
        return;
    }
    clear();
    title = "Банк заданий";
    TextField *search = field(query, 0, 0, 700, 120);
    search->setPlaceholder("Поиск по названию, условию или навыку");
    button("Найти", 720, 0, 220, [this, search] {
        query = search->getText();
        refresh();
    });
    search->onEnter = [this, search] {
        query = search->getText();
        defer([this] { refresh(); });
    };
    auto tasks = context.school.tasks();
    std::vector<TaskVersion> found;
    std::vector<std::string> rows;
    sf::String needle = unicode(query);
    for (std::size_t i = 0; i < needle.getSize(); ++i) {
        if (needle[i] >= 'A' && needle[i] <= 'Z')
            needle[i] += 32;
        if (needle[i] >= 0x410 && needle[i] <= 0x42f)
            needle[i] += 32;
    }
    for (const auto &task : tasks) {
        std::string contents = task.title + " " + task.description + " " + task.id;
        for (const auto &part : task.parts) {
            contents += " " + part.prompt;
            for (const auto &skill : context.school.skills())
                if (std::find(part.skills.begin(), part.skills.end(), skill.id) !=
                    part.skills.end())
                    contents += " " + skill.name;
        }
        sf::String haystack = unicode(contents);
        for (std::size_t i = 0; i < haystack.getSize(); ++i) {
            if (haystack[i] >= 'A' && haystack[i] <= 'Z')
                haystack[i] += 32;
            if (haystack[i] >= 0x410 && haystack[i] <= 0x42f)
                haystack[i] += 32;
        }
        if (haystack.find(needle) == sf::String::InvalidPos)
            continue;
        found.push_back(task);
        rows.push_back((task.archived    ? "Архив"
                        : task.published ? "Опубликовано"
                                         : "Черновик") +
                       std::string(" · ") + task.title +
                       " · частей: " + std::to_string(task.parts.size()));
    }
    ListBox *bank = list(rows, 0, 70, 940, 350);
    bank->setRowHeight(56);
    bank->setEmptyText("Задания не найдены. Измените поиск или создайте новое.");
    button("Открыть", 0, 445, 210, [this, bank, found] {
        if (bank->selection() < 0)
            return;
        const bool published = found[bank->selection()].published;
        draft = found[bank->selection()];
        if (draft.published) {
            draft = context.school.newVersion(draft.id);
        }
        typeDrafts.clear();
        scoreDrafts.clear();
        editing = true;
        partIndex = 0;
        editor();
        if (published)
            status = "Создана новая версия. Исходное задание и решения сохранены.";
    });
    button(
        "Новое задание", 230, 445, 220, [this] { createTask(false); }, true);
    button("Корни + график", 470, 445, 220, [this] { createTask(true); });
    button("В архив", 710, 445, 230, [this, bank, found] {
        if (bank->selection() < 0)
            return;
        std::string id = found[bank->selection()].id;
        ask("Убрать задание из подбора? История сохранится.", [this, id] {
            context.school.archive(id);
            refresh();
        });
    });
    text("Черновики можно исправлять. Для опубликованного задания создаётся новая версия. Проверка "
         "условий выполняется перед публикацией.",
         0, 520, 18, 900, 3);
}
} // namespace ui
} // namespace exam
