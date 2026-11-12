#include "ExamService.h"
#include "io/Html.h"
#include "io/Https.h"
#include "io/Files.h"
#include "io/Json.h"
#include "io/SvgImage.h"
#include <algorithm>
#include <set>
#include <sstream>
#include <iomanip>
#include <cstdint>
namespace exam {
namespace {
std::string fingerprint(const std::string &s) {
    std::uint64_t n = 14695981039346656037ULL;
    for (unsigned char c : s) {
        n ^= c;
        n *= 1099511628211ULL;
    }
    std::ostringstream o;
    o << std::hex << std::setw(16) << std::setfill('0') << n;
    return o.str();
}
bool digits(const std::string &s) {
    return !s.empty() && s.size() < 12 && s.find_first_not_of("0123456789") == std::string::npos;
}
bool answer(const std::string &s) {
    if (s.empty() || s.size() > 40)
        return false;
    std::size_t i = s[0] == '-' ? 1 : 0;
    bool decimal = false, number = false;
    for (; i < s.size(); ++i) {
        char c = s[i];
        if (c >= '0' && c <= '9') {
            number = true;
            continue;
        }
        if ((c == ',' || c == '.') && !decimal && number && i + 1 < s.size()) {
            decimal = true;
            continue;
        }
        return false;
    }
    return number;
}
std::string practicalSkill(const std::string &text) {
    if (text.find("стоимост") != std::string::npos || text.find("выгодн") != std::string::npos ||
        text.find("заплат") != std::string::npos || text.find("эконом") != std::string::npos)
        return "oge_cost";
    if (text.find("процент") != std::string::npos)
        return "oge_percent";
    if (text.find("обозначен") != std::string::npos || text.find("соответствие") != std::string::npos ||
        text.find("номера") != std::string::npos || text.find("Заполните таблицу") != std::string::npos)
        return "oge_tables";
    if (text.find("упаков") != std::string::npos || text.find("пачек") != std::string::npos)
        return "oge_rounding";
    if (text.find("объем") != std::string::npos || text.find("объём") != std::string::npos)
        return "oge_volume";
    if (text.find("площад") != std::string::npos)
        return "oge_grid_area";
    return "oge_practical";
}
std::string assignedSkill(const std::string &text, const ExamCategory &category) {
    if (category.number <= 5)
        return practicalSkill(text);
    if (category.id == 69 && text.find("ероятност") != std::string::npos)
        return "oge_probability";
    // В каталоге встречаются задачи о треугольниках в разделе окружностей.
    if (category.number == 16 && text.find("окружност") == std::string::npos &&
        text.find("круг") == std::string::npos && text.find("треугольник") != std::string::npos) {
        if (text.find("площад") != std::string::npos)
            return "oge_triangle_area";
        return "oge_triangles";
    }
    if (category.id == 32 && text.find("площад") != std::string::npos)
        return "oge_triangle_area";
    return category.skill;
}
} // namespace
std::vector<ImportedExamTask> parseSdamgia(const std::string &html, const ExamCategory &category) {
    std::vector<ImportedExamTask> result;
    for (const auto &card : htmlElements(html, "prob_maindiv")) {
        auto headers = htmlElements(card.inner, "prob_nums");
        if (headers.empty())
            continue;
        auto label = htmlText(headers.front().inner);
        auto typeAt = label.find("Тип ");
        if (typeAt == std::string::npos)
            continue;
        int number = 0;
        try {
            number = std::stoi(label.substr(typeAt + std::string("Тип ").size()));
        } catch (...) {
            continue;
        }
        if (number != category.number)
            continue;
        auto idAt = headers.front().inner.find("/problem?id=");
        if (idAt == std::string::npos)
            continue;
        idAt += 12;
        auto end = idAt;
        while (end < headers.front().inner.size() &&
               std::isdigit((unsigned char)headers.front().inner[end]))
            ++end;
        std::string id = headers.front().inner.substr(idAt, end - idAt);
        if (!digits(id))
            continue;
        auto answers = htmlElements(card.inner, "answer");
        if (answers.size() != 1)
            continue;
        std::string key = htmlText(answers.front().inner);
        auto colon = key.find(':');
        if (colon == std::string::npos)
            continue;
        key = key.substr(colon + 1);
        while (!key.empty() && key[0] == ' ')
            key.erase(0, 1);
        if (!answer(key))
            continue;
        auto solutions = htmlElements(card.inner, "solution");
        if (solutions.empty())
            continue;
        std::string conditions = card.inner.substr(0, solutions[0].start);
        auto bodies = htmlElements(conditions, "pbody");
        if (bodies.empty())
            continue;
        ImportedExamTask v;
        auto &e = v.metadata;
        auto &t = v.task;
        e.externalId = id;
        e.number = number;
        e.category = category.id;
        e.sourceUrl = "https://math-oge.sdamgia.ru/problem?id=" + id;
        TaskPart part;
        part.id = "answer";
        part.kind = PartKind::ShortAnswer;
        part.correctAnswer = key;
        for (std::size_t i = 0; i < bodies.size(); ++i) {
            auto body = bodies[i].inner;
            auto images = htmlImages(body);
            for (std::size_t j = images.size(); j > 0; --j)
                body.insert(images[j - 1].end,
                            " [Рисунок " + std::to_string(v.images.size() + j) + "] ");
            auto text = htmlText(body);
            const std::string redundant = "[Рисунок] [Рисунок ";
            std::size_t placeholder;
            while ((placeholder = text.find(redundant)) != std::string::npos)
                text.erase(placeholder, std::string("[Рисунок] ").size());
            if (i + 1 == bodies.size())
                part.prompt = text;
            else {
                if (!t.description.empty())
                    t.description += '\n';
                t.description += text;
            }
            for (const auto &image : images) {
                auto src = htmlAttribute(image.attributes, "src");
                if (src.empty())
                    throw std::runtime_error("В карточке отсутствует адрес рисунка");
                v.images.push_back(examResourceUrl(src));
            }
        }
        if (part.prompt.empty() || part.prompt.size() > 16000 || t.description.size() > 16000 ||
            v.images.size() > 32)
            continue;
        part.skills = {assignedSkill(part.prompt, category)};
        t.parts = {part};
        t.title = "ОГЭ · № " + std::to_string(number) + " · " + id;
        t.difficulty = 1000;
        e.fingerprint =
            fingerprint(t.description + "\n" + part.prompt + "\n" + key + "\n" + part.skills[0]);
        t.id = "oge2026-sdamgia-" + id + "-" + e.fingerprint;
        e.taskId = t.id;
        t.parameters = "ОГЭ-2026; " + e.sourceUrl;
        result.push_back(v);
    }
    return result;
}
std::string
ExamService::importTask(ImportedExamTask v,
                        const std::function<std::string(const std::string &)> &download) {
    if (v.metadata.year != 2026 || v.metadata.exam != "oge" || v.metadata.subject != "math" ||
        (v.metadata.source != "sdamgia" && v.metadata.source != "package"))
        throw std::runtime_error("Неподдерживаемая версия экзамена");
    if (v.metadata.source == "package")
        store.requireTeacher();
    bool categoryValid = false;
    for (const auto &c : ogeCategories())
        if (c.id == v.metadata.category && c.number == v.metadata.number)
            categoryValid = true;
    if (!categoryValid || !digits(v.metadata.externalId) || v.task.parts.size() != 1 ||
        v.images.size() > 32 || !v.task.resources.empty())
        throw std::runtime_error("Неверная карточка ОГЭ");
    auto &part = v.task.parts[0];
    if (part.kind != PartKind::ShortAnswer || !answer(part.correctAnswer))
        throw std::runtime_error("Нужен проверенный краткий ответ");
    v.metadata.fingerprint =
        fingerprint(v.task.title + "\n" + v.task.description + "\n" + part.prompt + "\n" +
                    part.correctAnswer + "\n" + std::to_string(v.metadata.category));
    for (const auto &skill : part.skills)
        v.metadata.fingerprint = fingerprint(v.metadata.fingerprint + skill);
    std::string imageRevision;
    int imageNumber = 0;
    for (const auto &url : v.images) {
        std::string bytes = download(examResourceUrl(url)), ext;
        if (bytes.compare(0, 8, "\211PNG\r\n\032\n") == 0)
            ext = ".png";
        else if (bytes.size() > 3 && (unsigned char)bytes[0] == 255 &&
                 (unsigned char)bytes[1] == 216 && (unsigned char)bytes[2] == 255)
            ext = ".jpg";
        else if (bytes.find("<svg") != std::string::npos && bytes.size() < 512 * 1024 &&
                 bytes.find("<!ENTITY") == std::string::npos)
            ext = ".svg";
        else
            throw std::runtime_error("Рисунок имеет неподдерживаемый формат");
        if (ext == ".svg") {
            bytes = svgToPng(bytes);
            ext = ".png";
        }
        auto hash = fingerprint(bytes);
        imageRevision += hash;
        std::ostringstream prefix;
        prefix << std::setw(2) << std::setfill('0') << ++imageNumber;
        std::string name = "exam/" + prefix.str() + "-" + hash + ext;
        auto path = resourcePath("data", name);
        makeDirectories(parentPath(path));
        if (!fileExists(path) || readFile(path, 8 * 1024 * 1024) != bytes)
            writeFileAtomic(path, bytes);
        if (std::find(v.task.resources.begin(), v.task.resources.end(), name) ==
            v.task.resources.end())
            v.task.resources.push_back(name);
    }
    v.metadata.fingerprint = fingerprint(v.metadata.fingerprint + imageRevision);
    v.task.id =
        "oge2026-" + v.metadata.source + "-" + v.metadata.externalId + "-" + v.metadata.fingerprint;
    v.metadata.taskId = v.task.id;
    SchoolService school(store);
    auto errors = school.validate(v.task);
    if (!errors.empty())
        throw std::runtime_error(errors.front());
    if (!store.hasTask(v.task.id))
        store.saveDraft(v.task);
    store.saveExamTask(v.metadata);
    auto existing = store.task(v.task.id);
    if (existing.archived)
        throw std::runtime_error("Это задание исключено учителем из банка");
    if (!existing.published)
        store.publish(v.task.id);
    return v.task.id;
}
ExamLoadResult ExamService::importPackage(const std::string &path,
                                          const std::function<void()> &checkpoint) {
    store.requireTeacher();
    installOgeCurriculum(store);
    auto document = parseJson(readFile(path, 2 * 1024 * 1024));
    if (document.field("schema").text() != "oge-2026/1")
        throw std::runtime_error("Ожидался пакет ОГЭ-2026 версии 1");
    const auto &items = document.field("tasks").array;
    if (items.empty() || items.size() > 100)
        throw std::runtime_error("В пакете должно быть от 1 до 100 заданий");
    ExamLoadResult result;
    std::set<std::string> known;
    for (const auto &task : store.examTasks())
        known.insert(task.taskId);
    for (const auto &item : items) {
        if (checkpoint)
            checkpoint();
        try {
            store.requireTeacher();
            int categoryId = item.field("category").integer();
            const ExamCategory *category = 0;
            for (const auto &c : ogeCategories())
                if (c.id == categoryId)
                    category = &c;
            if (!category)
                throw std::runtime_error("Неизвестный подтип ОГЭ");
            ImportedExamTask v;
            auto &meta = v.metadata;
            meta.source = "package";
            meta.externalId = item.field("id").text();
            meta.number = category->number;
            meta.category = categoryId;
            meta.sourceUrl = "Локальный пакет: " + document.field("title").text();
            if (meta.sourceUrl.size() > 400)
                throw std::runtime_error("Слишком длинное название пакета");
            v.task.title = "ОГЭ · № " + std::to_string(meta.number) + " · " + meta.externalId;
            v.task.parameters = meta.sourceUrl;
            TaskPart part;
            part.id = "answer";
            part.kind = PartKind::ShortAnswer;
            part.prompt = item.field("condition").text();
            part.correctAnswer = item.field("answer").text();
            part.skills = {assignedSkill(part.prompt, *category)};
            v.task.parts = {part};
            v.task.difficulty = 1000;
            auto id = importTask(v, [](const std::string &) -> std::string {
                throw std::runtime_error("В локальном пакете нет сетевых ресурсов");
            });
            if (std::find(result.tasks.begin(), result.tasks.end(), id) == result.tasks.end()) {
                result.tasks.push_back(id);
                if (known.insert(id).second)
                    ++result.added;
            }
        } catch (const std::exception &e) {
            if (checkpoint)
                checkpoint();
            result.warnings.push_back(e.what());
        }
    }
    return result;
}
ExamLoadResult ExamService::load(int categoryId, int count, bool online,
                                 const std::function<void()> &checkpoint,
                                 const std::vector<std::string> &excluded) {
    if (count < 1 || count > 20)
        throw std::runtime_error("Выберите от 1 до 20 заданий");
    const ExamCategory *category = 0;
    for (const auto &c : ogeCategories())
        if (c.id == categoryId)
            category = &c;
    if (!category)
        throw std::runtime_error("Неизвестный подтип ОГЭ");
    installOgeCurriculum(store);
    ExamLoadResult result;
    std::set<std::string> external;
    auto cached = store.examTasks(category->number, categoryId);
    for (const auto &e : cached)
        if (std::find(excluded.begin(), excluded.end(), e.taskId) != excluded.end())
            external.insert(e.source + ":" + e.externalId);
    for (const auto &e : cached) {
        if (checkpoint)
            checkpoint();
        auto task = store.task(e.taskId);
        auto externalKey = e.source + ":" + e.externalId;
        if (!task.published || task.archived || external.count(externalKey))
            continue;
        bool valid = true;
        for (const auto &r : task.resources) {
            try {
                auto bytes = readFile(resourcePath("data", r), 8 * 1024 * 1024);
                auto dot = r.find_last_of('.');
                if (dot < 16 || r.substr(dot - 16, 16) != fingerprint(bytes))
                    valid = false;
            } catch (const std::exception &) {
                valid = false;
            }
        }
        if (valid && int(result.tasks.size()) < count) {
            external.insert(externalKey);
            result.tasks.push_back(e.taskId);
        }
    }
    if (!online || int(result.tasks.size()) >= count)
        return result;
    auto fetch = [&](const std::string &url) { return downloadExamUrl(url, checkpoint); };
    std::set<std::string> pageIds;
    for (int page = 1; page <= 4 && int(result.tasks.size()) < count; ++page) {
        if (checkpoint)
            checkpoint();
        try {
            std::string url =
                "https://math-oge.sdamgia.ru/test?category_id=" + std::to_string(categoryId) +
                "&filter=all&page=" + std::to_string(page);
            auto tasks = parseSdamgia(fetch(url), *category);
            if (tasks.empty()) {
                result.warnings.push_back(
                    "Источник не вернул пригодных карточек. Попробуйте другой подтип");
                break;
            }
            int newIds = 0;
            for (auto &v : tasks) {
                if (checkpoint)
                    checkpoint();
                if (int(result.tasks.size()) >= count)
                    break;
                if (pageIds.insert(v.metadata.externalId).second)
                    ++newIds;
                if (!external.insert(v.metadata.source + ":" + v.metadata.externalId).second)
                    continue;
                try {
                    auto id = importTask(v, fetch);
                    result.tasks.push_back(id);
                    ++result.added;
                } catch (const std::exception &e) {
                    if (checkpoint)
                        checkpoint();
                    result.warnings.push_back(v.metadata.externalId + ": " + e.what());
                }
            }
            if (!newIds)
                break;
        } catch (const std::exception &e) {
            if (checkpoint)
                checkpoint();
            result.warnings.push_back(e.what());
            break;
        }
    }
    return result;
}
} // namespace exam
