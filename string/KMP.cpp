#include <iostream>
#include <string>
#include <vector>

bool KMP(const std::string& text, const std::string& pattern) {
    auto s = pattern;
    s.push_back('#');
    s.insert(s.end(), text.begin(), text.end());

    std::vector<int> pref(s.size());
    pref[0] = 0;
    for (int i = 1; i < pref.size(); ++i) {
        int j = pref[i - 1];
        while (j > 0 && s[j] != s[i]) {
            j = pref[j - 1];
        }
        if (s[j] == s[i]) {
            ++j;
        }

        pref[i] = j;
    }
    for (auto p: pref) {
        if (p == pattern.size()) 
            return true;
    } 
    
    return false;
}
