/**
 * MANAGER.CPP
 * Purpose: Logic layer for high-level commands like Add, Status, and Init.
 * Acts as the bridge between the CLI and the Core storage.
 */

#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <vector>
#include <algorithm>
#include "core.cpp"

using namespace std;
namespace fs = std::filesystem;

// Terminal Colors
#define RED "\x1B[31m"
#define GRN "\x1B[32m"
#define YEL "\x1B[33m"
#define END "\033[0m"

// =============================================================================
// FILE COMPARISON UTILITY
// =============================================================================

/**
 * Performs a binary comparison between two files to check if they are identical.
 */
bool filesAreSame(const fs::path &a, const fs::path &b) {
    if (!fs::exists(a) || !fs::exists(b)) return false;
    if (fs::file_size(a) != fs::file_size(b)) return false;

    ifstream fa(a, ios::binary);
    ifstream fb(b, ios::binary);

    return equal(
        istreambuf_iterator<char>(fa),
        istreambuf_iterator<char>(),
        istreambuf_iterator<char>(fb)
    );
}

// =============================================================================
// GIT CLASS DEFINITION
// =============================================================================

class gitClass {
public:
    commitNodeList list;

    // Core Commands
    void gitInit();
    void gitAdd();                          // git add .
    void gitAdd(string files[], int n);     // git add file1 file2
    bool gitCommit(string msg);
    bool gitRevert(string commitHash);
    void gitLog();
    void gitStatus();

private:
    void clearStagingArea();
    
    /**
     * Helper to check if a path should be ignored by the VCS.
     */
    bool isIgnored(const fs::path& rel) {
        string s = rel.string();
        if (s.empty()) return true;
        if (s.find(".git") == 0) return true;
        if (s.find(".vscode") == 0) return true;
        if (rel.filename() == "mygit.exe" || rel.filename() == "mygit") return true;
        return false;
    }

    /**
     * Reads and cleans the current HEAD hash.
     */
    string getHEAD() {
        ifstream file(".git/HEAD");
        string head;
        if (!getline(file, head)) return "NULL";
        head.erase(remove_if(head.begin(), head.end(), ::isspace), head.end());
        return head.empty() ? "NULL" : head;
    }
};

// =============================================================================
// COMMAND IMPLEMENTATIONS
// =============================================================================

void gitClass::gitInit() {
    try {
        fs::create_directories(".git/staging_area");
        fs::create_directories(".git/commits");
        
        ofstream headFile(".git/HEAD");
        headFile << "NULL";
        headFile.close();
        
        cout << GRN << "Initialized empty Git repository." << END << endl;
    } catch (const fs::filesystem_error& e) {
        cerr << RED << "Init failed: " << e.what() << END << endl;
    }
}

void gitClass::gitAdd() {
    fs::path root = fs::current_path();
    fs::path staging = root / ".git" / "staging_area";
    string head = getHEAD();
    fs::path committedData = (head != "NULL") ? root / ".git" / "commits" / head / "Data" : fs::path();

    for (fs::recursive_directory_iterator it(root); it != fs::recursive_directory_iterator(); ++it) {
        fs::path rel = fs::relative(it->path(), root);

        if (isIgnored(rel)) {
            it.disable_recursion_pending();
            continue;
        }

        if (!fs::is_regular_file(*it)) continue;

        fs::path stagedFile = staging / rel;
        fs::path committedFile = (!committedData.empty()) ? committedData / rel : fs::path();

        // Skip if the file matches the last commit exactly
        if (fs::exists(committedFile) && filesAreSame(it->path(), committedFile)) {
            // If it was in staging but now matches the commit, remove from staging
            if (fs::exists(stagedFile)) fs::remove(stagedFile);
            continue;
        }

        fs::create_directories(stagedFile.parent_path());
        fs::copy_file(it->path(), stagedFile, fs::copy_options::overwrite_existing);
    }
}

void gitClass::gitAdd(string files[], int n) {
    fs::path root = fs::current_path();
    fs::path staging = root / ".git" / "staging_area";
    string head = getHEAD();
    fs::path committedData = (head != "NULL") ? root / ".git" / "commits" / head / "Data" : fs::path();

    for (int i = 0; i < n; i++) {
        fs::path src = root / files[i];
        if (isIgnored(fs::relative(src, root))) continue;

        if (!fs::exists(src) || !fs::is_regular_file(src)) {
            cout << YEL << "Warning: " << files[i] << " does not exist or is not a file." << END << endl;
            continue;
        }

        fs::path rel = fs::relative(src, root);
        fs::path stagedFile = staging / rel;
        fs::path committedFile = (!committedData.empty()) ? committedData / rel : fs::path();

        if (fs::exists(committedFile) && filesAreSame(src, committedFile)) continue;

        fs::create_directories(stagedFile.parent_path());
        fs::copy_file(src, stagedFile, fs::copy_options::overwrite_existing);
    }
}

bool gitClass::gitCommit(string msg) {
    fs::path staging = fs::current_path() / ".git" / "staging_area";
    bool empty = true;

    if (fs::exists(staging)) {
        for (const auto& e : fs::recursive_directory_iterator(staging)) {
            if (fs::is_regular_file(e)) { empty = false; break; }
        }
    }

    if (empty) {
        cout << "Nothing to commit, staging area is empty." << endl;
        return false;
    }

    list.addOnTail(msg);
    clearStagingArea();
    cout << GRN << "Files committed successfully." << END << endl;
    return true;
}

void gitClass::clearStagingArea() {
    fs::path staging = fs::current_path() / ".git" / "staging_area";
    if (fs::exists(staging)) {
        fs::remove_all(staging);
        fs::create_directory(staging);
    }
}

void gitClass::gitStatus() {
    fs::path root = fs::current_path();
    fs::path staging = root / ".git" / "staging_area";
    string head = getHEAD();
    fs::path committedData = (head != "NULL") ? root / ".git" / "commits" / head / "Data" : fs::path();

    vector<string> staged, modified, untracked;

    // 1. Scan Staging Area
    if (fs::exists(staging)) {
        for (const auto& e : fs::recursive_directory_iterator(staging)) {
            if (fs::is_regular_file(e)) staged.push_back(fs::relative(e.path(), staging).string());
        }
    }

    // 2. Scan Working Directory
    for (fs::recursive_directory_iterator it(root); it != fs::recursive_directory_iterator(); ++it) {
        fs::path rel = fs::relative(it->path(), root);
        if (isIgnored(rel) || !fs::is_regular_file(*it)) {
            if (isIgnored(rel)) it.disable_recursion_pending();
            continue;
        }

        bool inStaging = fs::exists(staging / rel);
        bool inCommit = (!committedData.empty()) && fs::exists(committedData / rel);

        if (inCommit && !inStaging) {
            if (!filesAreSame(it->path(), committedData / rel)) modified.push_back(rel.string());
        } else if (!inCommit && !inStaging) {
            untracked.push_back(rel.string());
        }
    }

    // 3. Display Results
    if (!staged.empty()) {
        cout << GRN << "Changes to be committed:" << END << endl;
        for (const auto& s : staged) cout << "  " << s << endl;
    }
    if (!modified.empty()) {
        cout << YEL << "\nChanges not staged for commit:" << END << endl;
        for (const auto& m : modified) cout << "  " << m << endl;
    }
    if (!untracked.empty()) {
        cout << RED << "\nUntracked files:" << END << endl;
        for (const auto& u : untracked) cout << "  " << u << endl;
    }
    if (staged.empty() && modified.empty() && untracked.empty()) {
        cout << "Nothing to commit, working tree clean." << endl;
    }
}

// Pass-throughs to Core
bool gitClass::gitRevert(string hash) { return list.revertCommit(hash); }
void gitClass::gitLog() { list.printCommitList(); }