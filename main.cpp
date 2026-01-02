/**
 * MAIN.CPP
 * Purpose: Entry point for the VCS CLI.
 * Maps user terminal input to Manager functions.
 */

#include <iostream>
#include <string>
#include <vector>
#include "manager.cpp"

using namespace std;

// Terminal Colors for CLI UI
#define RED  "\x1B[31m"
#define GRN  "\x1B[32m"
#define YEL  "\x1B[33m"
#define CYN  "\x1B[36m"
#define END  "\033[0m"

// =============================================================================
// HELPER: DISPLAY USAGE
// =============================================================================
void displayHelp() {
    cout << CYN << "\n--- MyGit Version Control System (Minimal) ---" << END << endl;
    cout << "Usage:" << endl;
    cout << "  mygit init                       " << "Initialize a new repository" << endl;
    cout << "  mygit add <. | file_names>       " << "Stage files for commit" << endl;
    cout << "  mygit commit -m \"message\"        " << "Commit staged changes" << endl;
    cout << "  mygit status                     " << "Check status of working tree" << endl;
    cout << "  mygit log                        " << "View commit history" << endl;
    cout << "  mygit revert <hash | HEAD>       " << "Revert to a previous state" << endl;
    cout << "----------------------------------------------\n" << endl;
}

// =============================================================================
// MAIN ENTRY POINT
// =============================================================================
int main(int argc, char *argv[]) {
    gitClass myGit;

    // Handle case with no arguments
    if (argc < 2) {
        displayHelp();
        return 0;
    }

    string command = string(argv[1]);

    // 1. INIT
    if (command == "init") {
        myGit.gitInit();
    }

    // 2. ADD
    else if (command == "add") {
        if (argc == 2) {
            cout << RED << "Error: No files specified. Use '.' to add all or specify file names." << END << endl;
        } 
        else if (argc == 3 && string(argv[2]) == ".") {
            myGit.gitAdd();
        } 
        else {
            // Collect multiple file names from command line
            int numFiles = argc - 2;
            string* files = new string[numFiles];
            for (int i = 0; i < numFiles; i++) {
                files[i] = string(argv[i + 2]);
            }
            myGit.gitAdd(files, numFiles);
            delete[] files; // Clean up memory
        }
    }

    // 3. COMMIT
    else if (command == "commit") {
        if (argc == 4 && string(argv[2]) == "-m") {
            myGit.gitCommit(string(argv[3]));
        } else {
            cout << RED << "Error: Invalid commit syntax." << END << endl;
            cout << "Correct usage: mygit commit -m \"your message\"" << endl;
        }
    }

    // 4. REVERT
    else if (command == "revert") {
        if (argc == 3) {
            if (myGit.gitRevert(string(argv[2]))) {
                cout << GRN << "Successfully created a revert commit." << END << endl;
            }
        } else {
            cout << RED << "Error: Please specify a commit hash or 'HEAD'." << END << endl;
        }
    }

    // 5. LOG
    else if (command == "log") {
        myGit.gitLog();
    }

    // 6. STATUS
    else if (command == "status") {
        myGit.gitStatus();
    }

    // 7. INVALID COMMAND
    else {
        cout << RED << "Unknown command: '" << command << "'" << END << endl;
        displayHelp();
    }

    return 0;
}
