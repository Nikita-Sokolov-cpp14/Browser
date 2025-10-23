#pragma once

#include <string>
#include <sstream>
#include <vector>

std::string create_search_page() {
    return R"(
<!DOCTYPE html>
<html>
<head>
    <title>Search Engine</title>
</head>
<body>
    <h1>Search</h1>
    <form method="POST">
        <input type="text" name="query" placeholder="Введите запрос" maxlength="100" required>
        <button type="submit">Search</button>
    </form>
</body>
</html>
)";
}

// std::string create_results_page(const std::vector<SearchResult>& results,
//                                const std::string& query) {
//     std::stringstream html;
//     html << "<!DOCTYPE html><html><head><title>Results for " << query << "</title></head><body>";
//     html << "<h1>Results for: " << query << "</h1>";

//     if (results.empty()) {
//         html << "<p>No results found</p>";
//     } else {
//         html << "<ul>";
//         for (const auto& result : results) {
//             html << "<li><a href=\"" << result.url << "\">" << result.title
//                  << "</a> (relevance: " << result.relevance << ")</li>";
//         }
//         html << "</ul>";
//     }

//     html << "<a href=\"/\">New search</a></body></html>";
//     return html.str();
// }
