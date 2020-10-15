#include <fstream>
#include <iomanip>
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <dirent.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <stdio.h>

using namespace std;

char** vecStrToChar(vector<string>);

int main(int argc, char* const argv[], char *envp[]) {

  if(setenv("PATH", "bin:.", 1) == -1){
    cerr << "set env err" << endl;
  }
  string wordInCmd;
  string cmdInLine;
  vector<string> cmd;

  while(true){
    wordInCmd.clear();
    cmd.clear();
    cout << "% ";
    // fflush(stdout);
    getline(cin, cmdInLine);
    istringstream in(cmdInLine);
    while (in >> wordInCmd) {
        cmd.push_back(wordInCmd);
    }

    if (cmd.size() == 0){
      continue;
    }else if (cmd[0] == "exit"){
      return 0;
    }else if (cmd[0] == "printenv"){
      if (cmd.size() == 2){
        if (getenv(cmd[1].c_str()) != NULL){
          cout << getenv(cmd[1].c_str()) << endl;
        }else{
          cerr << "Error: no such env" << endl;
        }
      }else{
        cerr << "Error: missing argument" << endl;
      }
    }else if (cmd[0] == "setenv"){
      if (cmd.size() == 3){
        setenv(cmd[1].c_str(), cmd[2].c_str(), 1); // overwrite exist env
      }else{
        cerr << "Error: missing argument" << endl;
      }
    }else{
      pid_t pid;
      pid = fork();
      if (pid == 0){ // child
        // string restOfCmd = cmdInLine.substr(cmd[0].size()+1);
        if(execvp(cmd[0].c_str(), vecStrToChar(cmd)) == -1){
          cerr << "Unknown command: [" << cmd[0] << "]" << endl;
        }
      }else{ // parent
        waitpid(pid, NULL, 0);
      }
    }
    /*else {
      cout << "Unknown command: [" << cmd[0] << "]" << endl;
    }*/
  }

  return 0;
}

char** vecStrToChar(vector<string> cmd){
  char** result = (char**)malloc(sizeof(char*)*cmd.size());
  for(int i = 0; i < cmd.size(); i++){
    result[i] = strdup(cmd[i].c_str());
  }
  return result;
}

