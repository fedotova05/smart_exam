#include "WelcomeScreen.h"
namespace exam {
namespace ui {
void WelcomeScreen::enter(const std::string &id, const std::string &name) {
    context.school.logoutTeacher();
    context.selectProfile(id, name);
    context.role = UserRole::Student;
    context.page = "home";
}
void WelcomeScreen::refresh() {
    clear();
    title = "";
    note("МАТЕМАТИКА / ЛОКАЛЬНОЕ ПРИЛОЖЕНИЕ", 0, 10);
    heading("Как вы будете работать?", 0, 55, 40);
    note("Выберите роль. У ученика и учителя свои разделы.", 0, 120);
    surface(0, 183, 450, 263, Theme::paper);
    surface(490, 183, 450, 263, Theme::pale);
    note("01 / ПРАКТИКА", 28, 210, 390);
    heading("Я ученик", 28, 249, 30);
    text("Решаю задания, разбираю ошибки\nи смотрю свой прогресс.", 28, 304, 18, 390, 2);
    button("Выбрать профиль ученика →", 28, 380, 350, [this] { profiles(); }, true);
    note("02 / ПОДГОТОВКА ЗАНЯТИЙ", 518, 210, 390);
    heading("Я учитель", 518, 249, 30);
    text("Готовлю задания и занятия,\nсмотрю результаты учеников.", 518, 304, 18, 390, 2);
    button("Открыть кабинет учителя →", 518, 380, 350, [this] { teacherLogin(); });
    note("Профиль ученика — свободный вход. Кабинет учителя — по локальному паролю.", 0, 485);
    button("Посмотреть демонстрацию →", 0, 540, 350, [this] {
        std::string demo = "demo-training";
        for (const auto &profile : context.school.profiles())
            if (profile.first == "demo-deep")
                demo = profile.first;
        enter(demo, "Демонстрация: разбор ошибки");
        context.page = "training-demo";
    });
}
void WelcomeScreen::teacherLogin() {
    clear();
    bool first = !context.school.hasTeacherPassword();
    title = first ? "Первый вход учителя" : "Вход учителя";
    text(first ? "Задайте пароль для редактирования заданий и результатов."
               : "Введите пароль кабинета учителя.",
         0, 0, 22, 900, 2);
    note("Пароль действует на этом компьютере. Регистрация в интернете не нужна.", 0, 75, 900);
    text("Пароль (от 8 символов)", 0, 140, 18);
    TextField *password = field("", 0, 175, 450, 128);
    password->setPassword(true);
    TextField *repeat = nullptr;
    if (first) {
        text("Повторите пароль", 0, 240, 18);
        repeat = field("", 0, 275, 450, 128);
        repeat->setPassword(true);
    }
    button(
        first ? "Сохранить и войти" : "Войти", 0, 350, 300,
        [this, first, password, repeat] {
            std::string value = password->getText();
            if (first) {
                if (value != repeat->getText())
                    throw std::runtime_error("Пароли не совпадают");
                context.school.setTeacherPassword(value);
            }
            if (!context.school.loginTeacher(value))
                throw std::runtime_error("Неверный пароль");
            password->setText("");
            if (repeat)
                repeat->setText("");
            context.selectProfile("", "");
            context.role = UserRole::Teacher;
            context.page = "home";
        },
        true);
    note("Пароль ограничивает действия в приложении, но не доступ к файлу базы данных.", 0, 440,
         900);
    button("← Выбор роли", 0, 540, 230, [this] { refresh(); });
}
void WelcomeScreen::profiles() {
    clear();
    title = "Профиль ученика";
    auto all = context.school.profiles();
    std::vector<std::pair<std::string, std::string>> students;
    std::vector<std::string> names;
    for (const auto &profile : all)
        if (profile.first != "demo-student" && profile.first != "demo-training") {
            students.push_back(profile);
            names.push_back(profile.second);
        }
    text("Выберите свой профиль", 0, 0, 23);
    ListBox *choices = list(names, 0, 45, 440, 280);
    Button *open = button(
        "Войти →", 0, 350, 230,
        [this, students, choices] {
            if (choices->selection() < 0)
                throw std::runtime_error("Выберите профиль в списке");
            const auto &student = students[choices->selection()];
            enter(student.first, student.second);
        },
        true);
    open->setEnabled(!students.empty());
    text(students.empty() ? "Профилей пока нет. Создайте первый справа."
                          : "Здесь только личные профили.",
         0, 413, 16, 440, 2);
    surface(490, 0, 1, 470, Theme::line);
    text("Первый раз здесь?", 530, 0, 23);
    note("Создайте профиль, чтобы сохранять свои результаты отдельно.", 530, 45, 410);
    text("Ваше имя", 530, 120, 16);
    TextField *name = field("", 530, 150, 410, 100);
    name->setPlaceholder("Например, Мария");
    button(
        "Создать профиль и войти", 530, 220, 350,
        [this, name] {
            std::string value = name->getText();
            const std::size_t first = value.find_first_not_of(" \t\r\n");
            if (first == std::string::npos)
                throw std::runtime_error("Введите имя профиля");
            value = value.substr(first, value.find_last_not_of(" \t\r\n") - first + 1);
            enter(context.school.createProfile(value), value);
        },
        true);
    button("← Выбор роли", 0, 540, 230, [this] { refresh(); });
}
} // namespace ui
} // namespace exam
