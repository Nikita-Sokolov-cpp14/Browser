#pragma once

#include <string>
#include <sstream>
#include <vector>
#include <map>

/**
* @brief Получить строку с HTML кодом стартовой страницы запроса.
* @return Строка с HTML страницей.
*/
inline std::string createSearchPage() {
    return R"(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Search Engine</title>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }

        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            display: flex;
            align-items: center;
            justify-content: center;
            padding: 20px;
        }

        .container {
            background: white;
            border-radius: 12px;
            box-shadow: 0 15px 35px rgba(0, 0, 0, 0.1);
            padding: 40px;
            max-width: 500px;
            width: 100%;
            text-align: center;
        }

        .logo {
            font-size: 2.5rem;
            font-weight: 700;
            color: #333;
            margin-bottom: 10px;
        }

        .tagline {
            color: #666;
            margin-bottom: 30px;
            font-size: 1.1rem;
        }

        .search-form {
            display: flex;
            flex-direction: column;
            gap: 20px;
        }

        .search-input {
            padding: 15px 20px;
            border: 2px solid #e1e5e9;
            border-radius: 8px;
            font-size: 1rem;
            transition: all 0.3s ease;
            outline: none;
        }

        .search-input:focus {
            border-color: #667eea;
            box-shadow: 0 0 0 3px rgba(102, 126, 234, 0.1);
        }

        .search-button {
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            border: none;
            padding: 15px 30px;
            border-radius: 8px;
            font-size: 1rem;
            font-weight: 600;
            cursor: pointer;
            transition: transform 0.2s ease, box-shadow 0.2s ease;
        }

        .search-button:hover {
            transform: translateY(-2px);
            box-shadow: 0 5px 15px rgba(102, 126, 234, 0.4);
        }

        .search-button:active {
            transform: translateY(0);
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="logo">Search</div>
        <p class="tagline">Find what you're looking for</p>
        <form method="POST" class="search-form">
            <input type="text" name="query" placeholder="Enter your search query..." maxlength="100" required class="search-input">
            <button type="submit" class="search-button">Search</button>
        </form>
    </div>
</body>
</html>
)";
}

/**
* @brief Получить строку с HTML кодом страницы с ответами на запрос.
* @param results Результаты поиска в БД.
* @param query Строка с запросом.
* @return Строка с HTML страницей.
*/
inline std::string createResultsPage(
        const std::map<int, std::string, std::greater<int>> &results, const std::string &query) {
    std::stringstream html;

    html << R"(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Results for )" << query << R"(</title>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }

        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: #f8f9fa;
            color: #333;
            line-height: 1.6;
        }

        .header {
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            padding: 30px 0;
            box-shadow: 0 2px 10px rgba(0, 0, 0, 0.1);
        }

        .container {
            max-width: 800px;
            margin: 0 auto;
            padding: 0 20px;
        }

        .logo {
            font-size: 1.8rem;
            font-weight: 700;
            margin-bottom: 10px;
        }

        .search-query {
            font-size: 1.2rem;
            opacity: 0.9;
            margin-bottom: 20px;
        }

        .results-count {
            color: #666;
            margin-bottom: 25px;
            font-size: 1rem;
        }

        .results-list {
            background: white;
            border-radius: 8px;
            box-shadow: 0 2px 10px rgba(0, 0, 0, 0.05);
            overflow: hidden;
            margin-bottom: 30px;
        }

        .result-item {
            padding: 20px;
            border-bottom: 1px solid #eee;
            transition: background-color 0.2s ease;
        }

        .result-item:hover {
            background-color: #f8f9fa;
        }

        .result-item:last-child {
            border-bottom: none;
        }

        .result-link {
            color: #667eea;
            text-decoration: none;
            font-size: 1.1rem;
            font-weight: 500;
            display: block;
            margin-bottom: 5px;
        }

        .result-link:hover {
            color: #764ba2;
            text-decoration: underline;
        }

        .result-score {
            color: #28a745;
            font-size: 0.9rem;
            font-weight: 600;
        }

        .no-results {
            text-align: center;
            padding: 60px 20px;
            color: #666;
        }

        .no-results-icon {
            font-size: 3rem;
            margin-bottom: 20px;
            opacity: 0.5;
        }

        .back-button {
            display: inline-flex;
            align-items: center;
            background: #667eea;
            color: white;
            text-decoration: none;
            padding: 12px 24px;
            border-radius: 6px;
            font-weight: 600;
            transition: all 0.2s ease;
        }

        .back-button:hover {
            background: #5a6fd8;
            transform: translateY(-1px);
            box-shadow: 0 4px 12px rgba(102, 126, 234, 0.3);
        }

        .footer {
            text-align: center;
            margin-top: 40px;
        }
    </style>
</head>
<body>
    <div class="header">
        <div class="container">
            <div class="logo">Search Results</div>
            <div class="search-query">)" << query << R"(</div>
        </div>
    </div>

    <div class="container" style="margin-top: 30px;">
        <div class="results-count">)";

    if (results.empty()) {
        html << "No results found";
    } else {
        html << "Found " << results.size() << " result" << (results.size() != 1 ? "s" : "");
    }

    html << R"(</div>)";

    if (results.empty()) {
        html << R"(
        <div class="no-results">
            <div class="no-results-icon">🔍</div>
            <h3>No results found</h3>
            <p>Try different keywords or check your spelling</p>
        </div>)";
    } else {
        html << R"(
        <div class="results-list">)";

        for (const auto &result : results) {
            html << R"(
            <div class="result-item">
                <a href=")" << result.second << R"(" class="result-link" target="_blank">)"
                   << result.second << R"(</a>
                <div class="result-score">Relevance score: )" << result.first << R"(</div>
            </div>)";
        }

        html << R"(
        </div>)";
    }

    html << R"(
        <div class="footer">
            <a href="/" class="back-button">New Search</a>
        </div>
    </div>
</body>
</html>)";

    return html.str();
}

/**
* @brief Получить строку с HTML кодом страницы с ошибкой.
* @param error Ошибка.
* @return Строка с HTML страницей.
*/
inline std::string createErrorPage(const std::string &error) {
    return R"(<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Error</title>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }

        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: linear-gradient(135deg, #ff6b6b 0%, #ee5a24 100%);
            min-height: 100vh;
            display: flex;
            align-items: center;
            justify-content: center;
            padding: 20px;
        }

        .container {
            background: white;
            border-radius: 12px;
            box-shadow: 0 15px 35px rgba(0, 0, 0, 0.1);
            padding: 40px;
            max-width: 500px;
            width: 100%;
            text-align: center;
        }

        .error-icon {
            font-size: 4rem;
            margin-bottom: 20px;
        }

        h1 {
            color: #d63031;
            margin-bottom: 20px;
            font-size: 1.8rem;
        }

        .error-message {
            background: #ffeaa7;
            color: #2d3436;
            padding: 20px;
            border-radius: 8px;
            border-left: 4px solid #fdcb6e;
            margin-bottom: 30px;
            text-align: left;
            font-family: 'Courier New', monospace;
            font-size: 0.9rem;
            word-break: break-word;
        }

        .back-button {
            display: inline-flex;
            align-items: center;
            background: #667eea;
            color: white;
            text-decoration: none;
            padding: 12px 24px;
            border-radius: 6px;
            font-weight: 600;
            transition: all 0.2s ease;
        }

        .back-button:hover {
            background: #5a6fd8;
            transform: translateY(-1px);
            box-shadow: 0 4px 12px rgba(102, 126, 234, 0.3);
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="error-icon">⚠️</div>
        <h1>Something went wrong</h1>
        <div class="error-message">)" + error + R"(</div>
        <a href="/" class="back-button">Back to Search</a>
    </div>
</body>
</html>)";
}
