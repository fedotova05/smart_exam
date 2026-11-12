#include "LectureScreen.h"
#include "ui/widgets/TextArea.h"
#include "ui/widgets/TextPanel.h"
#include "ui/FileDialog.h"
#include <algorithm>
namespace exam {
namespace ui {
namespace {
std::wstring searchText(const std::string &text) {
    auto value = unicode(text).toWideString();
    for (auto &c : value) {
        if ((c >= L'А' && c <= L'Я') || (c >= L'A' && c <= L'Z'))
            c += 32;
        if (c == L'Ё' || c == L'ё')
            c = L'е';
    }
    return value;
}
} // namespace
void LectureScreen::refresh() {
    clear();
    title = "Лекции";
    bool teacher = context.role == UserRole::Teacher;
    auto query = field(search, 0, 0, 470, 160);
    query->setPlaceholder("Название или текст лекции");
    query->onChange = [this, query] { search = query->getText(); };
    query->onEnter = [this] { defer([this] { refresh(); }); };
    button("Найти", 490, 0, 150, [this] { refresh(); });
    if (teacher)
        button(
            "Создать лекцию", 660, 0, 280,
            [this] {
                draft = Lecture();
                attachment.clear();
                edit();
            },
            true);
    auto skills = context.school.skills();
    std::vector<std::string> names = {"Все навыки"};
    int active = 0;
    for (const auto &s : skills) {
        names.push_back(s.name);
        if (skill == s.id)
            active = int(names.size()) - 1;
    }
    auto filters = list(names, 0, 80, 280, 420);
    filters->setRowHeight(54);
    filters->select(active);
    filters->onChange = [this, skills](int i) {
        defer([this, skills, i] {
            skill = i ? skills[i - 1].id : "";
            refresh();
        });
    };
    std::vector<Lecture> values;
    names.clear();
    auto needle = searchText(search);
    for (auto v : context.school.lectures(teacher)) {
        if (!skill.empty() && std::find(v.skills.begin(), v.skills.end(), skill) == v.skills.end())
            continue;
        if (searchText(v.title + " " + v.body).find(needle) == std::wstring::npos)
            continue;
        names.push_back(v.title + (v.published ? "" : " · черновик"));
        values.push_back(v);
    }
    auto lectures = list(names, 310, 80, 630, 420);
    lectures->setRowHeight(64);
    lectures->setEmptyText("Лекций по этому запросу пока нет");
    button(
        "Открыть лекцию", 310, 540, 300,
        [this, lectures, values] {
            if (lectures->selection() < 0)
                throw std::runtime_error("Выберите лекцию");
            view(values[lectures->selection()].id);
        },
        true)
        ->setEnabled(!values.empty());
    note(
        teacher
            ? "Ученик видит только опубликованные лекции. Для публикации выберите навыки."
            : "Лекции связаны с навыками. В тренировке нужные материалы появятся рядом с заданием.",
        0, 595, 940);
}
void LectureScreen::view(const std::string &id) {
    bool teacher = context.role == UserRole::Teacher;
    auto value = context.school.lecture(id, teacher);
    clear();
    title = value.title;
    std::string names;
    for (const auto &s : context.school.skills())
        if (std::find(value.skills.begin(), value.skills.end(), s.id) != value.skills.end())
            names += (names.empty() ? "" : ", ") + s.name;
    note(names, 0, 0, 940);
    add(new TextPanel(font,
                      value.body.empty() ? "Материал находится во вложенном файле." : value.body,
                      940, 420),
        0, 65);
    button("← К лекциям", 0, 540, 220, [this] { refresh(); });
    if (!value.attachment.empty())
        button("Открыть PDF / Word", 240, 540, 260,
               [this, id, teacher] { context.school.openLectureFile(id, teacher); });
    if (teacher)
        button(
            "Редактировать", 660, 540, 280,
            [this, value] {
                draft = value;
                attachment.clear();
                edit();
            },
            true);
    if (!value.attachment.empty())
        note("Вложение: " + value.attachmentName, 0, 600, 940);
}
void LectureScreen::edit() {
    context.school.requireTeacher();
    clear();
    title = draft.id.empty() ? "Новая лекция" : "Редактирование лекции";
    auto name = field(draft.title, 0, 0, 940, 200);
    name->setPlaceholder("Название лекции");
    name->onChange = [this, name] { draft.title = name->getText(); };
    text("Текст", 0, 65, 18);
    text("Связанные навыки", 640, 65, 18);
    auto body = add(new TextArea(font, 610, 340), 0, 100);
    body->setText(draft.body);
    body->onChange = [this, body] { draft.body = body->getText(); };
    auto skills = context.school.skills();
    std::vector<std::string> names;
    for (auto &s : skills)
        names.push_back(s.name);
    auto links = list(names, 640, 100, 300, 340);
    links->setRowHeight(60);
    links->setCheckable(true);
    for (std::size_t i = 0; i < skills.size(); ++i)
        links->setChecked(i, std::find(draft.skills.begin(), draft.skills.end(), skills[i].id) !=
                                 draft.skills.end());
    links->onToggle = [this, links, skills](int i) {
        auto id = skills[i].id;
        auto it = std::find(draft.skills.begin(), draft.skills.end(), id);
        if (it == draft.skills.end())
            draft.skills.push_back(id);
        else
            draft.skills.erase(it);
        links->setChecked(i, std::find(draft.skills.begin(), draft.skills.end(), id) !=
                                 draft.skills.end());
    };
    button("Прикрепить PDF / Word", 0, 465, 300, [this] {
        auto path = chooseLecture();
        if (!path.empty()) {
            attachment = path;
            edit();
        }
    });
    button("Убрать вложение", 320, 465, 220, [this] {
        attachment.clear();
        draft.attachment.clear();
        draft.attachmentName.clear();
        edit();
    })->setEnabled(!attachment.empty() || !draft.attachment.empty());
    note(attachment.empty() ? draft.attachmentName : attachment, 0, 515, 940);
    auto save = [this](bool published) {
        auto value = draft;
        value.published = published;
        context.school.saveLecture(value, attachment);
        attachment.clear();
        refresh();
        status = published ? "Лекция опубликована" : "Черновик сохранён";
    };
    button("Сохранить черновик", 0, 565, 250, [save] { save(false); });
    button("Опубликовать", 270, 565, 250, [save] { save(true); }, true);
    button("Отмена", 540, 565, 180,
           [this] { ask("Закрыть редактор без сохранения?", [this] { refresh(); }); });
    if (!draft.id.empty())
        button("В архив", 740, 565, 200, [this] {
            ask("Убрать лекцию из каталога?", [this] {
                auto value = draft;
                value.archived = true;
                context.school.saveLecture(value);
                refresh();
            });
        });
    focusWidget(name);
}
} // namespace ui
} // namespace exam
