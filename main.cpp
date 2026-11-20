#include <iostream>
#include <string>
#include <vector>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

using namespace std;
using json = nlohmann::json;

struct Message {
    string role;
    string content;
};

vector<Message> history;

size_t WriteCallback(
    void* contents,
    size_t size,
    size_t nmemb,
    string* output
) {
    size_t total = size * nmemb;
    output->append((char*)contents, total);
    return total;
}

string generateAI(string prompt) {
    CURL* curl = curl_easy_init();

    if (!curl) return "Ошибка подключения.";

    history.push_back({"user", prompt});

    json messages = json::array();

    messages.push_back({
        {"role", "system"},
        {"content", "Ты полезный ИИ-ассистент. Отвечай на русском языке."}
    });

    for (auto& msg : history) {
        messages.push_back({
            {"role", msg.role},
            {"content", msg.content}
        });
    }

    json body = {
        {"model", "local-model"},
        {"messages", messages},
        {"temperature", 0.7},
        {"max_tokens", 500}
    };

    string response;

    string request = body.dump();

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(
        headers, "Content-Type: application/json"
    );

    curl_easy_setopt(curl, CURLOPT_URL,
        "http://localhost:8080/v1/chat/completions");

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    CURLcode result = curl_easy_perform(curl);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (result != CURLE_OK) {
        return "Не удалось подключиться к модели.";
    }

    try {
        json data = json::parse(response);

        string answer =
            data["choices"][0]["message"]["content"];

        history.push_back({"assistant", answer});

        return answer;
    }
    catch (...) {
        return "Ошибка обработки ответа модели.";
    }
}

int main() {
    curl_global_init(CURL_GLOBAL_DEFAULT);

    cout << "========================\n";
    cout << "       C++ AI CHAT\n";
    cout << "========================\n";
    cout << "Напиши 'выход' для завершения.\n";

    string input;

    while (true) {
        cout << "\nТы: ";
        getline(cin, input);

        if (input == "выход") break;

        cout << "\nИИ: " << generateAI(input) << "\n";
    }

    curl_global_cleanup();
    return 0;
}
