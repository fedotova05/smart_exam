#include "PackageService.h"
#include "io/Files.h"
#include <set>
namespace exam {
namespace {
bool identifier(const std::string &text) {
    if (text.empty() || text.size() > 80 || text == "." || text == "..")
        return false;
    for (char c : text)
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
              c == '-' || c == '_' || c == '.'))
            return false;
    return true;
}
const std::vector<Json> &array(const Json &value) {
    if (value.type != Json::Array)
        throw std::runtime_error("Ожидался массив");
    return value.array;
}
std::string optionalText(const Json &value, const std::string &field) {
    return value.object.count(field) ? value.field(field).text() : "";
}
Json texts(const std::vector<std::string> &values) {
    Json result = Json::makeArray();
    for (const std::string &text : values)
        result.array.push_back(Json(text));
    return result;
}
} // namespace
PackagePreview PackageService::inspect(const std::string &path) const {
    Json root = parseJson(readFile(path));
    if (root.field("schema").integer() != 1)
        throw std::runtime_error("Поддерживается схема пакета 1");
    PackagePreview preview;
    preview.id = root.field("id").text();
    preview.directory = parentPath(path);
    if (!identifier(preview.id))
        throw std::runtime_error("Неверный идентификатор пакета");
    std::set<std::string> skills, ids;
    for (const Json &skill : array(root.field("skills"))) {
        std::string id = skill.text();
        if (!identifier(id) || !skills.insert(id).second)
            throw std::runtime_error("Неверный или повторный навык пакета");
        preview.skills.push_back(id);
    }
    const auto &tasks = array(root.field("tasks"));
    if (tasks.empty() || tasks.size() > 100 || skills.size() > 100)
        throw std::runtime_error("Пакет: от 1 до 100 заданий, не более 100 навыков");
    for (std::size_t index = 0; index < tasks.size(); ++index) {
        PackageItem item;
        item.sourceId = std::to_string(index + 1);
        try {
            const Json &value = tasks[index];
            item.sourceId = value.field("id").text();
            if (!identifier(item.sourceId) || !ids.insert(item.sourceId).second)
                throw std::runtime_error("Неверный или повторный id задания");
            TaskVersion &task = item.task;
            task.id = preview.id + "." + item.sourceId;
            task.title = value.field("title").text();
            task.description = optionalText(value, "description");
            task.commonFunction = optionalText(value, "function");
            if (value.field("difficulty").type != Json::Number)
                throw std::runtime_error("Сложность должна быть числом");
            task.difficulty = value.field("difficulty").number;
            const auto &parts = array(value.field("parts"));
            if (parts.empty() || parts.size() > 100)
                throw std::runtime_error("Неверное число частей");
            std::set<std::string> partIds;
            for (const Json &partValue : parts) {
                TaskPart part;
                part.id = partValue.field("id").text();
                if (!identifier(part.id) || !partIds.insert(part.id).second)
                    throw std::runtime_error("Неверный или повторный id части");
                int kind = partValue.field("kind").integer();
                if (kind < 0 || kind > 3)
                    throw std::runtime_error("Неизвестный тип части");
                part.kind = static_cast<PartKind>(kind);
                part.prompt = partValue.field("prompt").text();
                part.expression = partValue.field("expression").text();
                part.maxScore = partValue.field("score").integer();
                if (partValue.object.count("snap")) {
                    const Json &value = partValue.field("snap");
                    if (value.type != Json::Number)
                        throw std::runtime_error("Привязка должна быть числом");
                    part.graphSnap = value.number;
                }
                if (partValue.object.count("tolerance")) {
                    const Json &value = partValue.field("tolerance");
                    if (value.type != Json::Number)
                        throw std::runtime_error("Допуск должен быть числом");
                    part.graphTolerance = value.number;
                }
                for (const Json &skill : array(partValue.field("skills"))) {
                    std::string id = skill.text();
                    if (!skills.count(id))
                        throw std::runtime_error("Навык не объявлен в пакете");
                    part.skills.push_back(id);
                }
                task.parts.push_back(part);
            }
            if (value.object.count("resources"))
                for (const Json &resource : array(value.field("resources"))) {
                    std::string relative = resource.text();
                    resourcePath(preview.directory, relative);
                    task.resources.push_back(relative);
                }
            if (task.resources.size() > 8)
                throw std::runtime_error("Не более восьми ресурсов на задание");
        } catch (const std::exception &error) {
            item.error = error.what();
        }
        preview.items.push_back(item);
    }
    return preview;
}
std::vector<ImportReport>
PackageService::importPackage(const PackagePreview &package,
                              const std::map<std::string, std::string> &mapping) {
    std::set<std::string> known, existing;
    for (const SkillInfo &skill : school.skills())
        known.insert(skill.id);
    for (const std::string &source : package.skills) {
        auto target = mapping.find(source);
        if (target == mapping.end() || !known.count(target->second))
            throw std::runtime_error("Сопоставьте навык: " + source);
    }
    for (const TaskVersion &task : school.tasks())
        existing.insert(task.id);
    std::vector<ImportReport> report;
    for (const PackageItem &item : package.items) {
        ImportReport row;
        row.id = item.sourceId;
        row.imported = false;
        try {
            if (!item.error.empty())
                throw std::runtime_error(item.error);
            if (existing.count(item.task.id)) {
                row.message = "Уже существует; пропущено";
                report.push_back(row);
                continue;
            }
            TaskVersion task = item.task;
            for (TaskPart &part : task.parts)
                for (std::string &skill : part.skills)
                    skill = mapping.at(skill);
            std::vector<std::pair<std::string, std::string>> resources;
            task.resources.clear();
            for (const std::string &source : item.task.resources) {
                std::string target = "imports/" + package.id + "/" + item.sourceId + "/" + source;
                std::string output = resourcePath(dataDirectory, target);
                resources.push_back(
                    {output, readFile(resourcePath(package.directory, source), 5 * 1024 * 1024)});
                task.resources.push_back(target);
            }
            std::vector<std::string> errors = school.validate(task);
            if (!errors.empty())
                throw std::runtime_error(errors.front());
            for (const auto &resource : resources) {
                makeDirectories(parentPath(resource.first));
                writeFile(resource.first, resource.second);
            }
            school.saveDraft(task);
            existing.insert(task.id);
            row.imported = true;
            row.message = "Черновик сохранён";
        } catch (const std::exception &error) {
            row.message = error.what();
        }
        report.push_back(row);
    }
    return report;
}
void PackageService::exportPackage(const std::string &path,
                                   const std::vector<std::string> &taskIds) const {
    if (taskIds.empty() || taskIds.size() > 100)
        throw std::runtime_error("Выберите от 1 до 100 заданий");
    Json root = Json::makeObject();
    root.object["schema"] = Json(1.0);
    root.object["id"] = Json(std::string("school"));
    Json tasks = Json::makeArray();
    std::set<std::string> skillIds;
    std::vector<std::pair<std::string, std::string>> copies;
    for (const std::string &id : taskIds) {
        TaskVersion task = school.task(id);
        Json item = Json::makeObject();
        if (!identifier(id))
            throw std::runtime_error("Id задания не подходит для пакета");
        item.object["id"] = Json(id);
        item.object["title"] = Json(task.title);
        item.object["description"] = Json(task.description);
        item.object["function"] = Json(task.commonFunction);
        item.object["difficulty"] = Json(task.difficulty);
        Json parts = Json::makeArray();
        for (const TaskPart &part : task.parts) {
            Json value = Json::makeObject();
            value.object["id"] = Json(part.id);
            value.object["kind"] = Json(static_cast<double>(static_cast<int>(part.kind)));
            value.object["prompt"] = Json(part.prompt);
            value.object["expression"] = Json(part.expression);
            value.object["score"] = Json(static_cast<double>(part.maxScore));
            value.object["skills"] = texts(part.skills);
            skillIds.insert(part.skills.begin(), part.skills.end());
            parts.array.push_back(value);
            parts.array.back().object["snap"] = Json(part.graphSnap);
            parts.array.back().object["tolerance"] = Json(part.graphTolerance);
        }
        item.object["parts"] = parts;
        item.object["resources"] = texts(task.resources);
        tasks.array.push_back(item);
        for (const std::string &resource : task.resources)
            copies.push_back({resourcePath(parentPath(path), resource),
                              readFile(resourcePath(dataDirectory, resource), 5 * 1024 * 1024)});
    }
    root.object["tasks"] = tasks;
    root.object["skills"] = texts(std::vector<std::string>(skillIds.begin(), skillIds.end()));
    for (const auto &copy : copies) {
        makeDirectories(parentPath(copy.first));
        writeFile(copy.first, copy.second);
    }
    writeFile(path, writeJson(root) + "\n");
}
} // namespace exam
