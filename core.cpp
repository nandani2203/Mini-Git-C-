/**
 * CORE.CPP
 * Purpose: Handles the low-level VCS operations including Commit creation,
 * inheritance of file snapshots, and historical log traversal.
 */

#include <iostream>
#include <fstream>
#include <filesystem>
#include <ctime>
#include <vector>
#include <string>
#include <algorithm>
#include <unistd.h>

// Terminal Colors
#define RED "\x1B[31m"
#define END "\033[0m"

using namespace std;
namespace fs = std::filesystem;

// =============================================================================
// UTILITY FUNCTIONS
// =============================================================================

/**
 * Generates a random alphanumeric string of a given length.
 * Used for creating unique Commit IDs.
 */
string gen_random(int len) {
    static const char alphanum[] =
        "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

    static bool seeded = false;
    if (!seeded) {
        srand(time(NULL) ^ getpid());
        seeded = true;
    }

    string s;
    for (int i = 0; i < len; i++)
        s += alphanum[rand() % (sizeof(alphanum) - 1)];

    return s;
}

/**
 * Returns current system time in YYYY/MM/DD HH:MM format.
 */
string get_time() {
    time_t t = time(nullptr);
    tm *now = localtime(&t);
    char buf[20];
    strftime(buf, sizeof(buf), "%Y/%m/%d %H:%M", now);
    return string(buf);
}

/**
 * Helper to trim whitespace and newline characters from strings.
 * Critical for cleaning up IDs read from files.
 */
string trim(string s) {
    if (s.empty()) return s;
    s.erase(0, s.find_first_not_of(" \n\r\t"));
    size_t last = s.find_last_not_of(" \n\r\t");
    if (last != string::npos) s.erase(last + 1);
    return s;
}

// =============================================================================
// COMMIT NODE CLASS
// Represents a single point in history.
// =============================================================================

class commitNode {
private:
    string commitID;
    string parentCommitID;
    string commitMsg;

public:
    commitNode(string id, string parent, string msg)
        : commitID(id), parentCommitID(parent), commitMsg(msg) {
        createCommit();
    }

    /**
     * Physically creates the commit directory and handles file snapshots.
     */
    void createCommit() {
        try {
            fs::path commitsRoot = fs::current_path() / ".git" / "commits";
            fs::path commitPath = commitsRoot / commitID;
            fs::path dataPath = commitPath / "Data";

            fs::create_directories(dataPath);

            // 1. INHERIT: Copy files from the parent commit (Snapshotting)
            if (!parentCommitID.empty()) {
                fs::path parentData = commitsRoot / parentCommitID / "Data";
                if (fs::exists(parentData)) {
                    for (const auto &e : fs::recursive_directory_iterator(parentData)) {
                        if (fs::is_regular_file(e.path())) {
                            fs::path rel = fs::relative(e.path(), parentData);
                            fs::path dst = dataPath / rel;
                            fs::create_directories(dst.parent_path());
                            
                            // Ensure clean copy
                            if (fs::exists(dst)) fs::remove(dst);
                            fs::copy_file(e.path(), dst);
                        }
                    }
                }
            }

            // 2. OVERLAY: Apply new changes from the staging area
            fs::path staging = fs::current_path() / ".git" / "staging_area";
            if (fs::exists(staging)) {
                for (const auto &e : fs::recursive_directory_iterator(staging)) {
                    if (fs::is_regular_file(e.path())) {
                        fs::path rel = fs::relative(e.path(), staging);
                        fs::path dst = dataPath / rel;
                        fs::create_directories(dst.parent_path());

                        if (fs::exists(dst)) fs::remove(dst);
                        fs::copy_file(e.path(), dst);
                    }
                }
            }

            // 3. METADATA: Save commit details
            ofstream info(commitPath / "commitInfo.txt");
            if (!info.is_open()) throw runtime_error("File Access Error");
            
            info << "1." << commitID << "\n";
            info << "2." << (parentCommitID.empty() ? "NULL" : parentCommitID) << "\n";
            info << "3." << commitMsg << "\n";
            info << "4." << get_time() << "\n";
            info.close();

        } catch (const fs::filesystem_error& ex) {
            cerr << RED << "FS Error: " << END << ex.what() << endl;
            exit(1); 
        } catch (const exception& ex) {
            cerr << RED << "Error: " << END << ex.what() << endl;
            exit(1);
        }
    }
};

// =============================================================================
// COMMIT LIST CLASS
// Manages the chain of commits and navigation.
// =============================================================================

class commitNodeList {
public:
    /**
     * Entry point for a new commit. Determines parent and updates HEAD.
     */
    void addOnTail(string msg) {
        string parentID;
        ifstream headIn(".git/HEAD");
        getline(headIn, parentID);
        headIn.close();

        parentID = trim(parentID);
        if (parentID == "NULL") parentID = "";

        string newCommitID = gen_random(8);
        commitNode newCommit(newCommitID, parentID, msg);

        ofstream headOut(".git/HEAD", ios::trunc);
        headOut << newCommitID;
        headOut.close();
    }

    /**
     * Creates a duplicate of an existing commit as a new "Revert" commit.
     */
    bool revertCommit(string commitHash) {
        string targetHash = commitHash;

        // Resolve "HEAD" to actual hash
        if (commitHash == "HEAD") {
            ifstream headFile(".git/HEAD");
            if (!headFile.is_open()) return false;
            getline(headFile, targetHash);
            headFile.close();
            targetHash = trim(targetHash);

            if (targetHash == "NULL" || targetHash.empty()) {
                cout << RED << "Error: No commits exist yet." << END << endl;
                return false;
            }
        }

        fs::path commitInfoPath = fs::current_path() / ".git" / "commits" / targetHash / "commitInfo.txt";
        if (!fs::exists(commitInfoPath)) {
            cout << RED << "Invalid commit hash: " << targetHash << END << endl;
            return false;
        }

        // Read message from target commit to reuse it
        string line, msg;
        ifstream file(commitInfoPath);
        while (getline(file, line)) {
            if (line[0] == '3') msg = line.substr(2);
        }
        file.close();

        addOnTail(msg + " (Revert of " + targetHash + ")");
        return true;
    }

    /**
     * Walks backward through history using Parent IDs.
     */
    void printCommitList() {
        ifstream headIn(".git/HEAD");
        string currID;
        getline(headIn, currID);
        headIn.close();

        currID = trim(currID);

        while (!currID.empty() && currID != "NULL") {
            fs::path infoPath = fs::current_path() / ".git" / "commits" / currID / "commitInfo.txt";
            if (!fs::exists(infoPath)) break;

            ifstream file(infoPath);
            string line, parentID = "";

            while (getline(file, line)) {
                if (line.empty()) continue;
                switch (line[0]) {
                    case '1': cout << "Commit ID:    " << line.substr(2) << endl; break;
                    case '3': cout << "Commit Msg:   " << line.substr(2) << endl; break;
                    case '4': cout << "Date & Time:  " << line.substr(2) << endl; break;
                    case '2': parentID = line.substr(2); break;
                }
            }
            file.close();

            cout << "============================\n\n";
            currID = trim(parentID);
        }
    }
};