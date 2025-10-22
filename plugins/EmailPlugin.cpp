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
#include "../src/utils/json.hpp"

using namespace std;
using json = nlohmann::json;

// Global curl initialization tracking
static bool curl_initialized = false;

// Helper function to log messages
static void log_message(const string& msg) {
    ofstream log("../logs/email_plugin.log", ios::app);
    if (log) {
        auto now = chrono::system_clock::now();
        time_t now_time = chrono::system_clock::to_time_t(now);
        log << ctime(&now_time) << msg << endl;
        log.close();
    }
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

        // Get environment variables
        const char* user_env = getenv("SMTP_USER");
        const char* pass_env = getenv("SMTP_PASS");

        string smtp_user = user_env ? user_env : "";
        string smtp_pass = pass_env ? pass_env : "";

        if (smtp_user.empty() || smtp_pass.empty()) {
            cerr << "Error: SMTP_USER and SMTP_PASS environment variables must be set" << endl;
            log_message("Missing SMTP credentials");
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
        string smtp_url = "smtp://smtp.gmail.com:587";

        // Set curl options for SMTP
        curl_easy_setopt(curl, CURLOPT_URL, smtp_url.c_str());
        curl_easy_setopt(curl, CURLOPT_USERNAME, smtp_user.c_str());
        curl_easy_setopt(curl, CURLOPT_PASSWORD, smtp_pass.c_str());
        curl_easy_setopt(curl, CURLOPT_MAIL_FROM, mail_from.c_str());
        curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);

        // Use SMTP protocol
        curl_easy_setopt(curl, CURLOPT_USE_SSL, CURLUSESSL_ALL);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);  // Verify SSL certificate
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);  // Verify host

        // Set upload for email body
        curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
        curl_easy_setopt(curl, CURLOPT_READFUNCTION, email_read_callback);
        curl_easy_setopt(curl, CURLOPT_READDATA, &payload);

        // Set timeout
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);

        // Perform the request
        log_message("Attempting to send email via SMTP");
        CURLcode res = curl_easy_perform(curl);

        // Clean up
        curl_slist_free_all(recipients);
        curl_easy_cleanup(curl);

        // Log result
        if (res != CURLE_OK) {
            log_message(string("CURL error: ") + curl_easy_strerror(res));
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
