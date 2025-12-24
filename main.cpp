#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <algorithm>

using namespace std;

#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define CYAN    "\033[36m"
#define BOLD    "\033[1m"

enum Priority { LOW, MEDIUM, HIGH };

struct Date {
    int d, m, y;
    bool exists = false;

    string toString() const {
        if (!exists) return "-";
        char buf[11];
        sprintf(buf, "%02d.%02d.%04d", d, m, y);
        return string(buf);
    }
};

struct Task {
    int id;
    string description;
    bool completed;
    Date created;
    Date deadline;
    Priority priority;
};

//Вспомогательные функции
Date getCurrentDate() {
    time_t t = time(0);
    tm* now = localtime(&t);
    return {now->tm_mday, now->tm_mon + 1, now->tm_year + 1800 + 1900 - 1800, true}; 
}

bool isExpired(Date deadline) {
    if (!deadline.exists) return false;
    Date now = getCurrentDate();
    if (deadline.y < now.y) return true;
    if (deadline.y == now.y && deadline.m < now.m) return true;
    if (deadline.y == now.y && deadline.m == now.m && deadline.d < now.d) return true;
    return false;
}

Date parseDate(string s) {
    if (s == "-" || s.empty()) return {0, 0, 0, false};
    Date dt;
    if (sscanf(s.c_str(), "%d.%d.%d", &dt.d, &dt.m, &dt.y) == 3) {
        dt.exists = true;
        return dt;
    }
    return {0, 0, 0, false};
}

string priorityToStr(Priority p) {
    if (p == LOW) return "Низкий";
    if (p == MEDIUM) return "Средний";
    return "Высокий";
}

//Класс управления списком
class TodoManager {
private:
    vector<Task> tasks;
    int nextId = 1;
    const string filename = "todo_data.txt";

    void save() {
        ofstream f(filename);
        for (const auto& t : tasks) {
            f << t.id << "|" << t.description << "|" << t.completed << "|"
              << t.created.toString() << "|" << t.deadline.toString() << "|" 
              << (int)t.priority << "\n";
        }
    }

public:
    TodoManager() {
        ifstream f(filename);
        string line;
        while (getline(f, line)) {
            stringstream ss(line);
            string part;
            Task t;
            getline(ss, part, '|'); t.id = stoi(part);
            getline(ss, part, '|'); t.description = part;
            getline(ss, part, '|'); t.completed = stoi(part);
            getline(ss, part, '|'); t.created = parseDate(part);
            getline(ss, part, '|'); t.deadline = parseDate(part);
            getline(ss, part, '|'); t.priority = (Priority)stoi(part);
            tasks.push_back(t);
            if (t.id >= nextId) nextId = t.id + 1;
        }
    }

    void addTask(string desc, Date dl, Priority p) {
        tasks.push_back({nextId++, desc, false, getCurrentDate(), dl, p});
        save();
    }

    void deleteTask(int id) {
        auto it = remove_if(tasks.begin(), tasks.end(), [id](Task& t) { return t.id == id; });
        if (it != tasks.end()) {
            tasks.erase(it, tasks.end());
            save();
            cout << GREEN << "Задача удалена." << RESET << endl;
        } else cout << RED << "ID не найден." << RESET << endl;
    }

    void toggleTask(int id, bool status) {
        for (auto& t : tasks) {
            if (t.id == id) {
                t.completed = status;
                save();
                return;
            }
        }
    }

    void showTasks(int filter = 0, string query = "", int priorityFilter = -1) {
        cout << BOLD << "┌───┬──────────────────────────┬────────────┬────────────┬───────────┐" << RESET << endl;
        cout << BOLD << "│ID │ Задача                   │ Статус     │ Дедлайн    │ Приоритет │" << RESET << endl;
        cout << BOLD << "├───┼──────────────────────────┼────────────┼────────────┼───────────┤" << RESET << endl;
        for (const auto& t : tasks) {
            bool expired = isExpired(t.deadline) && !t.completed;
            
            // Фильтрация
            if (filter == 1 && t.completed) continue;
            if (filter == 2 && !t.completed) continue;
            if (filter == 3 && !expired) continue;
            if (priorityFilter != -1 && (int)t.priority != priorityFilter) continue;
            if (!query.empty() && t.description.find(query) == string::npos) continue;

            string status = t.completed ? "[✓]" : "[ ]";
            string pStr = priorityToStr(t.priority);
            string dStr = t.deadline.toString();

            printf("│%-3d│ %-25.25s│ %-11s│ %-11s│ %-10s│\n", 
            t.id, t.description.c_str(), 
            (t.completed ? GREEN : (expired ? RED : RESET)) + status + RESET,
            dStr.c_str(), pStr.c_str());
        }
        cout << BOLD << "└───┴──────────────────────────┴────────────┴────────────┴───────────┘" << RESET << endl;
    }

    void showStats() {
        int comp = 0, exp = 0;
        for (const auto& t : tasks) {
            if (t.completed) comp++;
            if (isExpired(t.deadline) && !t.completed) exp++;
        }
        cout << CYAN << "Всего: " << tasks.size() << " | Выполнено: " << comp 
             << " | Активно: " << tasks.size() - comp << " | Просрочено: " << exp << RESET << endl;
    }
};

//Интерфейс пользователя
void clearInput() {
    cin.clear();
    cin.ignore(10000, '\n');
}

int main() {
    setlocale(LC_ALL, "Russian");
    TodoManager manager;
    int choice;

    while (true) {
        cout << BOLD << "\n--- МЕНЮ TODO LIST ---" << RESET << endl;
        cout << "1. Показать все задачи\n2. Добавить\n3. Выполнить/Вернуть\n4. Удалить\n5. Фильтры/Поиск\n6. Статистика\n0. Выход\n> ";
        cin >> choice;
        if (cin.fail()) { clearInput(); continue; }

        if (choice == 1) manager.showTasks();
        else if (choice == 2) {
            string desc, dStr; int p;
            cout << "Описание: "; clearInput(); getline(cin, desc);
            cout << "Дедлайн (DD.MM.YYYY или -): "; cin >> dStr;
            cout << "Приоритет (0-Низкий, 1-Средний, 2-Высокий): "; cin >> p;
            manager.addTask(desc, parseDate(dStr), (Priority)p);
        }
        else if (choice == 3) {
            int id, s;
            cout << "ID задачи: "; cin >> id;
            cout << "1 - Выполнено, 0 - Активно: "; cin >> s;
            manager.toggleTask(id, s == 1);
        }
        else if (choice == 4) {
            int id; char confirm;
            cout << "ID для удаления: "; cin >> id;
            cout << "Уверены? (y/n): "; cin >> confirm;
            if (confirm == 'y') manager.deleteTask(id);
        }
        else if (choice == 5) {
            cout << "1. Активные\n2. Выполненные\n3. Просроченные\n4. Поиск по тексту\n> ";
            int f; cin >> f;
            if (f == 4) {
                string q; cout << "Текст: "; cin >> q;
                manager.showTasks(0, q);
            } else manager.showTasks(f);
        }
        else if (choice == 6) manager.showStats();
        else if (choice == 0) break;
    }

    return 0;
}
