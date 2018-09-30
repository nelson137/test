#include <stdio.h>
#include <curl/curl.h>

int main(void) {
    CURL *curl;
    CURLcode res;

    curl = curl_easy_init();
    if (curl) {
        char *url = "https://api.github.com/repos/nelson137/test/commits/master";
        // Set the url
        curl_easy_setopt(curl, CURLOPT_URL, url);
        // Follow redirects
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        // Set the user agent
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "MI6 Agent/007");
        // Set the username and password
        curl_easy_setopt(curl, CURLOPT_USERPWD, "nelson137:w580fywx9EeL");

        // Perform the request
        res = curl_easy_perform(curl);
        if (res != CURLE_OK)
            fprintf(stderr, "curl_easy_perform() failed: %s\n",
                    curl_easy_strerror(res));

        curl_easy_cleanup(curl);
    }

    return 0;
}
