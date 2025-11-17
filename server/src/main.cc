#include <thread>
#include <chrono>
#include <future>
#include <atomic>
#include <unordered_map>
#include <mutex>
#include <random>
#include <sstream>
#include <iomanip>
#include <queue>
#include <condition_variable>

#include "converter.h"
#include "winit.h"

enum class TaskStatus {
    PENDING,
    PROCESSING,
    COMPLETED,
    FAILED,
    TIMEOUT
};

struct Task {
    std::string key;
    TaskStatus status;
    std::string result;
    std::string error_message;
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point completed_at;
    std::shared_ptr<std::atomic<bool>> cancel_flag;
    nlohmann::json j_aarc;
    nlohmann::json j_config;
    
    Task() : status(TaskStatus::PENDING), cancel_flag(std::make_shared<std::atomic<bool>>(false)) {
        created_at = std::chrono::system_clock::now();
    }
};

std::unordered_map<std::string, std::shared_ptr<Task>> tasks;
std::mutex tasks_mutex;

std::queue<std::shared_ptr<Task>> task_queue;
std::mutex queue_mutex;
std::condition_variable queue_cv;
std::atomic<bool> worker_running(true);

std::string generate_key() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);
    
    std::stringstream ss;
    for (int i = 0; i < 32; i++) {
        ss << std::hex << dis(gen);
    }
    return ss.str();
}

void cleanup_old_tasks() {
    std::lock_guard<std::mutex> lock(tasks_mutex);
    auto now = std::chrono::system_clock::now();
    auto day_ago = now - std::chrono::hours(24);
    
    for (auto it = tasks.begin(); it != tasks.end();) {
        if ((it->second->status == TaskStatus::COMPLETED || 
             it->second->status == TaskStatus::FAILED || 
             it->second->status == TaskStatus::TIMEOUT) &&
            it->second->completed_at < day_ago) {
            
            it = tasks.erase(it);
        } else {
            ++it;
        }
    }
}

void process_task(std::shared_ptr<Task> task) {
    try {
        task->status = TaskStatus::PROCESSING;
        
        geometry::Map map(task->j_aarc, task->j_config);

        std::promise<std::pair<bool, std::string>> promise;
        std::future<std::pair<bool, std::string>> future = promise.get_future();
        
        std::thread converter_thread([map, cancel_flag = task->cancel_flag](std::promise<std::pair<bool, std::string>> p) {
            try {
                rc::Map rcmap = converter::convert_to_rc(map, cancel_flag.get());
                nlohmann::json rc_json = rcmap.to_json();
                p.set_value({true, rc_json.dump()});
            } catch (const std::exception& e) {
                p.set_value({false, std::string(e.what())});
            } catch (...) {
                p.set_value({false, "Unknown conversion error"});
            }
        }, std::move(promise));
        
        if (future.wait_for(std::chrono::seconds(15)) == std::future_status::timeout) {
            task->cancel_flag->store(true);
            
            if (future.wait_for(std::chrono::milliseconds(500)) == std::future_status::ready) {
                converter_thread.join();
            } else {
                converter_thread.detach();
            }
            
            task->status = TaskStatus::TIMEOUT;
            task->error_message = "Conversion took longer than 15 seconds";
            task->completed_at = std::chrono::system_clock::now();
            return;
        }
        
        auto result = future.get();
        converter_thread.join();
        if (result.first) {
            task->status = TaskStatus::COMPLETED;
            task->result = result.second;
        } else {
            task->status = TaskStatus::FAILED;
            task->error_message = result.second;
        }
        task->completed_at = std::chrono::system_clock::now();
        
    } catch (const std::exception& e) {
        task->status = TaskStatus::FAILED;
        task->error_message = std::string("Conversion error: ") + e.what();
        task->completed_at = std::chrono::system_clock::now();
    } catch (...) {
        task->status = TaskStatus::FAILED;
        task->error_message = "Unknown error";
        task->completed_at = std::chrono::system_clock::now();
    }
}

void worker_thread_func() {
    while (worker_running) {
        std::shared_ptr<Task> task;
        
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            queue_cv.wait(lock, []{ return !task_queue.empty() || !worker_running; });
            
            if (!worker_running && task_queue.empty()) {
                break;
            }
            
            if (!task_queue.empty()) {
                task = task_queue.front();
                task_queue.pop();
            }
        }
        
        if (task) {
            process_task(task);
        }
    }
}

void cleanup_daemon() {
    while (true) {
        std::this_thread::sleep_for(std::chrono::hours(1));
        cleanup_old_tasks();
    }
}

int main() {
    crow::App<crow::CORSHandler>* app = winit::app();

    auto with_cors = [](crow::response res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        return res;
    };

    std::thread cleanup_thread(cleanup_daemon);
    cleanup_thread.detach();

    std::thread worker_thread(worker_thread_func);

    CROW_ROUTE((*app), "/create").methods("POST"_method)
    ([&](const crow::request& req) {
        nlohmann::json j_body;
        try {
            j_body = nlohmann::json::parse(req.body);
        } catch (const std::exception& e) {
            return with_cors(crow::response(400, std::string("Invalid JSON: ") + e.what()));
        }
        
        if (!j_body.contains("aarc")) {
            return with_cors(crow::response(400, "Invalid request: missing 'aarc'."));
        }

        // // Output request JSON to most_recent_request.json for debugging asynchronously
        // std::thread([j_body]() {
        //     std::ofstream debug_file("most_recent_request.json");
        //     debug_file << j_body.dump(4);
        // }).detach();

        // Extract aarc and config as nlohmann::json objects
        nlohmann::json j_aarc, j_config;
        try {
            j_aarc = j_body["aarc"];
            if (j_body.contains("config")) {
                j_config = j_body["config"];
                // if j_config is string, parse it as JSON
                if (j_config.is_string()) {
                    j_config = nlohmann::json::parse(j_config.get<std::string>());
                }
            }
            // if j_aarc is string, parse it as JSON
            if (j_aarc.is_string()) {
                j_aarc = nlohmann::json::parse(j_aarc.get<std::string>());
            }
        } catch (const std::exception& e) {
            return with_cors(crow::response(400, std::string("Invalid JSON format: ") + e.what()));
        }

        // Create task
        auto task = std::make_shared<Task>();
        task->key = generate_key();
        task->status = TaskStatus::PENDING;
        task->j_aarc = j_aarc;
        task->j_config = j_config;
        
        {
            std::lock_guard<std::mutex> lock(tasks_mutex);
            tasks[task->key] = task;
        }
        
        {
            std::lock_guard<std::mutex> lock(queue_mutex);
            task_queue.push(task);
        }
        queue_cv.notify_one();
        
        crow::json::wvalue response;
        response["key"] = task->key;
        response["status"] = "pending";
        return with_cors(crow::response(200, response));
    });

    CROW_ROUTE((*app), "/get").methods("POST"_method)
    ([&](const crow::request& req) {
        auto body = crow::json::load(req.body);
        if (!body || !body.has("key")) {
            return with_cors(crow::response(400, "Invalid request: missing 'key'."));
        }
        
        std::string key = body["key"].s();
        
        // Find task
        std::shared_ptr<Task> task;
        {
            std::lock_guard<std::mutex> lock(tasks_mutex);
            auto it = tasks.find(key);
            if (it == tasks.end()) {
                return with_cors(crow::response(404, "Task not found."));
            }
            task = it->second;
        }
        
        // Build response based on status
        crow::json::wvalue response;
        response["key"] = key;
        
        switch (task->status) {
            case TaskStatus::PENDING:
                response["status"] = "pending";
                return with_cors(crow::response(200, response));
                
            case TaskStatus::PROCESSING:
                response["status"] = "processing";
                return with_cors(crow::response(200, response));
                
            case TaskStatus::COMPLETED:
                response["status"] = "completed";
                response["result"] = crow::json::load(task->result);
                return with_cors(crow::response(200, response));
                
            case TaskStatus::FAILED:
                response["status"] = "failed";
                response["error"] = task->error_message;
                return with_cors(crow::response(200, response));
                
            case TaskStatus::TIMEOUT:
                response["status"] = "timeout";
                response["error"] = task->error_message;
                return with_cors(crow::response(200, response));
        }
        
        return with_cors(crow::response(500, "Unknown task status."));
    });

    winit::run(app);
    
    worker_running = false;
    queue_cv.notify_all();
    if (worker_thread.joinable()) {
        worker_thread.join();
    }
    
    return 0;
}