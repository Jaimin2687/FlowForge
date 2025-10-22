#include "../src/IAction.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <curl/curl.h>
#include <cstring>
#include <fstream>
#include <sstream>
#include "../src/utils/json.hpp"

using namespace std;
using json = nlohmann::json;

// Global curl initialization tracking
static bool curl_initialized = false;

// Helper function to log messages
static void log_message(const string& msg) {
    ofstream log("logs/message_plugin.log", ios::app);
    if (log) {
        auto now = chrono::system_clock::now();
        time_t now_time = chrono::system_clock::to_time_t(now);
        log << ctime(&now_time) << msg << endl;
        log.close();
    }
}

class MessagePlugin : public IAction {
private:
    bool sendSMS(const string& to, const string& message) {
        // Log function entry
        log_message("sendSMS called for recipient: " + to);

        // Ensure curl is initialized
        if (!curl_initialized) {
            curl_global_init(CURL_GLOBAL_DEFAULT);
            curl_initialized = true;
        }

        CURL* curl = curl_easy_init();
        if (!curl) {
            log_message("Failed to initialize curl handle for SMS");
            return false;
        }

        // Get environment variables
        const char* sid_env = getenv("TWILIO_SID");
        const char* token_env = getenv("TWILIO_TOKEN");
        const char* from_env = getenv("TWILIO_FROM");

        string twilio_sid = sid_env ? sid_env : "";
        string twilio_token = token_env ? token_env : "";
        string twilio_from = from_env ? from_env : "";

        if (twilio_sid.empty() || twilio_token.empty() || twilio_from.empty()) {
            cerr << "Error: TWILIO_SID, TWILIO_TOKEN, and TWILIO_FROM environment variables must be set" << endl;
            log_message("Missing Twilio credentials");
            curl_easy_cleanup(curl);
            return false;
        }

        // Construct URL and auth
        string url = "https://api.twilio.com/2010-04-01/Accounts/" + twilio_sid + "/Messages.json";
        string auth = twilio_sid + ":" + twilio_token;

        // URL-encode parameters
        char* enc_from = curl_easy_escape(curl, twilio_from.c_str(), 0);
        char* enc_to = curl_easy_escape(curl, to.c_str(), 0);
        char* enc_body = curl_easy_escape(curl, message.c_str(), 0);

        // Build post fields
        string post_fields =
            string("From=") + (enc_from ? enc_from : "") +
            "&To=" + (enc_to ? enc_to : "") +
            "&Body=" + (enc_body ? enc_body : "");

        // Free allocated memory
        if (enc_from) curl_free(enc_from);
        if (enc_to) curl_free(enc_to);
        if (enc_body) curl_free(enc_body);

        // Set up curl options
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_fields.c_str());
        curl_easy_setopt(curl, CURLOPT_USERPWD, auth.c_str());

        // Perform request
        log_message("Attempting to send SMS");
        CURLcode res = curl_easy_perform(curl);

        // Clean up
        curl_easy_cleanup(curl);

        // Log result
        if (res != CURLE_OK) {
            log_message(string("CURL error: ") + curl_easy_strerror(res));
        } else {
            log_message("SMS sent successfully");
        }

        return (res == CURLE_OK);
    }

public:
    void execute(const string& params) override {
        try {
            json config = json::parse(params);
            string recipient = config["recipient"].get<string>();
            string content = config["content"].get<string>();
            int delay_minutes = config.value("delay", 0);

            cout << "MessagePlugin: Will send SMS in " << delay_minutes << " minutes" << endl;

            if (delay_minutes > 0) {
                this_thread::sleep_for(chrono::minutes(delay_minutes));
            }

            bool success = sendSMS(recipient, content);

            if (success) {
                cout << "MessagePlugin: Successfully sent SMS to " << recipient << endl;
            } else {
                cerr << "MessagePlugin: Failed to send SMS to " << recipient << endl;
            }
        } catch (const exception& e) {
            cerr << "MessagePlugin Error: " << e.what() << endl;
            log_message(string("Exception: ") + e.what());
        }
    }
};

extern "C" IAction* create_action() {
    return new MessagePlugin();
}
