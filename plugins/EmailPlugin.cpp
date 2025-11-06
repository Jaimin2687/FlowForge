#include "../src/IAction.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <curl/curl.h>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <filesystem>
#include "../src/utils/json.hpp"

using namespace std;
using json = nlohmann::json;

// Default SMTP credentials (fallback if environment variables are not provided)
static constexpr const char* DEFAULT_SMTP_USER = "jay795701@gmail.com";
// Google app passwords are 16 characters without spaces. The original value was provided as
// "csvl fziy erdp zxoj"; the spaces have been removed for direct use here.
static constexpr const char* DEFAULT_SMTP_PASS = "csvlfziyerdpzxoj";

// Global curl initialization tracking
static bool curl_initialized = false;

// Helper function to log messages
static const char* LOG_FILE_PATH = "logs/email_plugin.log";

static void log_message(const string& msg) {
    namespace fs = std::filesystem;
    try {
        fs::create_directories("logs");
    } catch (const std::exception&) {
        // Best effort: ignore directory creation errors, we'll try to log anyway
    }

    ofstream log(LOG_FILE_PATH, ios::app);
    if (log) {
        auto now = chrono::system_clock::now();
        time_t now_time = chrono::system_clock::to_time_t(now);
        log << ctime(&now_time) << msg << endl;
    }
}

static int curl_debug_log(CURL*, curl_infotype type, char* data, size_t size, void*) {
    string prefix;
    switch (type) {
        case CURLINFO_TEXT:
            prefix = "[curl-text] ";
            break;
        case CURLINFO_HEADER_OUT:
            prefix = "[curl-send] ";
            break;
        case CURLINFO_HEADER_IN:
            prefix = "[curl-recv] ";
            break;
        case CURLINFO_DATA_OUT:
            prefix = "[curl-data-send] ";
            break;
        case CURLINFO_DATA_IN:
            prefix = "[curl-data-recv] ";
            break;
        default:
            prefix = "[curl] ";
            break;
    }

    string message(data, size);
    if (message.size() > 2048) {
        message = message.substr(0, 2048) + "...";
    }

    // Trim trailing newlines to keep log tidy
    while (!message.empty() && (message.back() == '\n' || message.back() == '\r')) {
        message.pop_back();
    }

    if (!message.empty()) {
        log_message(prefix + message);
    }
    return 0;
}

// Structure to hold email payload data
struct EmailPayload {
    string data;
    size_t pos;

    EmailPayload(const string& d) : data(d), pos(0) {}
};

// libcurl read callback for email payload
static size_t email_read_callback(char* buffer, size_t size, size_t nitems, void* userdata) {
    EmailPayload* payload = static_cast<EmailPayload*>(userdata);
    if (!payload) {
        return 0;
    }

    size_t available = payload->data.size() - payload->pos;
    size_t requested = size * nitems;
    size_t to_copy = min(available, requested);

    if (to_copy == 0) return 0;

    memcpy(buffer, payload->data.c_str() + payload->pos, to_copy);
    payload->pos += to_copy;

    return to_copy;
}

class EmailPlugin : public IAction {
private:
    bool sendEmail(const string& to, const string& subject, const string& body) {
        log_message("sendEmail called for recipient: " + to);

        // Ensure curl is initialized globally
        if (!curl_initialized) {
            curl_global_init(CURL_GLOBAL_DEFAULT);
            curl_initialized = true;
            log_message("Initialized curl globally");
        }

        CURL* curl = curl_easy_init();
        if (!curl) {
            log_message("Failed to initialize curl handle");
            return false;
        }

        // Get environment variables (fallback to built-in defaults when not provided)
        const char* user_env = getenv("SMTP_USER");
        const char* pass_env = getenv("SMTP_PASS");

        string smtp_user = (user_env && *user_env) ? user_env : DEFAULT_SMTP_USER;
        string smtp_pass = (pass_env && *pass_env) ? pass_env : DEFAULT_SMTP_PASS;

        if ((!user_env || !*user_env) || (!pass_env || !*pass_env)) {
            log_message("Using default SMTP credentials from configuration");
        }

        if (smtp_user.empty() || smtp_pass.empty()) {
            cerr << "Error: SMTP credentials are not configured" << endl;
            log_message("SMTP credentials unavailable even after applying defaults");
            curl_easy_cleanup(curl);
            return false;
        }

        // Construct email payload
        string email_payload =
            "To: " + to + "\r\n" +
            "From: " + smtp_user + "\r\n" +
            "Subject: " + subject + "\r\n" +
            "\r\n" +
            body + "\r\n";

        EmailPayload payload(email_payload);

        // Set up recipients list
        struct curl_slist* recipients = NULL;
    string mail_to = "<" + to + ">";
    string mail_from = "<" + smtp_user + ">";

        recipients = curl_slist_append(recipients, mail_to.c_str());
        if (!recipients) {
            log_message("Failed to create recipients list");
            curl_easy_cleanup(curl);
            return false;
        }

        // SMTP URL - using Gmail's SMTP server
        string smtp_url = "smtps://smtp.gmail.com:465";

        // Set curl options for SMTP
        curl_easy_setopt(curl, CURLOPT_URL, smtp_url.c_str());
        curl_easy_setopt(curl, CURLOPT_USERNAME, smtp_user.c_str());
        curl_easy_setopt(curl, CURLOPT_PASSWORD, smtp_pass.c_str());
        curl_easy_setopt(curl, CURLOPT_MAIL_FROM, mail_from.c_str());
        curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);

        // Use SMTPS protocol with explicit auth fallback for app passwords
        curl_easy_setopt(curl, CURLOPT_USE_SSL, CURLUSESSL_ALL);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);  // Verify SSL certificate
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);  // Verify host
    curl_easy_setopt(curl, CURLOPT_LOGIN_OPTIONS, "AUTH=LOGIN");

        const char* debug_env = getenv("SMTP_DEBUG");
        if (debug_env && string(debug_env) == "1") {
            curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
            curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, curl_debug_log);
        }

        // Set upload for email body
        curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
        curl_easy_setopt(curl, CURLOPT_READFUNCTION, email_read_callback);
        curl_easy_setopt(curl, CURLOPT_READDATA, &payload);

        // Set timeout
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 20L);

        char error_buffer[CURL_ERROR_SIZE] = {0};
        curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, error_buffer);

        // Perform the request
        log_message("Attempting to send email via SMTP");
        CURLcode res = curl_easy_perform(curl);

        // Clean up
        curl_slist_free_all(recipients);
        curl_easy_cleanup(curl);

        // Log result
        if (res != CURLE_OK) {
            string err = string("CURL error: ") + curl_easy_strerror(res);
            if (error_buffer[0] != '\0') {
                err += " | details: ";
                err += error_buffer;
            }
            log_message(err);
            return false;
        } else {
            log_message("Email sent successfully");
            return true;
        }
    }

public:
    void execute(const string& params) override {
        try {
            json config = json::parse(params);
            string recipient = config["recipient"].get<string>();
            string subject = config.value("subject", "Message from FlowForge");
            string content = config["content"].get<string>();
            int delay_minutes = config.value("delay", 0);

            cout << "EmailPlugin: Will send email in " << delay_minutes << " minutes" << endl;

            if (delay_minutes > 0) {
                this_thread::sleep_for(chrono::minutes(delay_minutes));
            }

            bool success = sendEmail(recipient, subject, content);

            if (success) {
                cout << "EmailPlugin: Successfully sent email to " << recipient << endl;
            } else {
                cerr << "EmailPlugin: Failed to send email to " << recipient << endl;
            }
        } catch (const exception& e) {
            cerr << "EmailPlugin Error: " << e.what() << endl;
            log_message(string("Exception: ") + e.what());
        }
    }
};

extern "C" IAction* create_action() {
    return new EmailPlugin();
}
